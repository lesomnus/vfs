#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <vector>

#include "vfs/impl/entry.hpp"

#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class Vfs: public Fs {
   public:
	Vfs(std::filesystem::path const& temp_dir);

	std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override;

	std::filesystem::path canonical(std::filesystem::path const& p) const override;
	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override;
	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) override;
	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) override;

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) override;
	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) override;

	bool create_directory(std::filesystem::path const& p) override;
	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p) override;
	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p, std::error_code& ec) noexcept override;

	bool create_directories(std::filesystem::path const& p) override;
	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override;

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override;
	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override;

	std::filesystem::path current_path() const override;
	std::filesystem::path current_path(std::error_code& ec) const override;

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& override;
	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override;

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept override;
	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept override;

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override;
	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override;

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override;
	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override;

	std::filesystem::path read_symlink(std::filesystem::path const& p) const override;
	std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override;

	bool remove(std::filesystem::path const& p) override;
	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	std::uintmax_t remove_all(std::filesystem::path const& p) override;
	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override;

	std::filesystem::file_status status(std::filesystem::path const& p) const override;
	std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override;
	std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	std::filesystem::path temp_directory_path() const override;
	std::filesystem::path temp_directory_path(std::error_code& ec) const override;

	bool is_empty(std::filesystem::path const& p) const override;
	bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override;

   protected:
	std::shared_ptr<DirectoryEntry const> from_of_(std::filesystem::path const& p) const {
		return p.is_absolute() ? this->root_ : this->cwd_;
	}

	std::shared_ptr<DirectoryEntry>& from_of_(std::filesystem::path const& p) {
		return p.is_absolute() ? this->root_ : this->cwd_;
	}

	std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) const {
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) {
		auto [entry, it] = static_cast<Vfs const*>(this)->navigate(first, last, ec);
		return std::make_pair(std::const_pointer_cast<Entry>(std::move(entry)), it);
	}

	std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last) const {
		std::error_code ec;
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last) {
		std::error_code ec;
		return this->from_of_(*first)->navigate(first, last, ec);
	}

	std::shared_ptr<Entry const> navigate(std::filesystem::path const& p) const {
		return this->from_of_(p)->navigate(p);
	}

	std::shared_ptr<Entry> navigate(std::filesystem::path const& p) {
		return std::const_pointer_cast<Entry>(static_cast<Vfs const*>(this)->navigate(p));
	}

	std::shared_ptr<Entry const> navigate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->from_of_(p)->navigate(p, ec);
	}

	std::shared_ptr<Entry> navigate(std::filesystem::path const& p, std::error_code& ec) {
		return std::const_pointer_cast<Entry>(static_cast<Vfs const*>(this)->navigate(p, ec));
	}

   private:
	Vfs(Vfs const& other, std::filesystem::path const& wd);
	Vfs(Vfs&& other, std::filesystem::path const& wd);

	std::shared_ptr<DirectoryEntry> root_;
	std::shared_ptr<DirectoryEntry> cwd_;
	std::filesystem::path           temp_;
};

}  // namespace impl
}  // namespace vfs
