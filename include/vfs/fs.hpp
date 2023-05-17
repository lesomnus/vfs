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

	/**
	 * @brief Composes an absolute path.
	 * @details Returns a path referencing the same file system location as \p p, for which \ref Fs::is_absolute(p) is `true`.
	 * 
	 * @param[in] p Path to convert to absolute form.
	 * @return Absolute (although not necessarily canonical) pathname referencing the same file as \p p.
	 */
	std::filesystem::path absolute(std::filesystem::path const& p) const {
		return p.is_absolute() ? p : this->current_path() / p;
	}

	/**
	 * @brief Composes an absolute path.
	 * @details Returns a path referencing the same file system location as \p p, for which \ref Fs::is_absolute(p) is `true`.
	 * 
	 * @param[in]  p  Path to convert to absolute form.
	 * @param[out] ec Error code to store error status to.
	 * @return Absolute (although not necessarily canonical) pathname referencing the same file as \p p.
	 */
	std::filesystem::path absolute(std::filesystem::path const& p, std::error_code& ec) const {
		return p.is_absolute() ? p : this->current_path(ec) / p;
	}

	/**
	 * @brief Compose a canonical path.
	 * @details Converts path \p p to a canonical absolute path, i.e. an absolute path that has no dot, dot-dot elements or symbolic links in its generic format representation.
	 * 
	 * @param[in] p Path which may be absolute or relative; it must be an existing path.
	 * @return Canonical absolute path that resolves to the same file as \ref Fs::absolute(p).
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p p does not exist.
	 */
	virtual std::filesystem::path canonical(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Compose a canonical path.
	 * @details Converts path \p p to a canonical absolute path, i.e. an absolute path that has no dot, dot-dot elements or symbolic links in its generic format representation.
	 * 
	 * @param[in]  p  Path which may be absolute or relative; it must be an existing path.
	 * @param[out] ec Error code to store error status to.
	 * @return Canonical absolute path that resolves to the same file as \ref std::filesystem::absolute(p).
	 */
	virtual std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Compose a canonical path.
	 * 
	 * @param[in] p Path which may be absolute or relative.
	 * @return Normal path of the form `canonical(x)/y`, where `x` is a path composed of the longest leading sequence of elements in \p p that exist, and `y` is a path composed of the remaining trailing non-existent elements of \p p.
	 */
	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Compose a canonical path.
	 * 
	 * @param[in]  p  Path which may be absolute or relative.
	 * @param[out] ec Error code to store error status to.
	 * @return Normal path of the form `canonical(x)/y`, where `x` is a path composed of the longest leading sequence of elements in \p p that exist, and `y` is a path composed of the remaining trailing non-existent elements of \p p.
	 */
	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Composes a relative path.
	 * 
	 * @param[in]  p  Existing path.
	 * @param[out] ec Error code to store error status to.
	 * @return \p p made relative against current path.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::error_code& ec) const {
		return this->relative(p, this->current_path(), ec);
	}

	/**
	 * @brief Composes a relative path.
	 * 
	 * @param[in] p    Existing path
	 * @param[in] base Base path, against which \p p will be made relative.
	 * @return \p p made relative against \p base.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base) const {
		return this->weakly_canonical(p).lexically_relative(this->weakly_canonical(base));
	}

	/**
	 * @brief Composes a relative path.
	 * 
	 * @param[in]  p    Existing path
	 * @param[in]  base Base path, against which \p p will be made relative.
	 * @param[out] ec   Error code to store error status to.
	 * @return \p p made relative against \p base.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_relative(std::filesystem::weakly_canonical(base, ec));
	}

	/**
	 * @brief Composes a proximate path.
	 * 
	 * @param[in] p Existing path.
	 * @return \p p made proximate against current path.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p) const {
		return this->proximate(p, this->current_path());
	}

	/**
	 * @brief Composes a proximate path.
	 * 
	 * @param[in]  p  Existing path.
	 * @param[out] ec Error code to store error status to.
	 * @return \p p made proximate against current path.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->proximate(p, this->current_path(), ec);
	}

	/**
	 * @brief Composes a proximate path.
	 * 
	 * @param[in] p    Existing path
	 * @param[in] base Base path, against which \p p will be made proximate.
	 * @return \p p made proximate against \p base.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base = std::filesystem::current_path()) const {
		return std::filesystem::weakly_canonical(p).lexically_proximate(std::filesystem::weakly_canonical(base));
	}

	/**
	 * @brief Composes a proximate path.
	 * 
	 * @param[in]  p    Existing path
	 * @param[in]  base Base path, against which \p p will be made proximate.
	 * @param[out] ec   Error code to store error status to.
	 * @return \p p made proximate against \p base.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_proximate(std::filesystem::weakly_canonical(base, ec));
	}

	/**
	 * @brief Copies files or directories.
	 * 
	 * @param[in] from Path to the source file, directory, or symlink to copy; it must be an existing path.
	 * @param[in] to   Path to the target file, directory, or symlink.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist.
	 */
	void copy(std::filesystem::path const& from, std::filesystem::path const& to) {
		this->copy(from, to, std::filesystem::copy_options::none);
	}

	/**
	 * @brief Copies files or directories.
	 * 
	 * @param[in]  from Path to the source file, directory, or symlink to copy; it must be an existing path.
	 * @param[in]  to   Path to the target file, directory, or symlink.
	 * @param[out] ec   Error code to store error status to.
	 */
	void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) {
		this->copy(from, to, std::filesystem::copy_options::none, ec);
	}

	/**
	 * @brief Copies files or directories.
	 * 
	 * @param[in] from    Path to the source file, directory, or symlink to copy; it must be an existing path.
	 * @param[in] to      Path to the target file, directory, or symlink.
	 * @param[in] options Copy options.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist.
	 */
	virtual void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) = 0;

	/**
	 * @brief Copies files or directories.
	 * 
	 * @param[in]  from    Path to the source file, directory, or symlink to copy; it must be an existing path.
	 * @param[in]  to      Path to the target file, directory, or symlink.
	 * @param[in]  options Copy options.
	 * @param[out] ec      Error code to store error status to.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist.
	 */
	virtual void copy(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) = 0;

	/**
	 * @brief Copies file contents.
	 * 
	 * @param[in] from Path to the source file to copy; it must be an existing path.
	 * @param[in] to   Path to the target file.
	 * @return `true` if the file was copied, `false` otherwise.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist or `!Fs::is_regular_file(from)`.
	 */
	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to) {
		return this->copy_file(from, to, std::filesystem::copy_options::none);
	}

	/**
	 * @brief Copies file contents.
	 * 
	 * @param[in]  from Path to the source file to copy; it must be an existing path.
	 * @param[in]  to   Path to the target file.
	 * @param[out] ec   Error code to store error status to.
	 * @return `true` if the file was copied, `false` otherwise.
	 */
	bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) {
		return this->copy_file(from, to, std::filesystem::copy_options::none, ec);
	}

	/**
	 * @brief Copies file contents.
	 * 
	 * @param[in] from    Path to the source file to copy; it must be an existing path.
	 * @param[in] to      Path to the target file.
	 * @param[in] options Copy options.
	 * @return `true` if the file was copied, `false` otherwise.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist or `!Fs::is_regular_file(from)`.
	 */
	virtual bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) = 0;

	/**
	 * @brief Copies file contents.
	 * 
	 * @param[in]  from    Path to the source file to copy; it must be an existing path.
	 * @param[in]  to      Path to the target file.
	 * @param[in]  options Copy options.
	 * @param[out] ec      Error code to store error status to.
	 * @return `true` if the file was copied, `false` otherwise.
	 */
	virtual bool copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) = 0;

	/**
	 * @brief Copies a symbolic link.
	 * 
	 * @param[in] from Path to symbolic link to copy; it must be an existing path.
	 * @param[in] to   Destination path of the new symbolic link; it must be an non-existing path.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist or \p to does exist.
	 */
	void copy_symlink(std::filesystem::path const& from, std::filesystem::path const& to) {
		auto const target = this->read_symlink(from);
		this->is_directory(target)
		    ? this->create_directory_symlink(target, to)
		    : this->create_symlink(target, to);
	}

	/**
	 * @brief Copies a symbolic link.
	 * 
	 * @param[in]  from Path to symbolic link to copy; it must be an existing path.
	 * @param[in]  to   Destination path of the new symbolic link; it must be an non-existing path.
	 * @param[out] ec   Error code to store error status to.
	 */
	void copy_symlink(std::filesystem::path const& from, std::filesystem::path const& to, std::error_code& ec) noexcept {
		if(auto const target = this->read_symlink(from, ec); ec) {
			return;
		} else {
			this->is_directory(target)
			    ? this->create_directory_symlink(target, to, ec)
			    : this->create_symlink(target, to, ec);
		}
	}

	/**
	 * @brief Creates a directory.
	 * 
	 * @param p 
	 * @return true 
	 * @return false 
	 */
	virtual bool create_directory(std::filesystem::path const& p)                               = 0;
	virtual bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept = 0;

	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p)                               = 0;
	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& existing_p, std::error_code& ec) noexcept = 0;

	virtual bool create_directories(std::filesystem::path const& p)                      = 0;
	virtual bool create_directories(std::filesystem::path const& p, std::error_code& ec) = 0;

	virtual void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link)                               = 0;
	virtual void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept = 0;

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

	virtual std::uintmax_t file_size(std::filesystem::path const& p) const                               = 0;
	virtual std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	virtual std::uintmax_t hard_link_count(std::filesystem::path const& p) const                               = 0;
	virtual std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	virtual std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const                               = 0;
	virtual std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	virtual void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type new_time)                               = 0;
	virtual void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type new_time, std::error_code& ec) noexcept = 0;

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

	virtual void rename(std::filesystem::path const& old_p, std::filesystem::path const& new_p)                               = 0;
	virtual void rename(std::filesystem::path const& old_p, std::filesystem::path const& new_p, std::error_code& ec) noexcept = 0;

	virtual void resize_file(std::filesystem::path const& p, std::uintmax_t new_size)                               = 0;
	virtual void resize_file(std::filesystem::path const& p, std::uintmax_t new_size, std::error_code& ec) noexcept = 0;

	virtual std::filesystem::space_info space(std::filesystem::path const& p) const                               = 0;
	virtual std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

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

std::shared_ptr<Fs> make_sys_fs();
std::shared_ptr<Fs> make_vfs(std::filesystem::path const& temp_dir = "/tmp");

}  // namespace vfs
