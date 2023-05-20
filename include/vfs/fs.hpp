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

	/**
	 * @brief Opens a file for reading.
	 * 
	 * @param filename Name of the file to be opened.
	 * @param mode     Open mode for the file.
	 * @return Input stream of the opened file.
	 */
	virtual std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const = 0;

	/**
	 * @brief Opens a file for writing.
	 * 
	 * @param filename Name of the file to be opened.
	 * @param mode     Open mode for the file.
	 * @return Output stream of the opened file.
	 */
	virtual std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) = 0;

	/**
	 * @brief Converts a given path to its absolute form, which is a fully qualified path that includes the root directory.
	 * 
	 * @param[in] p Path to be converted to its absolute form.
	 * @return Absolute (although not necessarily canonical) form of \p p.
	 */
	std::filesystem::path absolute(std::filesystem::path const& p) const {
		return p.is_absolute() ? p : this->current_path() / p;
	}

	/**
	 * @brief Converts a given path to its absolute form, which is a fully qualified path that includes the root directory.
	 * 
	 * @param[in]  p  Path to be converted to its absolute form.
	 * @param[out] ec Error code to store error status to.
	 * @return Absolute (although not necessarily canonical) form of \p p.
	 */
	std::filesystem::path absolute(std::filesystem::path const& p, std::error_code& ec) const {
		return p.is_absolute() ? p : this->current_path(ec) / p;
	}

	/**
	 * @brief Converts a given path to its canonical form, which is an absolute path with symbolic links resolved and redundant elements like ".", ".." removed.
	 *   The canonical form represents the unique physical file system path to a file or directory.
	 * 
	 * @param[in] p Path to be converted to its canonical form.
	 * @return Canonical form of \p p.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if \p p does not existing or insufficient permission.
	 */
	virtual std::filesystem::path canonical(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Converts a given path to its canonical form, which is an absolute path with symbolic links resolved and redundant elements like ".", ".." removed.
	 *   The canonical form represents the unique physical file system path to a file or directory.
	 * 
	 * @param[in]  p  Path to be converted to its canonical form.
	 * @param[out] ec Error code to store error status to.
	 * @return Canonical form of \p p.
	 */
	virtual std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Converts a given path to its weakly canonical form, which is an path with symbolic links resolved and redundant elements like ".", ".." removed.
	 *   The weakly canonical from may still contains symbolic links if they cannot be resolved.
	 * 
	 * @param[in] p Path to be converted to its weakly canonical form.
	 * @return Weakly canonical form of \p p.
	 */
	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Converts a given path to its weakly canonical form, which is an path with symbolic links resolved and redundant elements like ".", ".." removed.
	 *   The weakly canonical from may still contains symbolic links if they cannot be resolved.
	 * 
	 * @param[in]  p  Path to be converted to its weakly canonical form.
	 * @param[out] ec Error code to store error status to.
	 * @return Weakly canonical form of \p p.
	 */
	virtual std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Constructs a relative path from the current path to a target path.
	 * 
	 * @param[in]  p  Path to be converted from the current path to its relative form.
	 * @param[out] ec Error code to store error status to.
	 * @return Relative path from the current to \p p.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::error_code& ec) const {
		return this->relative(p, this->current_path(), ec);
	}

	/**
	 * @brief Constructs a relative path from a base path to a target path.
	 * 
	 * @param[in] p    Path to be converted from \p base to its relative form.
	 * @param[in] base Base path, against which \p p will be made relative.
	 * @return Relative path from \p base to \p p.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base) const {
		return this->weakly_canonical(p).lexically_relative(this->weakly_canonical(base));
	}

	/**
	 * @brief Constructs a relative path from a base path to a target path.
	 * 
	 * @param[in]  p    Path to be converted from \p base to its relative form.
	 * @param[in]  base Base path, against which \p p will be made relative.
	 * @param[out] ec   Error code to store error status to.
	 * @return Relative path from \p base to \p p.
	 */
	std::filesystem::path relative(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_relative(std::filesystem::weakly_canonical(base, ec));
	}

	/**
	 * @brief Constructs a proximate path from the current path to a target path.
	 * 
	 * @param[in]  p  Path to be converted from the current path to its proximate form.
	 * @param[out] ec Error code to store error status to.
	 * @return Proximate path from \p base to \p p.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->proximate(p, this->current_path(), ec);
	}

	/**
	 * @brief Constructs a proximate path from a base path to a target path.
	 * 
	 * @param[in] p    Path to be converted from \p base to its proximate form.
	 * @param[in] base Base path, against which \p p will be made proximate.
	 * @return Proximate path from \p base to \p p.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base = std::filesystem::current_path()) const {
		return std::filesystem::weakly_canonical(p).lexically_proximate(std::filesystem::weakly_canonical(base));
	}

	/**
	 * @brief Constructs a proximate path from a base path to a target path.
	 * 
	 * @param[in]  p    Path to be converted from \p base to its proximate form.
	 * @param[in]  base Base path, against which \p p will be made proximate.
	 * @param[out] ec   Error code to store error status to.
	 * @return Proximate path from \p base to \p p.
	 */
	std::filesystem::path proximate(std::filesystem::path const& p, std::filesystem::path const& base, std::error_code& ec) const {
		return std::filesystem::weakly_canonical(p, ec).lexically_proximate(std::filesystem::weakly_canonical(base, ec));
	}

	/**
	 * @brief Copies a file or directory from a source path to a destination path.
	 * 
	 * @param[in] src Path to the file or directory to be copied.
	 * @param[in] dst Path where \p src will be copied to.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p src does not exist.
	 */
	void copy(std::filesystem::path const& src, std::filesystem::path const& dst) {
		this->copy(src, dst, std::filesystem::copy_options::none);
	}

	/**
	 * @brief Copies a file or directory from a source path to a destination path.
	 * 
	 * @param[in]  src Path to the file or directory to be copied.
	 * @param[in]  dst Path where \p src will be copied to.
	 * @param[out] ec  Error code to store error status to.
	 */
	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) {
		this->copy(src, dst, std::filesystem::copy_options::none, ec);
	}

	/**
	 * @brief Copies a file or directory from a source path to a destination path.
	 * 
	 * @param[in] src  Path to the file or directory to be copied.
	 * @param[in] dst  Path where \p from will be copied to.
	 * @param[in] opts Copy options to control the behavior.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist.
	 */
	virtual void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) = 0;

	/**
	 * @brief Copies a file or directory from a source path to a destination path.
	 * 
	 * @param[in]  src  Path to the file or directory to be copied.
	 * @param[in]  dst  Path where \p from will be copied to.
	 * @param[in]  opts Copy options to control the behavior.
	 * @param[out] ec   Error code to store error status to.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist.
	 */
	virtual void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) = 0;

	/**
	 * @brief Copies a file from a source path to a destination path.
	 * 
	 * @param[in] src Path to the file or directory to be copied.
	 * @param[in] dst Path where \p src will be copied to.
	 * @return `true` if the file was copied, `false` otherwise.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist or `!Fs::is_regular_file(from)`.
	 */
	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst) {
		return this->copy_file(src, dst, std::filesystem::copy_options::none);
	}

	/**
	 * @brief Copies a file from a source path to a destination path.
	 * 
	 * @param[in]  src Path to the file or directory to be copied.
	 * @param[in]  dst Path where \p src will be copied to.
	 * @param[out] ec  Error code to store error status to.
	 * @return `true` if the file was copied, `false` otherwise.
	 */
	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) {
		return this->copy_file(src, dst, std::filesystem::copy_options::none, ec);
	}

	/**
	 * @brief Copies a file from a source path to a destination path.
	 * 
	 * @param[in] src  Path to the file to be copied.
	 * @param[in] dst  Path where \p src will be copied to.
	 * @param[in] opts Copy options to control the behavior.
	 * @return `true` if the file was copied, `false` otherwise.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p from does not exist or `!Fs::is_regular_file(from)`.
	 */
	virtual bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) = 0;

	/**
	 * @brief Copies a file from a source path to a destination path.
	 * 
	 * @param[in]  src  Path to the file to be copied.
	 * @param[in]  dst  Path where \p src will be copied to.
	 * @param[in]  opts Copy options to control the behavior.
	 * @param[out] ec   Error code to store error status to.
	 * @return `true` if the file was copied, `false` otherwise.
	 */
	virtual bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) = 0;

	/**
	 * @brief Copies a symbolic link.
	 * 
	 * @param[in] src Path to the symbolic link to be copied.
	 * @param[in] dst Path where \p src will be copied to.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if the path \p src does not exist or \p dst does exist.
	 */
	void copy_symlink(std::filesystem::path const& src, std::filesystem::path const& dst) {
		auto const target = this->read_symlink(src);
		this->is_directory(target)
		    ? this->create_directory_symlink(target, dst)
		    : this->create_symlink(target, dst);
	}

	/**
	 * @brief Copies a symbolic link.
	 * 
	 * @param[in]  src Path to the symbolic link to be copied.
	 * @param[in]  dst Path where \p src will be copied to.
	 * @param[out] ec  Error code to store error status to.
	 */
	void copy_symlink(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept {
		if(auto const target = this->read_symlink(src, ec); ec) {
			return;
		} else {
			this->is_directory(target)
			    ? this->create_directory_symlink(target, dst, ec)
			    : this->create_symlink(target, dst, ec);
		}
	}

	/**
	 * @brief Creates a directory.
	 * 
	 * @param[in] p Path to the directory to be created.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directory(std::filesystem::path const& p) = 0;

	/**
	 * @brief Creates a directory.
	 * 
	 * @param[in]  p  Path to the directory to be created.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Creates a directory.
	 * 
	 * @param[in] p    Path to the directory to be created.
	 * @param[in] attr Path to the directory to copy the attributes from.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) = 0;

	/**
	 * @brief Creates a directory.
	 * 
	 * @param[in]  p    Path to the directory to be created.
	 * @param[in]  attr Path to the directory to copy the attributes from.
	 * @param[out] ec   Error code to store error status to.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Creates directories and any missing parent directories.
	 * 
	 * @param[in] p  Path to the directory to be created.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directories(std::filesystem::path const& p) = 0;

	/**
	 * @brief Creates directories and any missing parent directories.
	 * 
	 * @param[in]  p  Path to the directory to be created.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if the directory was successfully created, `false` otherwise.
	 */
	virtual bool create_directories(std::filesystem::path const& p, std::error_code& ec) = 0;

	/**
	 * @brief Creates a hard link.
	 * 
	 * @param[in] target Path to the file or directory to link to.
	 * @param[in] link   Path to the hard link to be created.
	 */
	virtual void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) = 0;

	/**
	 * @brief Creates a hard link.
	 * 
	 * @param[in]  target Path to the file or directory to link to.
	 * @param[in]  link   Path to the hard link to be created.
	 * @param[out] ec     Error code to store error status to.
	 */
	virtual void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Creates a symbolic link.
	 * 
	 * @param[in] target Path to the file or directory to link to.
	 * @param[in] link   Path to the symbolic link to be created.
	 */
	virtual void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) = 0;

	/**
	 * @brief Creates a symbolic link.
	 * 
	 * @param[in]  target Path to the file or directory to link to.
	 * @param[in]  link   Path to the symbolic link to be created.
	 * @param[out] ec     Error code to store error status to.
	 */
	virtual void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Creates a symbolic link to a directory.
	 * 
	 * @param[in] target Path to the directory to link to.
	 * @param[in] link   Path to the symbolic link to be created.
	 */
	virtual void create_directory_symlink(std::filesystem::path const& target, std::filesystem::path const& link) {
		this->create_symlink(target, link);
	}

	/**
	 * @brief Creates a symbolic link to a directory
	 * 
	 * @param[in]  target Path to the directory to link to.
	 * @param[in]  link   Path to the symbolic link to be created.
	 * @param[out] ec     Error code to store error status to.
	 */
	virtual void create_directory_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept {
		this->create_symlink(target, link, ec);
	}

	/**
	 * @brief Retrieves path of the current working directory.
	 * 
	 * @return Path of the current working directory.
	 */
	virtual std::filesystem::path current_path() const = 0;

	/**
	 * @brief Retrieves path of the current working directory.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Path of the current working directory.
	 */
	virtual std::filesystem::path current_path(std::error_code& ec) const = 0;

	/**
	 * @brief Creates a new Fs that share the same file system but have different working directory.
	 * 
	 * @param[in] p Path to change the current working directory to.
	 * @return Same file system where the working directory is \p p.
	 */
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& = 0;

	/**
	 * @brief Sets the current working directory.
	 * 
	 * @param[in] p Path to change the current working directory to.
	 * @return Same file system where the working directory is \p p.
	 */
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && = 0;

	/**
	 * @brief Creates a new Fs that share the same file system but have different working directory.
	 * 
	 * @param[in]  p  Path to change the current working directory to.
	 * @param[out] ec Error code to store error status to.
	 * @return Same file system where the working directory is \p p.
	 */
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept = 0;

	/**
	 * @brief Sets the current working directory.
	 * 
	 * @param[in]  p  Path to change the current working directory to.
	 * @param[out] ec Error code to store error status to.
	 * @return Same file system where the working directory is \p p.
	 */
	virtual std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept = 0;

	/**
	 * @brief Checks whether the path refers to an existing file system object
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if file or directory on \p p exists, `false` otherwise.
	 */
	bool exists(std::filesystem::file_status s) const noexcept {
		return this->status_known(s) && s.type() != std::filesystem::file_type::not_found;
	}

	/**
	 * @brief Checks whether the path refers to an existing file system object
	 * 
	 * @param[in] p Path to the file or directory to check.
	 * @return `true` if file or directory on \p p exists, `false` otherwise.
	 */
	bool exists(std::filesystem::path const& p) const {
		return std::filesystem::exists(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to an existing file system object
	 * 
	 * @param[in]  p  Path to the file or directory to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory on \p p exists, `false` otherwise.
	 */
	bool exists(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		auto const s = this->status(p, ec);
		if(std::filesystem::status_known(s)) {
			ec.clear();
		}
		return std::filesystem::exists(s);
	}

	/**
	 * @brief Checks whether two paths refer to the same file system object.
	 * 
	 * @param[in] p1,p2 Paths to the file or directory to check for equivalence.
	 * @return `true` if \p p1 and \p p2 refer to the same file or directory, `false` otherwise.
	 * 
	 * @exception \ref std::filesystem::filesystem_error if both paths \p p1 and \p p2 do not exist.
	 */
	virtual bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const = 0;

	/**
	 * @brief Checks whether two paths refer to the same file system object.
	 * 
	 * @param[in]  p1,p2 Paths to the file or directory to check for equivalence.
	 * @param[out] ec    Error code to store error status to.
	 * @return `true` if \p p1 and \p p2 refer to the same file or directory, `false` otherwise.
	 */
	virtual bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the size of a file.
	 * 
	 * @param[in] p Path to the file for which the size is to be retrieved.
	 * @return Size of the file in bytes.
	 */
	virtual std::uintmax_t file_size(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the size of a file.
	 * 
	 * @param[in]  p  Path to the file for which the size is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Size of the file in bytes.
	 */
	virtual std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the number of hard links to a file.
	 * 
	 * @param[in] p Path to the file for which the number of hard links is to be retrieved.
	 * @return Number of hard links to the file.
	 */
	virtual std::uintmax_t hard_link_count(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the number of hard links to a file.
	 * 
	 * @param[in]  p  Path to the file for which the number of hard links is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Number of hard links to the file.
	 */
	virtual std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the last write time of a file.
	 * 
	 * @param[in] p Path to the file for which the last write time is to be retrieved.
	 * @return Last write time of the file.
	 */
	virtual std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the last write time of a file.
	 * 
	 * @param[in]  p  Path to the file for which the last write time is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Last write time of the file.
	 */
	virtual std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Sets the last write time of a file.
	 * 
	 * @param[in] p Path to the file for which the last write time is to be set.
	 * @param[in] t New last write time of the file.
	 */
	virtual void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) = 0;

	/**
	 * @brief Sets the last write time of a file.
	 * 
	 * @param[in]  p  Path to the file for which the last write time is to be set.
	 * @param[in]  t  New last write time of the file.
	 * @param[out] ec Error code to store error status to.
	 */
	virtual void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Modifies the permissions of a file or directory.
	 * 
	 * @param[in] p    Path to the file or directory for which the permissions are to be set.
	 * @param[in] prms Desired permissions to be set, added, or removed.
	 * @param[in] opts Options to control the behavior.
	 */
	virtual void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) = 0;

	/**
	 * @brief Modifies the permissions of a file or directory.
	 * 
	 * @param[in]  p    Path to the file or directory for which the permissions are to be set.
	 * @param[in]  prms Desired permissions to be set, added, or removed.
	 * @param[in]  opts Options to control the behavior.
	 * @param[out] ec   Error code to store error status to.
	 */
	virtual void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) = 0;

	/**
	 * @brief Modifies the permissions of a file or directory.
	 * 
	 * @param[in]  p    Path to the file or directory for which the permissions are to be set.
	 * @param[in]  prms Desired permissions to be set, added, or removed.
	 * @param[out] ec   Error code to store error status to.
	 */
	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::error_code& ec) noexcept {
		this->permissions(p, prms, std::filesystem::perm_options::replace, ec);
	}

	/**
	 * @brief Retrieves the target of a symbolic link.
	 * 
	 * @param[in] p Path to the symbolic link for which the target is to be retrieved.
	 * @return Target of the symbolic link.
	 */
	virtual std::filesystem::path read_symlink(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the target of a symbolic link.
	 * 
	 * @param[in]  p  Path to the symbolic link for which the target is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Target of the symbolic link.
	 */
	virtual std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Removes a file or an empty directory.
	 * 
	 * @param[in] p Path to the file or directory to be removed.
	 * @return `true` if the file or directory was removed, `false` otherwise.
	 */
	virtual bool remove(std::filesystem::path const& p) = 0;

	/**
	 * @brief Removes a file or an empty directory.
	 * 
	 * @param[in]  p  Path to the file or directory to be removed.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if the file or directory was removed, `false` otherwise.
	 */
	virtual bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Removes a file or directory and its contents recursively.
	 * 
	 * @param[in] p Path to the file or directory to be removed.
	 * @return Number of files and directories that were removed. 
	 */
	virtual std::uintmax_t remove_all(std::filesystem::path const& p) = 0;

	/**
	 * @brief Removes a file or directory and its contents recursively.
	 * 
	 * @param[in]  p  Path to the file or directory to be removed.
	 * @param[out] ec Error code to store error status to.
	 * @return Number of files and directories that were removed. 
	 */
	virtual std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) = 0;

	/**
	 * @brief Renames a file or directory.
	 * 
	 * @param[in] src Path to the file or directory to be renamed.
	 * @param[in] dst New path for the file or directory.
	 */
	virtual void rename(std::filesystem::path const& src, std::filesystem::path const& dst) = 0;

	/**
	 * @brief Renames a file or directory.
	 * 
	 * @param[in]  src Path to the file or directory to be renamed.
	 * @param[in]  dst New path for the file or directory.
	 * @param[out] ec  Error code to store error status to.
	 */
	virtual void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Resizes a regular file by truncation or zero-fill.
	 * 
	 * @param[in] p Path to the regular file to be resized.
	 * @param[in] n New size of the file in bytes.
	 */
	virtual void resize_file(std::filesystem::path const& p, std::uintmax_t n) = 0;

	/**
	 * @brief Resizes a regular file by truncation or zero-fill.
	 * 
	 * @param[in]  p  Path to the regular file to be resized.
	 * @param[in]  n  New size of the file in bytes.
	 * @param[out] ec Error code to store error status to.
	 */
	virtual void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept = 0;

	/**
	 * @brief Retrieves space information about a file system.
	 * 
	 * @param[in] p Path to a file or directory for which the space information to be retrieved.
	 * @return Space information of \p p.
	 */
	virtual std::filesystem::space_info space(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves space information about a file system.
	 * 
	 * @param[in]  p  Path to a file or directory for which the space information to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Space information of \p p.
	 */
	virtual std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the status of a file or directory.
	 * 
	 * @param[in] p Path to the file or directory for which the status is to be retrieved.
	 * @return Status of \p p.
	 */
	virtual std::filesystem::file_status status(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the status of a file or directory.
	 * 
	 * @param[in]  p  Path to the file or directory for which the status is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Status of \p p.
	 */
	virtual std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the status of a file or directory. If the given path is a symbolic link, it will not be followed and status of symbolic link is retrieved.
	 * 
	 * @param[in] p Path to the file or directory for which the status is to be retrieved.
	 * @return Status of \p p.
	 */
	virtual std::filesystem::file_status symlink_status(std::filesystem::path const& p) const = 0;

	/**
	 * @brief Retrieves the status of a file or directory. If the given path is a symbolic link, it will not be followed and status of symbolic link is retrieved.
	 * 
	 * @param[in]  p  Path to the file or directory for which the status is to be retrieved.
	 * @param[out] ec Error code to store error status to.
	 * @return Status of \p p.
	 */
	virtual std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept = 0;

	/**
	 * @brief Retrieves the path to the temporary directory.
	 * 
	 * @return Path to the temporary directory.
	 */
	virtual std::filesystem::path temp_directory_path() const = 0;

	/**
	 * @brief Retrieves the path to the temporary directory.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Path to the temporary directory.
	 */
	virtual std::filesystem::path temp_directory_path(std::error_code& ec) const = 0;

	/**
	 * @brief Checks whether the path refers to a block file.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a block file, `false` otherwise.
	 */
	bool is_block_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::block;
	}

	/**
	 * @brief Checks whether the path refers to a block file.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a block file, `false` otherwise.
	 */
	bool is_block_file(std::filesystem::path const& p) const {
		return this->is_block_file(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a block file.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a block file, `false` otherwise.
	 */
	bool is_block_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_block_file(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to a character file.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a character file, `false` otherwise.
	 */
	bool is_character_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::character;
	}

	/**
	 * @brief Checks whether the path refers to a character file.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a character file, `false` otherwise.
	 */
	bool is_character_file(std::filesystem::path const& p) const {
		return this->is_character_file(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a character file.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a character file, `false` otherwise.
	 */
	bool is_character_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_character_file(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to a directory.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a directory, `false` otherwise.
	 */
	bool is_directory(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::directory;
	}

	/**
	 * @brief Checks whether the path refers to a directory.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a directory, `false` otherwise.
	 */
	bool is_directory(std::filesystem::path const& p) const {
		return this->is_directory(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a directory.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a directory, `false` otherwise.
	 */
	bool is_directory(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_directory(this->status(p, ec));
	}

	virtual bool is_empty(std::filesystem::path const& p) const                      = 0;
	virtual bool is_empty(std::filesystem::path const& p, std::error_code& ec) const = 0;

	/**
	 * @brief Checks whether the path refers to a pipe file.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a pipe file, `false` otherwise.
	 */
	bool is_fifo(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::fifo;
	}

	/**
	 * @brief Checks whether the path refers to a pipe file.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a pipe file, `false` otherwise.
	 */
	bool is_fifo(std::filesystem::path const& p) const {
		return this->is_fifo(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a pipe file.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a pipe file, `false` otherwise.
	 */
	bool is_fifo(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_fifo(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to an other file.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to an other file, `false` otherwise.
	 */
	bool is_other(std::filesystem::file_status s) const noexcept {
		return this->exists(s) && !this->is_regular_file(s) && !this->is_directory(s) && !this->is_symlink(s);
	}

	/**
	 * @brief Checks whether the path refers to an other file.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to an other file, `false` otherwise.
	 */
	bool is_other(std::filesystem::path const& p) const {
		return this->is_other(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to an other file.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to an other file, `false` otherwise.
	 */
	bool is_other(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_other(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to a regular file.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a regular file, `false` otherwise.
	 */
	bool is_regular_file(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::regular;
	}

	/**
	 * @brief Checks whether the path refers to a regular file.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a regular file, `false` otherwise.
	 */
	bool is_regular_file(std::filesystem::path const& p) const {
		return this->is_regular_file(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a regular file.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a regular file, `false` otherwise.
	 */
	bool is_regular_file(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_regular_file(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to a socket.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a socket, `false` otherwise.
	 */
	bool is_socket(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::socket;
	}

	/**
	 * @brief Checks whether the path refers to a socket.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a socket, `false` otherwise.
	 */
	bool is_socket(std::filesystem::path const& p) const {
		return this->is_socket(this->status(p));
	}

	/**
	 * @brief Checks whether the path refers to a socket.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a socket, `false` otherwise.
	 */
	bool is_socket(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_socket(this->status(p, ec));
	}

	/**
	 * @brief Checks whether the path refers to a symbolic link.
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p p refers to a symbolic link, `false` otherwise.
	 */
	bool is_symlink(std::filesystem::file_status s) const noexcept {
		return s.type() == std::filesystem::file_type::symlink;
	}

	/**
	 * @brief Checks whether the path refers to a symbolic link.
	 * 
	 * @param[in] p Path to the file to check.
	 * @return `true` if \p p refers to a symbolic link, `false` otherwise.
	 */
	bool is_symlink(std::filesystem::path const& p) const {
		return this->is_symlink(this->symlink_status(p));
	}

	/**
	 * @brief Checks whether the path refers to a symbolic link.
	 * 
	 * @param[in]  p  Path to the file to check.
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if \p p refers to a symbolic link, `false` otherwise.
	 */
	bool is_symlink(std::filesystem::path const& p, std::error_code& ec) const noexcept {
		return this->is_symlink(this->symlink_status(p, ec));
	}

	/**
	 * @brief Checks whether the file status is known
	 * 
	 * @param[in] s File status to check.
	 * @return `true` if \p s holds known file type, `false` otherwise.
	 */
	bool status_known(std::filesystem::file_status s) const noexcept {
		return s.type() != std::filesystem::file_type::none;
	}
};

/**
 * @brief Makes `Fs` that represents the OS-provided file system, which is equivalent to `std::filesystem`.
 * 
 * @return New `Fs` that represents the OS-provided file system.
 */
std::shared_ptr<Fs> make_os_fs();

/**
 * @brief Makes empty `Fs` that is virtual. Regular files are written to the temporary directory of the OS and are deleted when the `Fs` is destructed.
 * 
 * @param temp_dir Path to the temporary directory in the created Fs.
 * @return New empty `Fs` that is virtual.
 */
std::shared_ptr<Fs> make_vfs(std::filesystem::path const& temp_dir = "/tmp");

}  // namespace vfs
