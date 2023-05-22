#include "vfs/impl/vfs.hpp"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include "vfs/fs.hpp"

#include "vfs/impl/entry.hpp"
#include "vfs/impl/file.hpp"
#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {

namespace impl {

Vfs::Vfs(fs::path const& temp_dir)
    : root_(DirectoryEntry::make_root())
    , temp_((fs::path("/") / temp_dir).lexically_normal())
    , cwd_(this->root_) {
	auto d = Directory::make_temp();
	this->create_directories(this->temp_);
}

Vfs::Vfs(Vfs const& other, DirectoryEntry& wd)
    : root_(other.root_)
    , temp_(other.temp_)
    , cwd_(wd.shared_from_this()->must_be<DirectoryEntry>()) { }

Vfs::Vfs(Vfs&& other, DirectoryEntry& wd)
    : root_(std::move(other.root_))
    , temp_(std::move(other.temp_))
    , cwd_(wd.shared_from_this()->must_be<DirectoryEntry>()) { }

std::shared_ptr<std::istream> Vfs::open_read(fs::path const& filename, std::ios_base::openmode mode) const {
	constexpr auto fail = [] {
		auto f = std::make_shared<std::ifstream>();
		f->setstate(std::ios_base::failbit);
		return f;
	};

	mode |= std::ios_base::in;

	auto ec = std::error_code();
	auto f  = this->navigate(filename, ec);
	if(ec) {
		return fail();
	}

	auto r = std::dynamic_pointer_cast<RegularFileEntry const>(std::move(f));
	if(!r) {
		return fail();
	}

	return r->typed_file()->open_read(mode);
}

std::shared_ptr<std::ostream> Vfs::open_write(fs::path const& filename, std::ios_base::openmode mode) {
	mode |= std::ios_base::out;

	constexpr auto fail = [] {
		auto f = std::make_shared<std::ofstream>();
		f->setstate(std::ios_base::failbit);
		return f;
	};

	auto ec      = std::error_code();
	auto [f, it] = this->navigate(filename.begin(), filename.end(), ec);
	if(!ec) {
		auto const r = std::dynamic_pointer_cast<RegularFile>(std::move(f));
		if(r) {
			return r->open_write(mode);
		} else {
			return fail();
		}
	}

	auto d = std::dynamic_pointer_cast<DirectoryEntry>(std::move(f));
	if(!d) {
		return fail();
	}

	auto const name = acc_paths(it, filename.end());
	if(std::distance(name.begin(), name.end()) > 1) {
		// No such directory.
		return fail();
	}

	if(name.empty() || name.is_absolute() || name == "." || name == "..") {
		// Not a filename.
		return fail();
	}

	auto r = std::make_shared<TempFile>(0, 0);

	auto stream = r->open_write(mode);
	d->insert(std::make_pair(name.filename(), std::move(r)));
	return stream;
}

fs::path Vfs::canonical(fs::path const& p) const {
	return this->navigate(p)->follow_chain()->path();
}

fs::path Vfs::canonical(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->canonical(p); }, ec);
}

fs::path Vfs::weakly_canonical(fs::path const& p) const {
	auto ec      = std::error_code();
	auto [f, it] = this->navigate(p.begin(), p.end(), ec);
	if(it == p.begin()) {
		return p.lexically_normal();
	}

	auto t = f->follow_chain()->path();
	if(it != p.end()) {
		t /= acc_paths(it, p.end());
	}
	return t.lexically_normal();
}

fs::path Vfs::weakly_canonical(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->weakly_canonical(p); }, ec);
}

