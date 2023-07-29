#include "vfs/impl/vfs.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <system_error>
#include <utility>

#include "vfs/fs.hpp"

#include "vfs/impl/entry.hpp"
#include "vfs/impl/mount_point.hpp"
#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfile.hpp"

namespace fs = std::filesystem;

namespace vfs {

namespace impl {

Vfs::Vfs(
    std::shared_ptr<DirectoryEntry> root,
    std::shared_ptr<DirectoryEntry> cwd,
    fs::path                        temp_dir)
    : root_(std::move(root))
    , cwd_(cwd ? std::move(cwd) : this->root_)
    , temp_(std::move(temp_dir)) { }

Vfs::Vfs(
    std::shared_ptr<DirectoryEntry> root,
    fs::path const&                 temp_dir)
    : Vfs(root, nullptr, "") {
	if(temp_dir.empty()) {
		return;
	}

	this->temp_ = (fs::path("/") / temp_dir).lexically_normal();
}

Vfs::Vfs(fs::path const& temp_dir)
    : Vfs(DirectoryEntry::make_root(), temp_dir) { }

Vfs::Vfs(Vfs const& other, DirectoryEntry& wd)
    : Vfs(
        other.root_,
        wd.shared_from_this()->must_be<DirectoryEntry>(),
        other.temp_) { }

Vfs::Vfs(Vfs&& other, DirectoryEntry& wd)
    : Vfs(
        std::move(other.root_),
        wd.shared_from_this()->must_be<DirectoryEntry>(),
        std::move(other.temp_)) { }

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
		if(!r) {
			// File exists but not a regular file.
			return fail();
		}

		return r->open_write(mode);
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

