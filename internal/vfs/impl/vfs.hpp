#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <stack>
#include <system_error>

#include "vfs/impl/entry.hpp"

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class Vfs: public Fs {
   public:
	Vfs(std::filesystem::path const& temp_dir);

	Vfs(Vfs const& other, DirectoryEntry& wd);
	Vfs(Vfs&& other, DirectoryEntry& wd);

	std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override;

	std::filesystem::path canonical(std::filesystem::path const& p) const override;
	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override;
	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override;

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options options) override;
	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options options, std::error_code& ec) override;

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options options) override;
	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options options, std::error_code& ec) override;

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

	std::filesystem::path current_path() const override;
	std::filesystem::path current_path(std::error_code& ec) const override;

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& override;
	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override;

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept override;
	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept override;

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override;
	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override;

	std::uintmax_t file_size(std::filesystem::path const& p) const override;
	std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	std::uintmax_t hard_link_count(std::filesystem::path const& p) const override;
	std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override;
	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override;
	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override;

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override;
	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override;

	std::filesystem::path read_symlink(std::filesystem::path const& p) const override;
	std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override;

	bool remove(std::filesystem::path const& p) override;
	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override;

	std::uintmax_t remove_all(std::filesystem::path const& p) override;
	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override;

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override;
	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override;

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override;
	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override;

	std::filesystem::space_info space(std::filesystem::path const& p) const override;
	std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override;

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

   protected:
	struct Frame {
		bool at_end() const {
			return this->begin == this->end;
		}

		Directory::const_iterator begin;
		Directory::const_iterator end;

		std::shared_ptr<DirectoryEntry const> dir;
	};

	class Cursor: public Fs::Cursor {
	   public:
		Cursor(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts);

		directory_entry const& value() const override {
			return this->entry_;
		}

		bool at_end() const override {
			return this->frame_.at_end();
		}

		void increment() override;

	   private:
		std::filesystem::directory_options opts_;

		Frame           frame_;
		directory_entry entry_;
	};

	class RecursiveCursor: public Fs::RecursiveCursor {
	   public:
		RecursiveCursor(Vfs const& fs, DirectoryEntry const& dir, std::filesystem::directory_options opts);

		directory_entry const& value() const override {
			return this->entry_;
		}

		bool at_end() const override {
			return this->frames_.empty();
		}

		void increment() override;

		std::filesystem::directory_options options() override {
			return this->opts_;
		}

		int depth() const override {
			if(this->frames_.empty()) {
				return 0;
			}
			return this->frames_.size() - 1;
		}

		bool recursion_pending() const override;

		void pop() override;

		void disable_recursion_pending() override;

	   private:
		std::filesystem::directory_options opts_;

		std::stack<Frame> frames_;
		directory_entry   entry_;
	};

	std::shared_ptr<Fs::Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

	std::shared_ptr<Fs::RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

   private:
	std::shared_ptr<DirectoryEntry> root_;
	std::shared_ptr<DirectoryEntry> cwd_;
	std::filesystem::path           temp_;
};

}  // namespace impl
}  // namespace vfs