void Vfs::copy(fs::path const& src, fs::path const& dst, fs::copy_options opts) {
	auto const src_f = this->navigate(src);
	if(auto const src_r = std::dynamic_pointer_cast<RegularFileEntry>(src_f); src_r) {
		if((opts & fs::copy_options::directories_only) == fs::copy_options::directories_only) {
			return;
		}
		if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
			this->create_symlink(src, dst);
			return;
		}
		if((opts & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links) {
			throw std::runtime_error("not implemented");
		}

		// TODO: optimize.
		auto [dst_f, it] = this->navigate(dst.begin(), dst.end());
		if(auto const dst_d = std::dynamic_pointer_cast<DirectoryEntry>(dst_f); dst_d && (it == dst.end())) {
			this->copy_file(src, dst / src_f->name(), opts);
		} else {
			this->copy_file(src, dst, opts);
		}
		return;
	}
	if(auto const src_s = std::dynamic_pointer_cast<SymlinkEntry>(src_f); src_s) {
		if((opts & fs::copy_options::skip_symlinks) == fs::copy_options::skip_symlinks) {
			return;
		}
		if((opts & fs::copy_options::copy_symlinks) == fs::copy_options::copy_symlinks) {
			this->copy_symlink(src, dst);
			return;
		}

		throw fs::filesystem_error(
		    "cannot copy symlink",
		    src_f->path(),
		    this->weakly_canonical(dst).lexically_normal(),
		    std::make_error_code(std::errc::invalid_argument));
	}

	auto const src_d = std::dynamic_pointer_cast<DirectoryEntry>(src_f);
	if(!src_d) {
		throw fs::filesystem_error(
		    "source is not a not a regular file, a directory, or a symlink",
		    src_f->path(),
		    std::make_error_code(std::errc::invalid_argument));
	}
	if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		throw fs::filesystem_error("", src_f->path(), std::make_error_code(std::errc::is_a_directory));
	}
	if(!(((opts & fs::copy_options::recursive) == fs::copy_options::recursive) || (opts == fs::copy_options::none))) {
		return;
	}

	auto [dst_f, it] = this->navigate(dst.begin(), dst.end());
	if(it != dst.end()) {
		this->create_directory(dst, src);
		dst_f = dst_f->must_be<DirectoryEntry>()->navigate(acc_paths(it, dst.end()));
	} else if(dst_f->type() != fs::file_type::directory) {
		throw fs::filesystem_error("", src_f->path(), dst_f->path(), std::make_error_code(std::errc::file_exists));
	}

	auto const src_p = src_f->path();
	auto const dst_p = dst_f->path();

	// TODO: optimize
	for(auto const [name, f]: src_d->typed_file()->files) {
		this->copy(src_p / name, dst_p / name, opts);
	}
}

void Vfs::copy(fs::path const& src, fs::path const& dst, fs::copy_options opts, std::error_code& ec) {
	handle_error([&] { this->copy(src, dst, opts); return 0; }, ec);
}

bool Vfs::copy_file(fs::path const& src, fs::path const& dst, fs::copy_options opts) {
	auto const src_f = this->navigate(src)->follow_chain();
	auto const src_r = std::dynamic_pointer_cast<RegularFileEntry>(src_f);
	if(!src_r) {
		throw fs::filesystem_error("not a regular file", src_f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	auto const t = this->weakly_canonical(dst).lexically_normal();
	auto [f, it] = this->navigate(t.begin(), t.end());

	auto const distance = std::distance(it, t.end());
	switch(distance) {
	default: {  // v > 1;
		throw fs::filesystem_error("", f->path(), *it, std::make_error_code(std::errc::no_such_file_or_directory));
	}
	case 1: {
		auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f);
		if(!d) {
			throw fs::filesystem_error("", f->path(), *it, std::make_error_code(std::errc::not_a_directory));
		}

		auto dst = std::make_shared<TempFile>(0, 0);
		*dst->open_write() << src_r->typed_file()->open_read()->rdbuf();
		d->insert(std::make_pair(*it, std::move(dst)));
		return true;
	}
	case 0: {
		auto const r = std::dynamic_pointer_cast<RegularFileEntry>(f);
		if(!r) {
			throw fs::filesystem_error("destination not a regular file", f->path(), std::make_error_code(std::errc::invalid_argument));
		}

		if((opts & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) {
			return false;
		} else if((opts & fs::copy_options::overwrite_existing) == fs::copy_options::overwrite_existing) {
			auto& dst = *r->typed_file();
			*dst.open_write() << src_r->typed_file()->open_read()->rdbuf();
			dst.owner = src_r->owner();
			dst.group = src_r->group();
			dst.perms = src_r->perms();
			return true;
		} else if((opts & fs::copy_options::update_existing) == fs::copy_options::update_existing) {
			throw std::runtime_error("not implemented");
		} else {
			throw fs::filesystem_error("", src_r->path(), f->path(), std::make_error_code(std::errc::file_exists));
		}

		// unreachable.
	}
	}

	// unreachable.
}

bool Vfs::copy_file(fs::path const& src, fs::path const& dst, fs::copy_options opts, std::error_code& ec) {
	return handle_error([&] { return this->copy_file(src, dst, opts); }, ec);
}

bool Vfs::create_directory(fs::path const& p) {
	return this->create_directory(p, fs::path("/"));
}

bool Vfs::create_directory(fs::path const& p, std::error_code& ec) noexcept {
	return this->create_directory(p, fs::path("/"), ec);
}

bool Vfs::create_directory(fs::path const& p, fs::path const& attr) {
	auto const t = this->weakly_canonical(p);
	auto [f, it] = this->navigate(t.begin(), t.end());

	switch(std::distance(it, t.end())) {
	default: {  // v > 1
		throw fs::filesystem_error("", f->path(), *it, std::make_error_code(std::errc::no_such_file_or_directory));
	}
	case 0: {
		if(std::dynamic_pointer_cast<DirectoryEntry>(f)) {
			return false;
		} else {
			throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::file_exists));
		}
	}
	case 1: {
		break;
	}
	}

	auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f);
	if(!d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::not_a_directory));
	}

	auto const other_f = this->navigate(attr);
	auto const other_d = std::dynamic_pointer_cast<DirectoryEntry>(other_f);
	if(!other_d) {
		throw fs::filesystem_error("", other_f->path(), std::make_error_code(std::errc::not_a_directory));
	}

	d->insert(std::make_pair(*it, std::make_shared<Directory>(0, 0)));
	d->perms(other_d->perms());

	return true;
}