	auto r = d->emplace_regular_file(name);
	return r->open_write(mode);
}

std::shared_ptr<Fs const> Vfs::change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const {
	auto d    = this->navigate(p / "")->must_be<DirectoryEntry const>();
	auto root = std::make_shared<DirectoryEntry>("/", nullptr, std::const_pointer_cast<Directory>(d->typed_file()));

	return std::make_shared<Vfs>(root, nullptr, temp_dir);
}

fs::path Vfs::canonical(fs::path const& p) const {
	return this->navigate(p)->follow_chain()->path();
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

bool Vfs::copy_file(fs::path const& src, fs::path const& dst, fs::copy_options opts) {
	auto const src_f = this->navigate(src)->follow_chain();
	auto const src_r = std::dynamic_pointer_cast<RegularFileEntry>(src_f);
	if(!src_r) {
		throw fs::filesystem_error("not a regular file", src_f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	auto const dst_p = this->weakly_canonical(dst);
	auto const prev  = this->navigate(dst_p.parent_path() / "")->must_be<DirectoryEntry>();
	auto [dst_r, ok] = prev->typed_file()->emplace_regular_file(dst_p.filename());
	if(ok) {
		// Destination does not exists.
		dst_r->copy_from(*src_r->typed_file());
		return true;
	}
	if(!dst_r) {
		throw fs::filesystem_error("destination not a regular file", dst_p, std::make_error_code(std::errc::invalid_argument));
	}

	if((opts & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) {
		return false;
	}
	if((opts & fs::copy_options::overwrite_existing) == fs::copy_options::overwrite_existing) {
		dst_r->copy_from(*src_r->typed_file());
		return true;
	}
	if((opts & fs::copy_options::update_existing) == fs::copy_options::update_existing) {
		if(src_r->typed_file()->last_write_time() < dst_r->last_write_time()) {
			return false;
		}

		dst_r->copy_from(*src_r->typed_file());
		return true;
	}

	throw fs::filesystem_error("", src_r->path(), dst_p, std::make_error_code(std::errc::file_exists));
}

bool Vfs::create_directory(fs::path const& p) {
	return this->create_directory(p, fs::path("/"));
}

bool Vfs::create_directory(fs::path const& p, fs::path const& attr) {
	auto const dst_p = this->weakly_canonical(p);
	auto const prev  = this->navigate(p.parent_path() / "")->must_be<DirectoryEntry>();

	auto [d, ok] = prev->typed_file()->emplace_directory(dst_p.filename());
	if(d) {
		if(ok) {
			auto const oth_d = this->navigate(attr / "")->must_be<DirectoryEntry>();
			d->perms(oth_d->typed_file()->perms(), fs::perm_options::replace);
		}

		return ok;
	}

	auto curr = prev->next(dst_p.filename());
	if(auto s = std::dynamic_pointer_cast<SymlinkEntry>(std::move(curr)); s) {
		// Existing file is symbolic link that linked to a directory; no exception.
		if(std::dynamic_pointer_cast<DirectoryEntry>(s->follow_chain())) {
			return false;
		}
	}

	throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::file_exists));
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

	auto prev = d->typed_file();
	for(; it != t.end(); ++it) {
		auto [curr, ok] = prev->emplace_directory(*it);
		assert(ok);

		prev = std::move(curr);
	}

	return true;
}

void Vfs::create_hard_link(fs::path const& target, fs::path const& link) {
	auto const dst_f = this->navigate(target);
	if(dst_f->file()->type() == fs::file_type::directory) {
		throw fs::filesystem_error("", target, std::make_error_code(std::errc::operation_not_permitted));
	}

	auto const src_p = this->weakly_canonical(link);
	auto const prev  = this->navigate(src_p.parent_path() / "")->must_be<DirectoryEntry>();
	prev->typed_file()->link(src_p.filename(), dst_f->file());
}

void Vfs::create_symlink(fs::path const& target, fs::path const& link) {
	auto const src_p = this->weakly_canonical(link);
	auto const prev  = this->navigate(src_p.parent_path() / "")->must_be<DirectoryEntry>();
	if(prev->typed_file()->contains(src_p.filename())) {
		throw fs::filesystem_error("", src_p, std::make_error_code(std::errc::file_exists));
	}

	prev->emplace_symlink(src_p.filename(), target);
}

fs::path Vfs::current_path() const {
	return this->cwd_->path();
}

fs::path Vfs::current_path(std::error_code& ec) const {
	return this->cwd_->path();
}

std::shared_ptr<Fs const> Vfs::current_path(fs::path const& p) const {
	auto d = this->navigate(p / "")->must_be<DirectoryEntry const>();
	return std::make_shared<Vfs>(*this, const_cast<DirectoryEntry&>(*d));
}

std::shared_ptr<Fs> Vfs::current_path(fs::path const& p) {
	return std::const_pointer_cast<Fs>(static_cast<Vfs const*>(this)->current_path(p));
}

bool Vfs::equivalent(fs::path const& p1, fs::path const& p2) const {
	std::shared_ptr<Entry const> f1;
	std::shared_ptr<Entry const> f2;

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

std::uintmax_t Vfs::hard_link_count(fs::path const& p) const {
	auto const f = this->navigate(p)->file();
	return f.use_count() - 1;  // Minus one for being held by `f`.
}

fs::file_time_type Vfs::last_write_time(fs::path const& p) const {
	auto const f = this->navigate(p)->follow_chain();
	return f->file()->last_write_time();
}

void Vfs::last_write_time(fs::path const& p, fs::file_time_type t) {
	auto const f = this->navigate(p)->follow_chain();
	f->file()->last_write_time(t);
}

void Vfs::permissions(fs::path const& p, fs::perms prms, fs::perm_options opts) {
	auto f = this->navigate(p);
	if((opts & fs::perm_options::nofollow) != fs::perm_options::nofollow) {
		f = f->follow_chain();
	}

	f->file()->perms(prms, opts);
}

fs::path Vfs::read_symlink(fs::path const& p) const {
	auto const f = this->navigate(p);
	auto const s = std::dynamic_pointer_cast<SymlinkEntry const>(f);
	if(!s) {
		throw fs::filesystem_error("", f->path(), std::make_error_code(std::errc::invalid_argument));
	}

	return s->typed_file()->target();
}

bool Vfs::remove(fs::path const& p) {
	auto       ec = std::error_code();
	auto const f  = this->navigate(p, ec);
	if(ec) {
		return false;
	}

	if(auto const d = std::dynamic_pointer_cast<DirectoryEntry>(f); d && !d->typed_file()->empty()) {
		throw fs::filesystem_error("", d->path(), std::make_error_code(std::errc::directory_not_empty));
	}

	auto const cnt = f->prev()->typed_file()->erase(f->name());
	assert(cnt == 1);
	return true;
}

std::uintmax_t Vfs::remove_all(fs::path const& p) {
	auto       ec = std::error_code();
	auto const f  = this->navigate(p, ec);
	if(ec) {
		return 0;
	}

	return f->prev()->typed_file()->erase(f->name());
}

namespace {

void throw_if_not_overwritable_(std::shared_ptr<File const> const& src, std::shared_ptr<File const> const& dst, fs::path const& dst_p) {
	auto const dst_d = std::dynamic_pointer_cast<Directory const>(dst);
	if(src->type() == fs::file_type::directory) {
		if(!dst_d) {
			// Source is a directory but destination is not.
			throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::not_a_directory));
		}
		if(!dst_d->empty()) {
			// Destination is a directory that is not empty.
			throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::directory_not_empty));
		}

		// Source is a directory and destination is empty directory.
	} else {
		if(dst_d) {
			// Source is not a directory but destination is a directory.
			throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::is_a_directory));
		}

		// Both source and destination are not directory.
	}
}

}  // namespace

