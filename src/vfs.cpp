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

#include "vfs/impl/entry.hpp"
#include "vfs/impl/file.hpp"
#include "vfs/impl/utils.hpp"

#include "vfs/fs.hpp"

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

Vfs::Vfs(Vfs const& other, fs::path const& wd)
    : root_(other.root_)
    , temp_(other.temp_)
    , cwd_(const_cast<Vfs&>(other).navigate(wd)->must_be<DirectoryEntry>()) { }

Vfs::Vfs(Vfs&& other, fs::path const& wd)
    : root_(std::move(other.root_))
    , temp_(std::move(other.temp_))
    , cwd_(std::move(other.cwd_)) {
	this->cwd_ = this->navigate(wd)->must_be<DirectoryEntry>();
}

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
	return this->navigate(p)->path();
}

fs::path Vfs::canonical(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->canonical(p); }, ec);
}

fs::path Vfs::weakly_canonical(fs::path const& p) const {
	auto ec      = std::error_code();
	auto [f, it] = this->navigate(p.begin(), p.end(), ec);

	auto t = f->path();
	if(it != p.end()) {
		t /= acc_paths(it, p.end());
	}
	return t;
}

fs::path Vfs::weakly_canonical(fs::path const& p, std::error_code& ec) const {
	return handle_error([&] { return this->weakly_canonical(p); }, ec);
}

