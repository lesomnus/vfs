#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <stack>
#include <system_error>

#include "vfs/impl/entry.hpp"
#include "vfs/impl/file.hpp"
#include "vfs/impl/fs.hpp"

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class Vfs: public FsBase {
   public:
	Vfs(
	    std::shared_ptr<DirectoryEntry> root,
	    std::shared_ptr<DirectoryEntry> cwd,
	    std::filesystem::path           temp_dir);

	Vfs(
	    std::shared_ptr<DirectoryEntry> root,
	    std::filesystem::path const&    temp_dir);

	Vfs(std::filesystem::path const& temp_dir);

	Vfs(Vfs const& other, DirectoryEntry& wd);
	Vfs(Vfs&& other, DirectoryEntry& wd);

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream>               open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override;

	[[nodiscard]] std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override;

	[[nodiscard]] std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		return std::const_pointer_cast<Fs>(static_cast<Fs const*>(this)->change_root(p, temp_dir));
	}

	void mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) override;
	void unmount(std::filesystem::path const& target) override;

	std::shared_ptr<DirectoryEntry> const& current_working_directory_entry() {
		return this->cwd_;
	}

	[[nodiscard]] std::filesystem::path canonical(std::filesystem::path const& p) const override;
	std::filesystem::path               canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	[[nodiscard]] std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override;
	std::filesystem::path               weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override;
	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override;

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override;
	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override;

	bool create_directory(std::filesystem::path const& p) override;
	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) override;
	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept override;

	bool create_directories(std::filesystem::path const& p) override;
	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override;

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override;
	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override;

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override;
	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override;

	[[nodiscard]] std::filesystem::path current_path() const override;
	[[nodiscard]] std::filesystem::path current_path(std::error_code& ec) const override;

	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p) const override;
	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p) override;
	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override;
	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p) const override;
	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p) const override;
	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override;
	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override;
	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override;

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override;
	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override;

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p) const override;
	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override;

	bool remove(std::filesystem::path const& p) override;
	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	std::uintmax_t remove_all(std::filesystem::path const& p) override;
	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override;

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override;
	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override;

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override;
	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override;

	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p) const override;
	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p) const override;
	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override;
	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	[[nodiscard]] std::filesystem::path temp_directory_path() const override;
	[[nodiscard]] std::filesystem::path temp_directory_path(std::error_code& ec) const override;

	[[nodiscard]] bool is_empty(std::filesystem::path const& p) const override;
	[[nodiscard]] bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override;

	[[nodiscard]] std::shared_ptr<File const> file_at(std::filesystem::path const& p) const override {
		return this->navigate(p)->file();
	}

	[[nodiscard]] std::shared_ptr<File> file_at(std::filesystem::path const& p) override {
		return std::const_pointer_cast<File>(static_cast<Vfs const*>(this)->file_at(p));
	}

	[[nodiscard]] std::shared_ptr<File const> file_at_followed(std::filesystem::path const& p) const override {
		return this->navigate(p)->follow_chain()->file();
	}

	[[nodiscard]] std::shared_ptr<File> file_at_followed(std::filesystem::path const& p) override {
		return std::const_pointer_cast<File>(static_cast<Vfs const*>(this)->file_at_followed(p));
	}

	[[nodiscard]] std::shared_ptr<Directory const> cwd() const override {
		return this->cwd_->typed_file();
	}

	[[nodiscard]] std::shared_ptr<Directory> cwd() override {
		return std::const_pointer_cast<Directory>(static_cast<Vfs const*>(this)->cwd());
	}

	[[nodiscard]] std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) const {
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	[[nodiscard]] std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) {
		auto [entry, it] = static_cast<Vfs const*>(this)->navigate(first, last, ec);
		return std::make_pair(std::const_pointer_cast<Entry>(std::move(entry)), it);
	}

	[[nodiscard]] std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last) const {
		std::error_code ec;
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	[[nodiscard]] std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last) {
		std::error_code ec;
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	[[nodiscard]] std::shared_ptr<Entry const> navigate(std::filesystem::path const& p) const {
		return this->from_of_(p)->navigate(p);
	}

	[[nodiscard]] std::shared_ptr<Entry> navigate(std::filesystem::path const& p) {
		return std::const_pointer_cast<Entry>(static_cast<Vfs const*>(this)->navigate(p));
	}

	[[nodiscard]] std::shared_ptr<Entry const> navigate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->from_of_(p)->navigate(p, ec);
	}

	[[nodiscard]] std::shared_ptr<Entry> navigate(std::filesystem::path const& p, std::error_code& ec) {
		return std::const_pointer_cast<Entry>(static_cast<Vfs const*>(this)->navigate(p, ec));
	}

   protected:
	class Cursor_;
	class RecursiveCursor_;

	void copy_(std::filesystem::path const& src, Fs& other, std::filesystem::path const& dst, std::filesystem::copy_options opts) const override;

	[[nodiscard]] std::shared_ptr<DirectoryEntry const> from_of_(std::filesystem::path const& p) const {
		return p.is_absolute() ? this->root_ : this->cwd_;
	}

	[[nodiscard]] std::shared_ptr<DirectoryEntry> const& from_of_(std::filesystem::path const& p) {
		return p.is_absolute() ? this->root_ : this->cwd_;
	}

	[[nodiscard]] std::shared_ptr<Fs::Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

	[[nodiscard]] std::shared_ptr<Fs::RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

   private:
	std::shared_ptr<DirectoryEntry> root_;
	std::shared_ptr<DirectoryEntry> cwd_;
	std::filesystem::path           temp_;
};

}  // namespace impl
}  // namespace vfs
