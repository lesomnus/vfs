#include "vfs/impl/os_fs.hpp"
#include "vfs/fs.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>

#include "vfs/impl/file.hpp"
#include "vfs/impl/os_file.hpp"
#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfs.hpp"

namespace fs = std::filesystem;

namespace vfs {

namespace impl {

template<typename C, typename It>
class StdFs::Cursor_: public C {
   public:
	Cursor_(StdFs const& fs, fs::path const& p, fs::directory_options opts)
	    : path_(p)
	    , normal_path_(fs.os_path_of(p))
	    , it_(this->normal_path_, opts)
	    , entry_(fs) {
		this->refresh_();
	}

	[[nodiscard]] directory_entry const& value() const override {
		return this->entry_;
	}

	[[nodiscard]] bool at_end() const override {
		return this->it_ == fs::end(this->it_);
	}

	void increment() override {
		if(this->at_end()) {
			return;
		}

		this->it_++;
		this->refresh_();
	}

   protected:
	void refresh_() {
		if(Cursor_::at_end()) {
			this->entry_ = directory_entry();
		} else {
			auto const p = this->it_->path();
			auto const r = p.lexically_relative(this->normal_path_);

			this->entry_.assign(this->path_ / r);
		}
	}

	fs::path path_;
	fs::path normal_path_;

	It it_;

	directory_entry entry_;
};

class StdFs::RecursiveCursor_: public StdFs::Cursor_<Fs::RecursiveCursor, fs::recursive_directory_iterator> {
   public:
	RecursiveCursor_(StdFs const& fs, fs::path const& p, fs::directory_options opts)
	    : Cursor_(fs, p, opts) { }

	fs::directory_options options() override {
		return this->it_.options();
	}

	[[nodiscard]] int depth() const override {
		return this->it_.depth();
	}

	[[nodiscard]] bool recursion_pending() const override {
		return this->it_.recursion_pending();
	}

	void pop() override {
		this->it_.pop();
		this->refresh_();
	}

	void disable_recursion_pending() override {
		this->it_.disable_recursion_pending();
	}
};

std::shared_ptr<Fs const> StdFs::change_root(fs::path const& p, fs::path const& temp_dir) const {
	return std::make_shared<ChRootedStdFs>(StdFs::canonical(p), "/", temp_dir);
}

std::shared_ptr<File const> StdFs::file_at(fs::path const& p) const {
	// Don't follow a symbolic link.
	auto const c = (fs::canonical(this->os_path_of(p.parent_path())) / p.filename()).lexically_normal();

	auto const status = this->symlink_status(p);
	auto const type   = status.type();
	switch(type) {
	case fs::file_type::regular: {
		return std::make_shared<OsRegularFile>(c);
	}
	case fs::file_type::directory: {
		return std::make_shared<OsDirectory>(c);
	}
	case fs::file_type::symlink: {
		return std::make_shared<OsSymlink>(c);
	}

	default: {
		return std::make_shared<UnkownOsFile>(c);
	}
	}
}

std::shared_ptr<File const> StdFs::file_at_followed(fs::path const& p) const {
	auto const c = fs::canonical(this->os_path_of(p));

	auto const status = this->status(p);
	auto const type   = status.type();
	switch(type) {
	case fs::file_type::regular: {
		return std::make_shared<OsRegularFile>(c);
	}
	case fs::file_type::directory: {
		return std::make_shared<OsDirectory>(c);
	}
	case fs::file_type::symlink: {
		throw std::logic_error("symlink must be followed");
	}

	default: {
		return std::make_shared<UnkownOsFile>(c);
	}
	}
}

std::shared_ptr<Fs::Cursor> StdFs::cursor_(fs::path const& p, fs::directory_options opts) const {
	return std::make_shared<StdFs::Cursor_<Fs::Cursor, fs::directory_iterator>>(*this, p, opts);
}

std::shared_ptr<Fs::RecursiveCursor> StdFs::recursive_cursor_(fs::path const& p, fs::directory_options opts) const {
	return std::make_shared<StdFs::RecursiveCursor_>(*this, p, opts);
}

fs::path ChRootedStdFs::os_path_of(fs::path const& p) const {
	auto const a = this->base_ / (this->cwd_ / p).relative_path();
	auto const c = fs::weakly_canonical(a);
	auto const r = c.lexically_relative(this->base_);
	if(r.empty() || (*r.begin() == ".")) {
		return this->base_;
	}
	if(*r.begin() == "..") {
		auto it = std::find_if(r.begin(), r.end(), [](auto entry) { return entry != ".."; });
		if(it == r.end()) {
			return this->base_;
		}
		return this->base_ / acc_paths(it, r.end());
	}

	return a;
}

}  // namespace impl

std::shared_ptr<Fs> make_os_fs() {
	auto std_fs = std::make_shared<impl::StdFs>(fs::current_path());
	return std::make_shared<impl::OsFsProxy>(std::move(std_fs));
}

}  // namespace vfs
