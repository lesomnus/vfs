#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <system_error>
#include <utility>

#include "vfs/impl/fs.hpp"

#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class FsProxy: public FsBase {
   public:
	FsProxy(std::shared_ptr<Fs> fs)
	    : fs_(fs_base(fs)) { }

	FsProxy(std::shared_ptr<FsBase> fs)
	    : fs_(std::move(fs)) { }

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode) const override {
		return this->fs_->open_read(filename, mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode) override {
		return this->fs_->open_write(filename, mode);
	}

	[[nodiscard]] std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override {
		auto fs = this->fs_->change_root(p, temp_dir);
		return this->make_proxy_(std::move(fs));
	}

	std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		auto fs = this->fs_->change_root(p, temp_dir);
		return this->make_proxy_(std::move(fs));
	}

	void mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) override {
		this->fs_->mount(target, other, source);
	}

	void unmount(std::filesystem::path const& target) override {
		this->fs_->unmount(target);
	}

	[[nodiscard]] std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return this->fs_->canonical(p);
	}

	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return this->fs_->canonical(p, ec);
	}

	[[nodiscard]] std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		return this->fs_->weakly_canonical(p);
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return this->fs_->weakly_canonical(p, ec);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		this->fs_->copy(src, dst, opts);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		this->fs_->copy(src, dst, opts, ec);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		return this->fs_->copy_file(src, dst, opts);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		return this->fs_->copy_file(src, dst, opts, ec);
	}

	bool create_directory(std::filesystem::path const& p) override {
		return this->fs_->create_directory(p);
	}

	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return this->fs_->create_directory(p, ec);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) override {
		return this->fs_->create_directory(p, attr);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept override {
		return this->fs_->create_directory(p, attr, ec);
	}

	bool create_directories(std::filesystem::path const& p) override {
		return this->fs_->create_directories(p);
	}

	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override {
		return this->fs_->create_directories(p, ec);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override {
		return this->fs_->create_hard_link(target, link);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		return this->fs_->create_hard_link(target, link, ec);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override {
		return this->fs_->create_symlink(target, link);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		return this->fs_->create_symlink(target, link, ec);
	}

	[[nodiscard]] std::filesystem::path current_path() const override {
		return this->fs_->current_path();
	}

	std::filesystem::path current_path(std::error_code& ec) const override {
		return this->fs_->current_path(ec);
	}

	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const override {
		return this->make_proxy_(this->fs_->current_path(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		auto fs = this->fs_->current_path(p, ec);
		if(ec) {
			return nullptr;
		}

		return this->make_proxy_(std::move(fs));
	}

	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override {
		return this->fs_->equivalent(p1, p2);
	}

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override {
		return this->fs_->equivalent(p1, p2, ec);
	}

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p) const override {
		return this->fs_->file_size(p);
	}

	std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->file_size(p, ec);
	}

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p) const override {
		return this->fs_->hard_link_count(p);
	}

	std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->hard_link_count(p, ec);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override {
		return this->fs_->last_write_time(p);
	}

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->last_write_time(p, ec);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override {
		this->fs_->last_write_time(p, t);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override {
		this->fs_->last_write_time(p, t, ec);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts) override {
		this->fs_->permissions(p, prms, opts);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override {
		this->fs_->permissions(p, prms, opts, ec);
	}

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p) const override {
		return this->fs_->read_symlink(p);
	}

	std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override {
		return this->fs_->read_symlink(p, ec);
	}

	bool remove(std::filesystem::path const& p) override {
		return this->fs_->remove(p);
	}

	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return this->fs_->remove(p, ec);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p) override {
		return this->fs_->remove_all(p);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override {
		return this->fs_->remove_all(p, ec);
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override {
		this->fs_->rename(src, dst);
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override {
		this->fs_->rename(src, dst, ec);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override {
		this->fs_->resize_file(p, n);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override {
		this->fs_->resize_file(p, n, ec);
	}

	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p) const override {
		return this->fs_->space(p);
	}

	std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->space(p, ec);
	}

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p) const override {
		return this->fs_->status(p);
	}

	std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->status(p, ec);
	}

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override {
		return this->fs_->symlink_status(p);
	}

	std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return this->fs_->symlink_status(p, ec);
	}

	[[nodiscard]] std::filesystem::path temp_directory_path() const override {
		return this->fs_->temp_directory_path();
	}

	std::filesystem::path temp_directory_path(std::error_code& ec) const override {
		return this->fs_->temp_directory_path(ec);
	}

	[[nodiscard]] bool is_empty(std::filesystem::path const& p) const override {
		return this->fs_->is_empty(p);
	}

	bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override {
		return this->fs_->is_empty(p, ec);
	}

	[[nodiscard]] std::shared_ptr<File const> file_at(std::filesystem::path const& p) const override {
		return this->fs_->file_at(p);
	}

	std::shared_ptr<File> file_at(std::filesystem::path const& p) override {
		return this->fs_->file_at(p);
	}

	[[nodiscard]] std::shared_ptr<File const> file_at_followed(std::filesystem::path const& p) const override {
		return this->fs_->file_at_followed(p);
	}

	std::shared_ptr<File> file_at_followed(std::filesystem::path const& p) override {
		return this->fs_->file_at_followed(p);
	}

	[[nodiscard]] std::shared_ptr<Directory const> cwd() const override {
		return this->fs_->cwd();
	}

	std::shared_ptr<Directory> cwd() override {
		return this->fs_->cwd();
	}

	[[nodiscard]] std::shared_ptr<FsBase const> source_fs() const {
		return this->fs_;
	}

	std::shared_ptr<FsBase> const& source_fs() {
		return this->fs_;
	}

   protected:
	void copy_(std::filesystem::path const& src, Fs& other, std::filesystem::path const& dst, std::filesystem::copy_options opts) const override {
		this->fs_->copy(src, other, dst, opts);
	}

	[[nodiscard]] virtual std::shared_ptr<FsProxy> make_proxy_(std::shared_ptr<Fs> fs) const {
		return std::make_shared<FsProxy>(std::move(fs));
	}

	[[nodiscard]] std::shared_ptr<Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return Fs::cursor_of_(*this->fs_, p, opts);
	}

	[[nodiscard]] std::shared_ptr<RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return Fs::recursive_cursor_of_(*this->fs_, p, opts);
	}

	std::shared_ptr<FsBase> fs_;
};

template<std::derived_from<Fs> T>
T* fs_cast(std::conditional_t<std::is_const_v<T>, Fs const, Fs>* fs) {
	if(auto proxy = dynamic_cast<std::conditional_t<std::is_const_v<T>, FsProxy const, FsProxy>*>(fs); proxy) {
		return fs_cast<T>(proxy->source_fs().get());
	}

	return dynamic_cast<T*>(fs);
}

}  // namespace impl
}  // namespace vfs