void Vfs::rename(fs::path const& src, fs::path const& dst) {
	auto const src_f = this->navigate(src);
	if(auto const m = std::dynamic_pointer_cast<MountPoint>(src_f); m) {
		throw fs::filesystem_error("", src, std::make_error_code(std::errc::device_or_resource_busy));
	}

	auto const dst_p = this->weakly_canonical(dst);
	auto const prev  = this->navigate(dst_p.parent_path() / "")->must_be<DirectoryEntry>();

	if(src_f->file()->type() == fs::file_type::directory) {
		auto cursor = std::static_pointer_cast<DirectoryEntry const>(prev);
		do {
			if(src_f->holds_same_file_with(*cursor)) {
				throw fs::filesystem_error("source cannot be an ancestor of destination", src_f->path(), dst_p, std::make_error_code(std::errc::invalid_argument));
			}

			cursor = cursor->prev();
		} while(!cursor->is_root());
	}

	if(auto const f = prev->typed_file()->next(dst_p.filename()); f) {
		// Destination is existing file.

		if(src_f->holds(*f)) {
			// Source file and destination file are equivalent.
			return;
		}

		throw_if_not_overwritable_(src_f->file(), f, dst_p);
		prev->typed_file()->erase(dst_p.filename());
	}

	// Destination does not exist.

	try {
		prev->typed_file()->link(dst_p.filename(), src_f->file());
	} catch(fs::filesystem_error const& error) {
		if(error.code() == std::errc::cross_device_link) {
			this->copy(src, dst_p, fs::copy_options::recursive);
		}

		std::shared_ptr<OsFile> prev_os_f;
		{
			auto prev_f = prev->file();
			if(auto m = std::dynamic_pointer_cast<MountPoint>(std::move(prev_f)); m) {
				prev_f = m->attachment();
			}

			prev_os_f = std::dynamic_pointer_cast<OsFile>(std::move(prev_f));
		}

		auto const src_os_f = std::dynamic_pointer_cast<OsFile>(src_f->file());
		if(!(src_os_f && prev_os_f)) {
			throw error;
		}

		// Both source and destination are OsFile so simply move.
		src_os_f->move_to(prev_os_f->path() / dst_p.filename());

		// Note that prev of source cannot be a VDirectory
		// since if it is, source is a mount point and the source cannot be renamed.
		return;
	}

	src_f->prev()->typed_file()->unlink(src_f->name());
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

fs::space_info Vfs::space(fs::path const& p) const {
	return this->navigate(p)->follow_chain()->file()->space();
}

fs::file_status Vfs::status(fs::path const& p) const {
	try {
		auto const f = this->navigate(p)->follow_chain()->file();
		return fs::file_status(f->type(), f->perms());
	} catch(fs::filesystem_error const& err) {
		switch(static_cast<std::errc>(err.code().value())) {
		case std::errc::no_such_file_or_directory:
		case std::errc::not_a_directory: {
			return fs::file_status(fs::file_type::not_found);
		}

		default: {
			break;
		}
		}

		throw err;
	}
}

fs::file_status Vfs::symlink_status(fs::path const& p) const {
	try {
		auto const f = this->navigate(p)->file();
		return fs::file_status(f->type(), f->perms());
	} catch(fs::filesystem_error const& err) {
		switch(static_cast<std::errc>(err.code().value())) {
		case std::errc::no_such_file_or_directory:
		case std::errc::not_a_directory: {
			return fs::file_status(fs::file_type::not_found);
		}

		default: {
			break;
		}
		}

		throw err;
	}
}

fs::path Vfs::temp_directory_path() const {
	if(this->temp_.empty()) {
		throw fs::filesystem_error("", std::make_error_code(std::errc::no_such_file_or_directory));
	}

	return this->temp_;
}

bool Vfs::is_empty(fs::path const& p) const {
	auto const f = this->navigate(p);
	if(auto d = std::dynamic_pointer_cast<DirectoryEntry const>(f); d) {
		return d->typed_file()->empty();
	}
	if(auto r = std::dynamic_pointer_cast<RegularFile const>(f); r) {
		return r->size() == 0;
	}
	throw fs::filesystem_error("cannot determine if file is empty", f->path(), std::make_error_code(std::errc::no_such_file_or_directory));
}

class Vfs::Cursor_: public Fs::Cursor {
   public:
	Cursor_(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts)
	    : cursor_(dir.typed_file()->cursor())
	    , opts_(opts) {
		if(cursor_->at_end()) {
			return;
		}

		this->entry_ = directory_entry(fs, dir.path() / this->cursor_->name());
	}

	[[nodiscard]] directory_entry const& value() const override {
		return this->entry_;
	}

	[[nodiscard]] bool at_end() const override {
		return this->cursor_->at_end();
	}

	void increment() override {
		this->cursor_->increment();
		if(this->cursor_->at_end()) {
			return;
		}

		this->entry_.replace_filename(this->cursor_->name());
	}

   private:
	std::shared_ptr<Directory::Cursor> cursor_;
	std::filesystem::directory_options opts_;

	directory_entry entry_;
};

class Vfs::RecursiveCursor_: public Fs::RecursiveCursor {
   public:
	RecursiveCursor_(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts)
	    : cwd_(fs.cwd_)
	    , cursors_({dir.typed_file()->cursor()})
	    , opts_(opts) {
		if(this->cursors_.top()->at_end()) {
			this->cursors_.pop();
			return;
		}

		this->entry_ = directory_entry(fs, dir.path() / this->cursors_.top()->name());
	}

	[[nodiscard]] directory_entry const& value() const override {
		return this->entry_;
	}

	[[nodiscard]] bool at_end() const override {
		return this->cursors_.empty();
	}

	void increment() override {
		std::size_t cnt_stepped_out = 0;
		while(!this->at_end()) {
			auto& cursor = *this->cursors_.top();
			if(cursor.at_end()) {
				this->cursors_.pop();
				++cnt_stepped_out;
				continue;
			}

			if(cnt_stepped_out == 0) {
				auto f = cursor.file();
				if((this->opts_ & fs::directory_options::follow_directory_symlink) == fs::directory_options::follow_directory_symlink) {
					if(f->type() == fs::file_type::symlink) {
						f = this->cwd_->navigate(this->entry_.path())->follow_chain()->file();
					}
				}
				if(auto d = std::dynamic_pointer_cast<Directory const>(f); d && !d->empty()) {
					auto c = d->cursor();

					this->entry_.assign(this->entry_.path() / c->name());
					this->cursors_.push(std::move(c));
					return;
				}
			}

			cursor.increment();
			if(cursor.at_end()) {
				continue;
			}

			if(cnt_stepped_out == 0) {
				this->entry_.replace_filename(cursor.name());
			} else {
				auto p = this->entry_.path();
				for(std::size_t i = 0; i < cnt_stepped_out; ++i) {
					p = p.parent_path();
				}

				p.replace_filename(cursor.name());
				this->entry_.assign(p);
			}

			break;
		}
	}

	std::filesystem::directory_options options() override {
		return this->opts_;
	}

	[[nodiscard]] std::size_t depth() const override {
		if(this->cursors_.empty()) {
			return 0;
		}
		return this->cursors_.size() - 1;
	}

	[[nodiscard]] bool recursion_pending() const override {
		if(this->at_end()) {
			return false;
		}

		auto const& cursor = *this->cursors_.top();
		if(cursor.at_end()) {
			// Increment will step out.
			return false;
		}

		auto f = cursor.file();
		if((this->opts_ & fs::directory_options::follow_directory_symlink) == fs::directory_options::follow_directory_symlink) {
			if(f->type() == fs::file_type::symlink) {
				f = this->cwd_->navigate(this->entry_.path())->follow_chain()->file();
			}
		}

		return std::dynamic_pointer_cast<Directory const>(f) != nullptr;
	}

	void pop() override {
		fs::path p = this->entry_.path();
		while(!this->at_end()) {
			this->cursors_.pop();
			if(this->at_end()) {
				return;
			}

			p = p.parent_path();

			auto& cursor = *this->cursors_.top();
			if(cursor.at_end()) {
				continue;
			}

			cursor.increment();
			if(cursor.at_end()) {
				continue;
			}

			this->entry_.assign(p.replace_filename(cursor.name()));
			break;
		}
	}

	void disable_recursion_pending() override {
		if(!this->recursion_pending()) {
			return;
		}

		this->cursors_.top()->increment();
		this->cursors_.push(std::make_shared<Directory::NilCursor>());  // Causing step out; makes recursion_pending resulting `false`.
	}

   private:
	std::shared_ptr<DirectoryEntry>                cwd_;
	std::stack<std::shared_ptr<Directory::Cursor>> cursors_;
	std::filesystem::directory_options             opts_;

	directory_entry entry_;
};

std::shared_ptr<Fs::Cursor> Vfs::cursor_(fs::path const& p, fs::directory_options opts) const {
	auto const d = this->navigate(p / "")->must_be<DirectoryEntry>();
	return std::make_shared<Vfs::Cursor_>(*this, *d, opts);
}

std::shared_ptr<Fs::RecursiveCursor> Vfs::recursive_cursor_(fs::path const& p, fs::directory_options opts) const {
	auto const d = this->navigate(p / "")->must_be<DirectoryEntry>();
	return std::make_shared<Vfs::RecursiveCursor_>(*this, *d, opts);
}

}  // namespace impl

std::shared_ptr<Fs> make_vfs(fs::path const& temp_dir) {
	return std::make_shared<impl::Vfs>(temp_dir);
}

}  // namespace vfs