bool Vfs::create_directory(fs::path const& p, fs::path const& attr, std::error_code& ec) noexcept {
	return handle_error([&] { return this->create_directory(p, attr); }, ec);
}

bool Vfs::create_directories(fs::path const& p) {
	auto const t = this->weakly_canonical(p);
	auto [f, it] = this->navigate(t.begin(), t.end());
	if(it == t.end()) {
		return false;
	}

	auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f);
	if(!d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::not_a_directory));
	}

	auto prev = f->must_be<DirectoryEntry>()->typed_file();
	for(; it != t.end(); ++it) {
		auto curr = std::make_shared<Directory>(0, 0);
		prev->files.insert(std::make_pair(*it, curr));
		prev = std::move(curr);
	}

	return true;
}

bool Vfs::create_directories(fs::path const& p, std::error_code& ec) {
	return handle_error([&] { return this->create_directories(p); }, ec);
}

void Vfs::create_hard_link(fs::path const& target, fs::path const& link) {
	auto const dst_f = this->navigate(target);

	auto const src_p = this->weakly_canonical(link);
	auto const prev  = this->navigate(src_p.parent_path() / "")->must_be<DirectoryEntry>();
	prev->insert(std::make_pair(src_p.filename(), dst_f->file()));
}

void Vfs::create_hard_link(fs::path const& target, fs::path const& link, std::error_code& ec) noexcept {
	handle_error([&] { this->create_hard_link(target, link); return 0; }, ec);
}

void Vfs::create_symlink(fs::path const& target, fs::path const& link) {
	auto const t = this->weakly_canonical(link);
	auto [f, it] = this->navigate(t.begin(), t.end());

	switch(std::distance(it, t.end())) {
	default: {  // v > 1
		throw fs::filesystem_error("", f->path(), *it, std::make_error_code(std::errc::no_such_file_or_directory));
	}
	case 0: {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::file_exists));
	}
	case 1: {
		break;
	}
	}

	auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f);
	if(!d) {
		throw fs::filesystem_error("", f->path(), *it, std::make_error_code(std::errc::not_a_directory));
	}

	d->insert(std::make_pair(*it, std::make_shared<Symlink>(target)));
}

void Vfs::create_symlink(fs::path const& target, fs::path const& link, std::error_code& ec) noexcept {
	handle_error([&] {this->create_symlink(target, link); return 0; }, ec);
}

fs::path Vfs::current_path() const {
	return this->cwd_->path();
}

fs::path Vfs::current_path(std::error_code& ec) const {
	return this->cwd_->path();
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p) const& {
	auto f = this->navigate(p);
	auto d = std::dynamic_pointer_cast<DirectoryEntry const>(f);
	if(!d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::not_a_directory));
	}

	return std::shared_ptr<Vfs>(new Vfs(*this, const_cast<DirectoryEntry&>(*d)));
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p) && {
	auto f = this->navigate(p);
	auto d = std::dynamic_pointer_cast<DirectoryEntry>(f);
	if(!d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::not_a_directory));
	}

	return std::shared_ptr<Vfs>(new Vfs(std::move(*this), *d));
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p, std::error_code& ec) const& noexcept {
	return handle_error([&] { return this->current_path(p); }, ec);
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p, std::error_code& ec) && noexcept {
	return handle_error([&] { return std::move(*this).current_path(p); }, ec);
}