void Vfs::copy(fs::path const& from, fs::path const& to, fs::copy_options options) {
	auto const src_f = this->navigate(from);
	if(auto const src_r = std::dynamic_pointer_cast<RegularFile>(src_f); src_r) {
		if((options & fs::copy_options::directories_only) == fs::copy_options::directories_only) {
			return;
		}
		if((options & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
			this->create_symlink(from, to);
			return;
		}
		if((options & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links) {
			throw std::runtime_error("not implemented");
		}

		// TODO: optimize.
		auto [dst_f, it] = this->navigate(to.begin(), to.end());
		if(auto const dst_d = std::dynamic_pointer_cast<DirectoryEntry>(dst_f); dst_d && (it == to.end())) {
			this->copy_file(from, to / src_f->name(), options);
		} else {
			this->copy_file(from, to, options);
		}
	}
	if(auto const src_s = std::dynamic_pointer_cast<SymlinkEntry>(src_f); src_s) {
		if((options & fs::copy_options::skip_symlinks) == fs::copy_options::skip_symlinks) {
			return;
		}
		if((options & fs::copy_options::copy_symlinks) == fs::copy_options::copy_symlinks) {
			this->copy_symlink(from, to);
			return;
		}

		throw fs::filesystem_error(
		    "cannot copy symlink",
		    src_f->path(),
		    this->weakly_canonical(to).lexically_normal(),
		    std::make_error_code(std::errc::invalid_argument));
	}

	auto const src_d = std::dynamic_pointer_cast<DirectoryEntry>(src_f);
	if(!src_d) {
		throw fs::filesystem_error(
		    "source is not a not a regular file, a directory, or a symlink",
		    src_f->path(),
		    std::make_error_code(std::errc::invalid_argument));
	}
	if((options & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		throw fs::filesystem_error("", src_f->path(), std::make_error_code(std::errc::is_a_directory));
	}
	if(!(((options & fs::copy_options::recursive) == fs::copy_options::recursive) || (options == fs::copy_options::none))) {
		return;
	}

	auto [dst_f, it] = this->navigate(to.begin(), to.end());
	if(it != to.end()) {
		this->create_directory(from, to);
		return;
	}
	if(dst_f->type() == fs::file_type::regular) {
		throw fs::filesystem_error("", src_f->path(), dst_f->path(), std::make_error_code(std::errc::file_exists));
	}

	auto dst_d = dst_f->must_be<DirectoryEntry>();

	auto const src_p = src_f->path();
	auto const dst_p = dst_f->path();

	// TODO: optimize
	for(auto const [name, f]: src_d->typed_file()->files) {
		this->copy(src_p / name, dst_p / name, options);
	}
}

void Vfs::copy(fs::path const& from, fs::path const& to, fs::copy_options options, std::error_code& ec) {
	handle_error([&] { this->copy(from, to, options); return 0; }, ec);
}

bool Vfs::copy_file(fs::path const& from, fs::path const& to, fs::copy_options options) {
	auto const src = this->navigate(from)->follow_chain()->must_be<RegularFileEntry>();

	auto const t = this->weakly_canonical(to).lexically_normal();
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
		*dst->open_write() << src->typed_file()->open_read()->rdbuf();
		d->insert(std::make_pair(*it, std::move(dst)));
		return true;
	}
	case 0: {
		auto const r = std::dynamic_pointer_cast<RegularFileEntry>(f);
		if(!r) {
			throw fs::filesystem_error("destination not a regular file", f->path(), std::make_error_code(std::errc::invalid_argument));
		}

		if((options & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) {
			return false;
		} else if((options & fs::copy_options::overwrite_existing) == fs::copy_options::overwrite_existing) {
			auto& dst = *r->typed_file();
			*dst.open_write() << src->typed_file()->open_read()->rdbuf();
			dst.owner = src->owner();
			dst.group = src->group();
			dst.perms = src->perms();
			return true;
		} else if((options & fs::copy_options::update_existing) == fs::copy_options::update_existing) {
			throw std::runtime_error("not implemented");
		} else {
			throw fs::filesystem_error("", src->path(), f->path(), std::make_error_code(std::errc::file_exists));
		}

		// unreachable.
	}
	}
}

bool Vfs::copy_file(fs::path const& from, fs::path const& to, fs::copy_options options, std::error_code& ec) {
	return handle_error([&] { return this->copy_file(from, to, options); }, ec);
}

bool Vfs::create_directory(fs::path const& p) {
	return this->create_directory(p, fs::path("/"));
}

bool Vfs::create_directory(fs::path const& p, std::error_code& ec) noexcept {
	return this->create_directory(p, fs::path("/"), ec);
}

bool Vfs::create_directory(fs::path const& p, fs::path const& existing_p) {
	auto const t = this->weakly_canonical(p).lexically_normal();
	auto [f, it] = this->navigate(t.begin(), t.end());

	switch(std::distance(it, t.end())) {
	default: {  // v > 1
		throw fs::filesystem_error("", t, std::make_error_code(std::errc::no_such_file_or_directory));
	}
	case 0: {
		if(std::dynamic_pointer_cast<DirectoryEntry>(f)) {
			return false;
		} else {
			throw fs::filesystem_error("", t, std::make_error_code(std::errc::file_exists));
		}
	}
	case 1: {
		break;
	}
	}

	// TODO: must_be and should_be

	auto const other_d = this->navigate(existing_p)->must_be<DirectoryEntry>();
	f->must_be<DirectoryEntry>()->insert(std::make_pair(*it, std::make_shared<Directory>(*other_d->typed_file())));

	return true;
}

bool Vfs::create_directory(fs::path const& p, fs::path const& existing_p, std::error_code& ec) noexcept {
	return handle_error([&] { return this->create_directory(p, existing_p); }, ec);
}

bool Vfs::create_directories(fs::path const& p) {
	auto const t = this->weakly_canonical(p).lexically_normal();
	auto [f, it] = this->navigate(t.begin(), t.end());
	if(it == t.end()) {
		return false;
	}

	auto prev = f->must_be<DirectoryEntry>()->typed_file();
	for(; it != t.end(); ++it) {
		auto d = std::make_shared<Directory>(0, 0);
		prev->files.insert(std::make_pair(*it, d));
		prev = std::move(d);
	}

	return true;
}

bool Vfs::create_directories(fs::path const& p, std::error_code& ec) {
	return handle_error([&] { return this->create_directories(p); }, ec);
}

void Vfs::create_symlink(fs::path const& target, fs::path const& link) {
	auto const t = this->weakly_canonical(link).lexically_normal();
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
	return std::shared_ptr<Vfs>(new Vfs(*this, p));
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p) && {
	return std::shared_ptr<Vfs>(new Vfs(std::move(*this), p));
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p, std::error_code& ec) const& noexcept {
	return handle_error([&] { return this->current_path(p); }, ec);
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p, std::error_code& ec) && noexcept {
	return handle_error([&] { return std::move(*this).current_path(p); }, ec);
}

bool Vfs::equivalent(fs::path const& p1, fs::path const& p2) const {
	auto const f1 = this->navigate(p1)->follow_chain();
	auto const f2 = this->navigate(p2)->follow_chain();

	return f1->holds_same_file_with(*f2);
}

bool Vfs::equivalent(fs::path const& p1, fs::path const& p2, std::error_code& ec) const noexcept {
	return handle_error([&] { return this->equivalent(p1, p2); }, ec);
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
		throw std::invalid_argument("unexpected value of \"std::filesystem::perm_options\": " + std::to_string(v));
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

}  // namespace impl

std::shared_ptr<Fs> make_vfs(std::filesystem::path const& temp_dir) {
	return std::make_shared<impl::Vfs>(temp_dir);
}

}  // namespace vfs
