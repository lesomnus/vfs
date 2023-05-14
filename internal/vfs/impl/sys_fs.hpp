#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

#include "vfs/fs.hpp"

#include "vfs/impl/utils.hpp"

namespace vfs {
namespace impl {

class SysFs: public Fs {
   public:
	SysFs(std::filesystem::path const& cwd)
	    : cwd_(std::filesystem::canonical(cwd / "")) { }

	std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override {
		return std::make_shared<std::ifstream>(this->normal_(filename), mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override {
		return std::make_shared<std::ofstream>(this->normal_(filename), mode);
	}

	std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return std::filesystem::canonical(this->normal_(p));
	}

	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::canonical(this->normal_(p), ec);
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		return std::filesystem::weakly_canonical(this->normal_(p));
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::weakly_canonical(this->normal_(p), ec);
	}

	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) override {
		std::filesystem::copy(this->normal_(from), this->normal_(to), options);
	}

	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) override {
		std::filesystem::copy(this->normal_(from), this->normal_(to), options, ec);
	}

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) override {
		return std::filesystem::copy_file(this->normal_(from), this->normal_(to), options);
	}

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) override {
		return std::filesystem::copy_file(this->normal_(from), this->normal_(to), options, ec);
	}

	bool create_directory(std::filesystem::path const& p) override {
		return std::filesystem::create_directory(this->normal_(p));
	}

	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->normal_(p), ec);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p) override {
		return std::filesystem::create_directory(this->normal_(p), this->normal_(existing_p));
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->normal_(p), this->normal_(existing_p), ec);
	}

	bool create_directories(std::filesystem::path const& p) override {
		return std::filesystem::create_directories(this->normal_(p));
	}

	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::create_directories(this->normal_(p), ec);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_hard_link(this->normal_(target), this->normal_(link));
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_hard_link(this->normal_(target), this->normal_(link), ec);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_symlink(target, this->normal_(link));
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_symlink(target, this->normal_(link), ec);
	}

	std::filesystem::path current_path() const override {
		return this->cwd_;
	}

	std::filesystem::path current_path(std::error_code& ec) const override {
		return this->cwd_;
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& override {
		return std::make_shared<SysFs>(this->normal_(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override {
		return std::make_shared<SysFs>(this->normal_(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept override {
		return handle_error([&] { return this->current_path(p); }, ec);
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept override {
		return handle_error([&] { return std::move(*this).current_path(p); }, ec);
	}

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override {
		return std::filesystem::equivalent(this->normal_(p1), this->normal_(p2));
	}

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override {
		return std::filesystem::equivalent(this->normal_(p1), this->normal_(p2), ec);
	}

	std::uintmax_t file_size(std::filesystem::path const& p) const override {
		return std::filesystem::file_size(this->normal_(p));
	}

	std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::file_size(this->normal_(p), ec);
	}

	std::uintmax_t hard_link_count(std::filesystem::path const& p) const override {
		return std::filesystem::hard_link_count(this->normal_(p));
	}

	std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::hard_link_count(this->normal_(p), ec);
	}

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override {
		return std::filesystem::last_write_time(this->normal_(p));
	}

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::last_write_time(this->normal_(p), ec);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type new_time) override {
		std::filesystem::last_write_time(this->normal_(p), new_time);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type new_time, std::error_code& ec) noexcept override {
		std::filesystem::last_write_time(this->normal_(p), new_time, ec);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override {
		std::filesystem::permissions(this->normal_(p), prms, opts);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override {
		std::filesystem::permissions(this->normal_(p), prms, opts, ec);
	}

	std::filesystem::path read_symlink(std::filesystem::path const& p) const override {
		return std::filesystem::read_symlink(this->normal_(p));
	}

	std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::read_symlink(this->normal_(p), ec);
	}

	bool remove(std::filesystem::path const& p) override {
		return std::filesystem::remove(this->normal_(p));
	}

	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::remove(this->normal_(p), ec);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p) override {
		return std::filesystem::remove_all(this->normal_(p));
	}

	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::remove_all(this->normal_(p), ec);
	}

	void rename(std::filesystem::path const& old_p, std::filesystem::path const& new_p) override {
		std::filesystem::rename(this->normal_(old_p), this->normal_(new_p));
	}

	void rename(std::filesystem::path const& old_p, std::filesystem::path const& new_p, std::error_code& ec) noexcept override {
		std::filesystem::rename(this->normal_(old_p), this->normal_(new_p), ec);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t new_size) override {
		std::filesystem::resize_file(this->normal_(p), new_size);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t new_size, std::error_code& ec) noexcept override {
		std::filesystem::resize_file(this->normal_(p), new_size, ec);
	}

	std::filesystem::space_info space(std::filesystem::path const& p) const override {
		return std::filesystem::space(this->normal_(p));
	}

	std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::space(this->normal_(p), ec);
	}

	std::filesystem::file_status status(std::filesystem::path const& p) const override {
		return std::filesystem::status(this->normal_(p));
	}

	std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::status(this->normal_(p), ec);
	}

	std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override {
		return std::filesystem::symlink_status(this->normal_(p));
	}

	std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::symlink_status(this->normal_(p), ec);
	}

	std::filesystem::path temp_directory_path() const override {
		return std::filesystem::temp_directory_path();
	}

	std::filesystem::path temp_directory_path(std::error_code& ec) const override {
		return std::filesystem::temp_directory_path(ec);
	}

	bool is_empty(std::filesystem::path const& p) const override {
		return std::filesystem::is_empty(this->normal_(p));
	}

	bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::is_empty(this->normal_(p), ec);
	}

   private:
	std::filesystem::path normal_(std::filesystem::path const& p) const {
		return p.is_absolute() ? p : (this->cwd_ / p);
	}

	std::filesystem::path cwd_;
};

}  // namespace impl
}  // namespace vfs