std::uintmax_t Vfs::file_size(fs::path const& p) const {
	auto const f = this->navigate(p)->follow_chain();
	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry const>(f); d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::is_a_directory));
	}

	auto const r = std::dynamic_pointer_cast<RegularFileEntry const>(f);
	if(!r) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	return r->typed_file()->size();
}

std::uintmax_t Vfs::file_size(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->file_size(p); }, ec, static_cast<std::uintmax_t>(-1));
}

bool Vfs::equivalent(fs::path const& p1, fs::path const& p2) const {
	std::shared_ptr<Entry const> f1, f2;

	try {
		f1 = this->navigate(p1)->follow_chain();
	} catch(fs::filesystem_error const& err) {
	}

	try {
		f2 = this->navigate(p2)->follow_chain();
	} catch(fs::filesystem_error const& err) {
	}

	if(f1 && f2) {
		return f1->holds_same_file_with(*f2);
	}
	if(f1 || f2) {
		return false;
	}

	throw fs::filesystem_error("", p1, p2, std::make_error_code(std::errc::no_such_file_or_directory));
}

bool Vfs::equivalent(fs::path const& p1, fs::path const& p2, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->equivalent(p1, p2); }, ec);
}

std::uintmax_t Vfs::hard_link_count(fs::path const& p) const {
	auto const f = this->navigate(p)->file();
	return f.use_count() - 1;  // Minus one for being held by `f`.
}

std::uintmax_t Vfs::hard_link_count(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->hard_link_count(p); }, ec, static_cast<std::uintmax_t>(-1));
}

fs::file_time_type Vfs::last_write_time(fs::path const& p) const {
	auto const f = this->navigate(p)->follow_chain();
	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry const>(f); d) {
		// TODO: no throw?
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::is_a_directory));
	}

	auto const r = std::dynamic_pointer_cast<RegularFileEntry const>(f);
	if(!r) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	return r->typed_file()->last_write_time();
}

fs::file_time_type Vfs::last_write_time(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->last_write_time(p); }, ec, fs::file_time_type::min());
}

void Vfs::last_write_time(fs::path const& p, fs::file_time_type t) {
	auto const f = this->navigate(p)->follow_chain();
	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f); d) {
		// TODO: no throw?
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::is_a_directory));
	}

	auto const r = std::dynamic_pointer_cast<RegularFileEntry>(f);
	if(!r) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	r->typed_file()->last_write_time(t);
}

void Vfs::last_write_time(fs::path const& p, fs::file_time_type t, std::error_code& ec) noexcept {
	handle_error([&] { this->last_write_time(p, t); return 0; }, ec);
}

void Vfs::permissions(fs::path const& p, fs::perms prms, fs::perm_options opts) {
	auto f = this->navigate(p);
	if((opts & fs::perm_options::nofollow) != fs::perm_options::nofollow) {
		f = f->follow_chain();
	}

	opts = opts & ~fs::perm_options::nofollow;
	switch(opts) {
	case fs::perm_options::replace: {
		f->perms(prms & fs::perms::mask);
		break;
	}
	case fs::perm_options::add: {
		f->perms(f->perms() | (prms & fs::perms::mask));
		break;
	}
	case fs::perm_options::remove: {
		f->perms(f->perms() & ~(prms & fs::perms::mask));
		break;
	}

	default: {
		auto const v = static_cast<std::underlying_type_t<fs::perm_options>>(opts);
		throw std::invalid_argument("unexpected value of \"fs::perm_options\": " + std::to_string(v));
	}
	}
}

void Vfs::permissions(fs::path const& p, fs::perms prms, fs::perm_options opts, std::error_code& ec) {
	handle_error([&] { this->permissions(p, prms, opts); return 0; }, ec);
}

fs::path Vfs::read_symlink(fs::path const& p) const {
	auto       f = this->navigate(p);
	auto const s = std::dynamic_pointer_cast<SymlinkEntry const>(std::move(f));
	if(!s) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	return s->typed_file()->target();
}

fs::path Vfs::read_symlink(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->read_symlink(p); }, ec);
}

bool Vfs::remove(fs::path const& p) {
	auto       ec = std::error_code();
	auto const f  = this->navigate(p, ec);
	if(ec) {
		return false;
	}

	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f); d && !d->typed_file()->files.empty()) {
		throw fs::filesystem_error("", d->path(), std::make_error_code(std::errc::directory_not_empty));
	}

	auto const ok = f->prev()->typed_file()->files.erase(f->name()) == 1;
	assert(ok);

	return ok;
}

