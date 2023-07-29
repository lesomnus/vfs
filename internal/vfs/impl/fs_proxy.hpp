#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

#include "vfs/impl/fs.hpp"
#include "vfs/impl/utils.hpp"

#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

template<typename T>
requires std::same_as<std::remove_const_t<T>, FsBase>
class BasicFsProxy: public FsBase {
   public:
	BasicFsProxy(std::conditional_t<std::is_const_v<T>, Fs const, Fs>& fs)
	    : fs_(fs_base(fs.shared_from_this())) { }

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode) const override {
		return this->fs_->open_read(filename, mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode) override {
		return this->mutable_fs_()->open_write(filename, mode);
	}

	[[nodiscard]] std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override {
		auto fs = this->fs_->change_root(p, temp_dir);
		return this->make_proxy_(std::move(fs));
	}

	[[nodiscard]] std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		auto fs = this->mutable_fs_()->change_root(p, temp_dir);
		return this->make_proxy_(std::move(fs));
	}

	void mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) override {
		this->mutable_fs_()->mount(target, other, source);
	}

	void unmount(std::filesystem::path const& target) override {
		this->mutable_fs_()->unmount(target);
	}

	[[nodiscard]] std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return this->fs_->canonical(p);
	}

	[[nodiscard]] std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		return this->fs_->weakly_canonical(p);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		this->mutable_fs_()->copy(src, dst, opts);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		return this->mutable_fs_()->copy_file(src, dst, opts);
	}

	bool create_directory(std::filesystem::path const& p) override {
		return this->mutable_fs_()->create_directory(p);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) override {
		return this->mutable_fs_()->create_directory(p, attr);
	}

	bool create_directories(std::filesystem::path const& p) override {
		return this->mutable_fs_()->create_directories(p);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override {
		return this->mutable_fs_()->create_hard_link(target, link);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override {
		return this->mutable_fs_()->create_symlink(target, link);
	}

	[[nodiscard]] std::filesystem::path current_path() const override {
		return this->fs_->current_path();
	}

	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p) const override {
		return this->make_proxy_(this->fs_->current_path(p));
	}

	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p) override {
		return this->make_proxy_(this->mutable_fs_()->current_path(p));
	}

	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override {
		return this->fs_->equivalent(p1, p2);
	}

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p) const override {
		return this->fs_->file_size(p);
	}

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p) const override {
		return this->fs_->hard_link_count(p);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override {
		return this->fs_->last_write_time(p);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override {
		this->mutable_fs_()->last_write_time(p, t);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts) override {
		this->mutable_fs_()->permissions(p, prms, opts);
	}

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p) const override {
		return this->fs_->read_symlink(p);
	}

	bool remove(std::filesystem::path const& p) override {
		return this->mutable_fs_()->remove(p);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p) override {
		return this->mutable_fs_()->remove_all(p);
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override {
		this->mutable_fs_()->rename(src, dst);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override {
		this->mutable_fs_()->resize_file(p, n);
	}

	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p) const override {
		return this->fs_->space(p);
	}

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p) const override {
		return this->fs_->status(p);
	}

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override {
		return this->fs_->symlink_status(p);
	}

	[[nodiscard]] std::filesystem::path temp_directory_path() const override {
		return this->fs_->temp_directory_path();
	}

	[[nodiscard]] bool is_empty(std::filesystem::path const& p) const override {
		return this->fs_->is_empty(p);
	}

	[[nodiscard]] std::shared_ptr<File const> file_at(std::filesystem::path const& p) const override {
		return this->fs_->file_at(p);
	}

	[[nodiscard]] std::shared_ptr<File> file_at(std::filesystem::path const& p) override {
		return this->mutable_fs_()->file_at(p);
	}

	[[nodiscard]] std::shared_ptr<File const> file_at_followed(std::filesystem::path const& p) const override {
		return this->fs_->file_at_followed(p);
	}

	[[nodiscard]] std::shared_ptr<File> file_at_followed(std::filesystem::path const& p) override {
		return this->mutable_fs_()->file_at_followed(p);
	}

	[[nodiscard]] std::shared_ptr<Directory const> cwd() const override {
		return this->fs_->cwd();
	}

	[[nodiscard]] std::shared_ptr<Directory> cwd() override {
		return this->mutable_fs_()->cwd();
	}

	[[nodiscard]] std::shared_ptr<FsBase const> source_fs() const {
		return this->fs_;
	}

	[[nodiscard]] auto const& source_fs() {
		return this->fs_;
	}

   protected:
	[[nodiscard]] std::shared_ptr<std::remove_const_t<FsBase>> const& mutable_fs_() const {
		if constexpr(std::is_const_v<T>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->fs_;
		}
	}

	void copy_(std::filesystem::path const& src, Fs& other, std::filesystem::path const& dst, std::filesystem::copy_options opts) const override {
		this->fs_->copy(src, other, dst, opts);
	}

	[[nodiscard]] virtual std::shared_ptr<BasicFsProxy<T const> const> make_proxy_(std::shared_ptr<Fs const> fs) const {
		return std::make_shared<BasicFsProxy<T const>>(*fs);
	}

	[[nodiscard]] virtual std::shared_ptr<BasicFsProxy<T>> make_proxy_(std::shared_ptr<Fs> fs) {
		return std::make_shared<BasicFsProxy<T>>(*fs);
	}

	[[nodiscard]] std::shared_ptr<Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return Fs::cursor_of_(*this->fs_, p, opts);
	}

	[[nodiscard]] std::shared_ptr<RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return Fs::recursive_cursor_of_(*this->fs_, p, opts);
	}

	std::shared_ptr<std::conditional_t<std::is_const_v<T>, FsBase const, FsBase>> fs_;
};

using FsProxy         = BasicFsProxy<FsBase>;
using ReadOnlyFsProxy = BasicFsProxy<FsBase const>;

template<std::derived_from<Fs> T>
T* fs_cast(std::conditional_t<std::is_const_v<T>, Fs const, Fs>* fs) {
	if(auto proxy = dynamic_cast<std::conditional_t<std::is_const_v<T>, FsProxy const, FsProxy>*>(fs); proxy) {
		return fs_cast<T>(proxy->source_fs().get());
	}

	return dynamic_cast<T*>(fs);
}

}  // namespace impl
}  // namespace vfs
