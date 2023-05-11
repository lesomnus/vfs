#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

namespace vfs {

class Fs {
   public:
	virtual ~Fs() = default;

	virtual std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const = 0;
	virtual std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out)     = 0;

	std::filesystem::path absolute(std::filesystem::path const& p) const {
		return p.is_absolute() ? p : this->current_path() / p;
	}

	std::filesystem::path absolute(std::filesystem::path const& p, std::error_code& ec) const {
		return p.is_absolute() ? p : this->current_path(ec) / p;
	}

	virtual std::filesystem::path canonical(std::filesystem::path const& p) const                      = 0;
	virtual std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p) const                      = 0;
	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	std::filesystem::path relative(std::filesystem::path const& p) const {
		return this->relative(p, this->current_path());
	}

	std::filesystem::path relative(std::filesystem::path const& p, std::error_code& ec) const {
		return this->relative(p, this->current_path(), ec);
	}

	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base) const {
		return this->weakly_canonical(p).lexically_relative(this->weakly_canonical(base));
	}

	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_relative(std::filesystem::weakly_canonical(base, ec));
	}

	std::filesystem::path proximate(std::filesystem::path const& p) const {
		return this->proximate(p, this->current_path());
	}

	std::filesystem::path proximate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->proximate(p, this->current_path(), ec);
	}

	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base = std::filesystem::current_path()) const {
		return std::filesystem::weakly_canonical(p).lexically_proximate(std::filesystem::weakly_canonical(base));
	}

	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_proximate(std::filesystem::weakly_canonical(base, ec));
	}

	void copy(std::filesystem::path const& from, std::filesystem::path const& to) {
		this->copy(from, to, std::filesystem::copy_options::none);
	}

	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) {
		this->copy(from, to, std::filesystem::copy_options::none, ec);
	}

	virtual void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options)                      = 0;
	virtual void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) = 0;

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to) {
		return this->copy_file(from, to, std::filesystem::copy_options::none);
	}

	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) {
		return this->copy_file(from, to, std::filesystem::copy_options::none, ec);
	}

	virtual bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options)                      = 0;
	virtual bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) = 0;

	void copy_symlink(std::filesystem::path const& from, std::filesystem::path const& to) {
		auto const target = this->read_symlink(from);
		this->is_directory(target)
		    ? this->create_directory_symlink(target, to)
		    : this->create_symlink(target, to);
	}

	void copy_symlink(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) noexcept {
		if(auto const target = this->read_symlink(from, ec); ec) {
			return;
		} else {
			this->is_directory(target)
			    ? this->create_directory_symlink(target, to, ec)
			    : this->create_symlink(target, to, ec);
		}
	}

	virtual bool create_directory(std::filesystem::path const& p)                               = 0;
	virtual bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept = 0;

	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p)                               = 0;
	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p, std::error_code& ec) noexcept = 0;

	virtual bool create_directories(std::filesystem::path const& p)                      = 0;
	virtual bool create_directories(std::filesystem::path const& p, std::error_code& ec) = 0;

	virtual void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link)                               = 0;
	virtual void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept = 0;

	virtual void create_directory_symlink(std::filesystem::path const& target, std::filesystem::path const& link) {
		this->create_symlink(target, link);
	}

	virtual void create_directory_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept {
		this->create_symlink(target, link, ec);
	}

	virtual std::filesystem::path current_path() const                    = 0;
	virtual std::filesystem::path current_path(std::error_code& ec) const = 0;

	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& = 0;
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p) &&     = 0;

	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept = 0;
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept     = 0;

	bool exists(std::filesystem::file_status s) const noexcept {
		return this->status_known(s) && s.type() != std::filesystem::file_type::not_found;
	}

	bool exists(std::filesystem::path const& p) const {
		return std::filesystem::exists(this->status(p));
	}

	bool exists(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		auto const s = this->status(p, ec);
		if(std::filesystem::status_known(s)) {
			ec.clear();
		}
		return std::filesystem::exists(s);
	}

	virtual bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const                               = 0;
	virtual bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept = 0;

	virtual void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) = 0;
	virtual void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec)                     = 0;

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::error_code& ec) noexcept {
		this->permissions(p, prms, std::filesystem::perm_options::replace, ec);
	}

	virtual std::filesystem::path read_symlink(std::filesystem::path const& p) const                      = 0;
	virtual std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const = 0;

	virtual bool remove(std::filesystem::path const& p)                               = 0;
	virtual bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept = 0;

	virtual std::uintmax_t remove_all(std::filesystem::path const& p)                      = 0;
	virtual std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) = 0;

	virtual std::filesystem::file_status status(std::filesystem::path const& p) const                               = 0;
	virtual std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	virtual std::filesystem::file_status symlink_status(std::filesystem::path const& p) const                               = 0;
	virtual std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	virtual std::filesystem::path temp_directory_path() const                    = 0;
	virtual std::filesystem::path temp_directory_path(std::error_code& ec) const = 0;

	bool is_block_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::block;
	}

	bool is_block_file(std::filesystem::path const& p) const {
		return this->is_block_file(this->status(p));
	}

	bool is_block_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_block_file(this->status(p, ec));
	}

	bool is_character_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::character;
	}

	bool is_character_file(std::filesystem::path const& p) const {
		return this->is_character_file(this->status(p));
	}

	bool is_character_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_character_file(this->status(p, ec));
	}

	bool is_directory(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::directory;
	}

	bool is_directory(std::filesystem::path const& p) const {
		return this->is_directory(this->status(p));
	}

	bool is_directory(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_directory(this->status(p, ec));
	}

	virtual bool is_empty(std::filesystem::path const& p) const                      = 0;
	virtual bool is_empty(std::filesystem::path const& p, std::error_code& ec) const = 0;

	bool is_fifo(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::fifo;
	}

	bool is_fifo(std::filesystem::path const& p) const {
		return this->is_fifo(this->status(p));
	}

	bool is_fifo(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_fifo(this->status(p, ec));
	}

	bool is_other(std::filesystem::file_status s) const noexcept {
		return this->exists(s) && !this->is_regular_file(s) && !this->is_directory(s) && !this->is_symlink(s);
	}

	bool is_other(std::filesystem::path const& p) const {
		return this->is_other(this->status(p));
	}

	bool is_other(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_other(this->status(p, ec));
	}

	bool is_regular_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::regular;
	}

	bool is_regular_file(std::filesystem::path const& p) const {
		return this->is_regular_file(this->status(p));
	}

	bool is_regular_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_regular_file(this->status(p, ec));
	}

	bool is_socket(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::socket;
	}

	bool is_socket(std::filesystem::path const& p) const {
		return this->is_socket(this->status(p));
	}

	bool is_socket(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_socket(this->status(p, ec));
	}

	bool is_symlink(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::symlink;
	}

	bool is_symlink(std::filesystem::path const& p) const {
		return this->is_symlink(this->symlink_status(p));
	}

	bool is_symlink(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_symlink(this->symlink_status(p, ec));
	}

	bool status_known(std::filesystem::file_status s) const noexcept {
		return s.type() != std::filesystem::file_type::none;
	}
};

std::shared_ptr<Fs> make_vfs(std::filesystem::path const& temp_dir = "/tmp");

}  // namespace vfs