bool Vfs::remove(fs::path const& p, std::error_code& ec) noexcept {
	return handle_error([&] { return this->remove(p); }, ec);
}

std::uintmax_t Vfs::remove_all(fs::path const& p) {
	auto       ec = std::error_code();
	auto const f  = this->navigate(p, ec);
	if(ec) {
		return 0;
	}

	auto const cnt = f->file()->count();
	auto const ok  = f->prev()->typed_file()->files.erase(f->name()) == 1;
	assert(ok);

	return cnt;
}

std::uintmax_t Vfs::remove_all(fs::path const& p, std::error_code& ec) {
	return handle_error([&] { return this->remove(p); }, ec, static_cast<std::uintmax_t>(-1));
}

void Vfs::rename(std::filesystem::path const& src, std::filesystem::path const& dst) {
	auto const src_f = this->navigate(src);

	auto const dst_p = this->weakly_canonical(dst);
	auto const prev  = this->navigate(dst_p.parent_path() / "")->must_be<DirectoryEntry>();

	if(src_f->type() == fs::file_type::directory) {
		auto cursor = std::static_pointer_cast<DirectoryEntry const>(prev);
		do {
			if(src_f->holds_same_file_with(*cursor)) {
				throw fs::filesystem_error("source cannot be an ancestor of destination", src_f->path(), dst_p, std::make_error_code(std::errc::invalid_argument));
			}

			cursor = cursor->prev();
		} while(!cursor->is_root());
	}

	auto const& files = prev->typed_file()->files;
	if(auto const it = files.find(dst_p.filename()); it != files.end()) {
		// Destination is existing file.

		if(src_f->holds(it->second)) {
			// Source file and destination file are equivalent.
			return;
		}

		auto const dst_d = std::dynamic_pointer_cast<Directory>(it->second);
		if(auto const src_d = std::dynamic_pointer_cast<DirectoryEntry>(src_f); src_d) {
			if(!dst_d) {
				// Source is a directory but destination is not.
				throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::not_a_directory));
			}
			if(!dst_d->files.empty()) {
				// Destination is a directory that is not empty.
				throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::directory_not_empty));
			}
		} else {
			if(dst_d) {
				// Source is not a directory but destination is a directory.
				throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::is_a_directory));
			}
		}
	}

	prev->typed_file()->files.insert_or_assign(dst_p.filename(), src_f->file());
	src_f->prev()->typed_file()->files.erase(src_f->name());
}

void Vfs::rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept {
	handle_error([&] { this->rename(src, dst); return 0; }, ec);
}

void Vfs::resize_file(fs::path const& p, std::uintmax_t n) {
	auto const f = this->navigate(p)->follow_chain();
	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f); d) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::is_a_directory));
	}

	auto const r = std::dynamic_pointer_cast<RegularFileEntry>(f);
	if(!r) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	r->typed_file()->resize(n);
}

void Vfs::resize_file(fs::path const& p, std::uintmax_t n, std::error_code& ec) noexcept {
	handle_error([&] {  this->resize_file(p, n); return 0; }, ec);
}

fs::space_info Vfs::space(fs::path const& p) const {
	return this->navigate(p)->follow_chain()->file()->space();
}

fs::space_info Vfs::space(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->space(p); }, ec);
}

fs::file_status Vfs::status(fs::path const& p) const {
	try {
		auto const f = this->navigate(p)->follow_chain();
		return fs::file_status(f->type(), f->perms());
	} catch(fs::filesystem_error const& err) {
		switch(static_cast<std::errc>(err.code().value())) {
		case std::errc::no_such_file_or_directory:
		case std::errc::not_a_directory: {
			return fs::file_status(fs::file_type::not_found);
		}
		}

		throw err;
	}
}

fs::file_status Vfs::status(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->status(p); }, ec);
}

fs::file_status Vfs::symlink_status(fs::path const& p) const {
	try {
		auto const f = this->navigate(p);
		return fs::file_status(f->type(), f->perms());
	} catch(fs::filesystem_error const& err) {
		switch(static_cast<std::errc>(err.code().value())) {
		case std::errc::no_such_file_or_directory:
		case std::errc::not_a_directory: {
			return fs::file_status(fs::file_type::not_found);
		}
		}

		throw err;
	}
}

fs::file_status Vfs::symlink_status(fs::path const& p, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->symlink_status(p); }, ec);
}

fs::path Vfs::temp_directory_path() const {
	return this->temp_;
}

fs::path Vfs::temp_directory_path(std::error_code& ec) const {
	ec.clear();
	return this->temp_;
}

bool Vfs::is_empty(fs::path const& p) const {
	auto const f = this->navigate(p);
	if(auto d = std::dynamic_pointer_cast<DirectoryEntry const>(f); d) {
		return d->typed_file()->files.empty();
	} else if(auto r = std::dynamic_pointer_cast<RegularFile const>(f); r) {
		return r->size() == 0;
	} else {
		throw fs::filesystem_error("cannot determine if file is empty", f->path(), std::make_error_code(std::errc::no_such_file_or_directory));
	}
}

bool Vfs::is_empty(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->is_empty(p); }, ec);
}

std::shared_ptr<Fs::Cursor> Vfs::cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const {
	auto const d = this->navigate(p / "")->must_be<DirectoryEntry>();
	return std::make_shared<Vfs::Cursor>(*this, *d, opts);
}

std::shared_ptr<Fs::RecursiveCursor> Vfs::recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const {
	auto const d = this->navigate(p / "")->must_be<DirectoryEntry>();
	return std::make_shared<Vfs::RecursiveCursor>(*this, *d, opts);
}

Vfs::Cursor::Cursor(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts)
    : opts_(opts)
    , frame_({
          .begin = dir.typed_file()->files.begin(),
          .end   = dir.typed_file()->files.end(),
      }) {
	if(this->frame_.begin != this->frame_.end) {
		this->entry_ = directory_entry(fs, dir.path() / this->frame_.begin->first);
	}
}

void Vfs::Cursor::increment() {
	if(this->at_end()) {
		return;
	}

	this->frame_.begin++;
	if(this->at_end()) {
		this->entry_ = directory_entry();
	} else {
		this->entry_.assign(this->frame_.begin->first);
	}

	return;
}

Vfs::RecursiveCursor::RecursiveCursor(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts)
    : dir_(dir.shared_from_this()->must_be<DirectoryEntry const>())
    , opts_(opts) {
	auto const& files = dir.typed_file()->files;

	Frame frame{
	    .begin = files.begin(),
	    .end   = files.end(),
	};
	if(!frame.at_end()) {
		this->entry_ = directory_entry(fs, dir.path() / frame.begin->first);
		this->frames_.push(std::move(frame));
	}
}

void Vfs::RecursiveCursor::increment() {
	do {
		if(this->at_end()) {
			return;
		}

		auto& frame = this->frames_.top();
		if(frame.at_end()) {
			// Step out current directory.
			this->pop();
			continue;
		}

		auto const curr_name = frame.begin->first;

		// Current iterator must be incremented in any case.
		frame.begin++;

		if(auto d = std::dynamic_pointer_cast<DirectoryEntry const>(this->dir_->next(curr_name)); d && !d->typed_file()->files.empty()) {
			// Step in the directory if directory is not empty.
			auto const& files = d->typed_file()->files;
			this->entry_.assign(d->path() / files.begin()->first);
			this->frames_.push(Frame{
			    .begin = files.begin(),
			    .end   = files.end(),
			});

			this->dir_ = std::move(d);
			return;
		}

		if(!frame.at_end()) {
			this->entry_.replace_filename(frame.begin->first);
			break;
		}
	} while(true);
}

bool Vfs::RecursiveCursor::recursion_pending() const {
	if(this->at_end()) {
		return false;
	}

	auto const& frame = this->frames_.top();
	if(frame.at_end()) {
		return false;
	}

	auto const next = this->dir_->next(frame.begin->first);
	return std::reinterpret_pointer_cast<DirectoryEntry const>(next) != nullptr;
}

void Vfs::RecursiveCursor::pop() {
	if(this->at_end()) {
		return;
	}

	this->dir_ = this->dir_->prev();
	this->entry_.assign(this->entry_.path().parent_path());
	this->frames_.pop();

	// Iterator on parent frame already incremented before step in the current directory.
}

void Vfs::RecursiveCursor::disable_recursion_pending() {
	if(!this->recursion_pending()) {
		return;
	}

	this->frames_.top().begin++;
	this->frames_.push(Frame{});  // Causing step out; makes recursion_pending resulting `false`.
}

}  // namespace impl

std::shared_ptr<Fs> make_vfs(fs::path const& temp_dir) {
	return std::make_shared<impl::Vfs>(temp_dir);
}

}  // namespace vfs
