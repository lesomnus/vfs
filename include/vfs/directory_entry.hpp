#pragma once

#include <compare>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>

#include "vfs/fs.hpp"

namespace vfs {

class directory_entry {
   public:
	directory_entry() noexcept                  = default;
	directory_entry(directory_entry const&)     = default;
	directory_entry(directory_entry&&) noexcept = default;

	directory_entry(Fs const& fs) noexcept
	    : fs_(fs.shared_from_this()) { }

	explicit directory_entry(Fs const& fs, std::filesystem::path p)
	    : fs_(fs.shared_from_this())
	    , path_(std::move(p)) {
		this->refresh();
	}

	directory_entry(Fs const& fs, std::filesystem::path p, std::error_code& ec)
	    : fs_(fs.shared_from_this())
	    , path_(std::move(p)) {
		this->refresh(ec);
	}

	directory_entry& operator=(directory_entry const& other)     = default;
	directory_entry& operator=(directory_entry&& other) noexcept = default;

	/**
	 * @brief Assigns a new path and calls `refresh` to update the cached attributes.
	 * 
	 * @param[in] p New path to assign.
	 */
	void assign(std::filesystem::path const& p) {
		this->path_ = p;
		this->refresh();
	}

	/**
	 * @brief Assigns a new path and calls `refresh` to update the cached file attributes.
	 * 
	 * @param[in]  p  New path to assign.
	 * @param[out] ec Error code to store error status to.
	 */
	void assign(std::filesystem::path const& p, std::error_code& ec) {
		this->path_ = p;
		this->refresh(ec);
	}

	/**
	 * @brief Updates the cached file attributes.
	 * 
	 */
	void refresh() {
		this->type_ = this->symlink_status().type();
	}

	/**
	 * @brief Updates the cached file attributes.
	 * 
	 * @param[out] ec Error code to store error status to.
	 */
	void refresh(std::error_code& ec) noexcept {
		this->type_ = this->symlink_status(ec).type();
	}

	/**
	 * @brief Replaces the filename of the path and calls `refresh` to update the cached file attributes.
	 * 
	 * @param[in] p New filename to replace the existing filename with.
	 */
	void replace_filename(std::filesystem::path const& p) {
		this->path_.replace_filename(p);
		this->refresh();
	}

	/**
	 * @brief Replaces the filename of the associating path and calls `refresh` to update the cached file attributes.
	 * 
	 * @param[in]  p  New filename to replace the existing filename with.
	 * @param[out] ec Error code to store error status to.
	 */
	void replace_filename(std::filesystem::path const& p, std::error_code& ec) {
		this->path_.replace_filename(p);
		this->refresh(ec);
	}

	/**
	 * @brief Returns the associated path.
	 * 
	 * @return Associated path.
	 */
	[[nodiscard]] std::filesystem::path const& path() const noexcept {
		return this->path_;
	}

	/**
	 * @brief Returns the associated path.
	 * 
	 * @return Associated path.
	 */
	operator std::filesystem::path const&() const noexcept {
		return this->path_;
	}

	/**
	 * @brief Checks whether the associating path refers to an existing file system object.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool exists() const {
		return this->fs_->exists(this->path_);
	}

	/**
	 * @brief Checks whether the associating path refers to an existing file system object.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool exists(std::error_code& ec) const noexcept {
		return this->fs_->exists(this->path_, ec);
	}

	/**
	 * @brief Checks whether the associating path refers to a block file.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_block_file() const {
		return this->get_type_() == std::filesystem::file_type::block;
	}

	/**
	 * @brief Checks whether the associating path refers to a block file.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_block_file(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::block;
	}

	/**
	 * @brief Checks whether the associating path refers to a character file.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_character_file() const {
		return this->get_type_() == std::filesystem::file_type::character;
	}

	/**
	 * @brief Checks whether the associating path refers to a character file.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_character_file(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::character;
	}

	/**
	 * @brief Checks whether the associating path refers to a directory.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_directory() const {
		return this->get_type_() == std::filesystem::file_type::directory;
	}

	/**
	 * @brief Checks whether the associating path refers to a directory.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_directory(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::directory;
	}

	/**
	 * @brief Checks whether the associating path refers to a pipe file.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_fifo() const {
		return this->get_type_() == std::filesystem::file_type::fifo;
	}

	/**
	 * @brief Checks whether the associating path refers to a pipe file.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_fifo(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::fifo;
	}

	/**
	 * @brief Checks whether the associating path refers to an other file.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_other() const {
		return this->exists() && !this->is_regular_file() && !this->is_directory() && !this->is_symlink();
	}

	/**
	 * @brief Checks whether the associating path refers to an other file.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_other(std::error_code& ec) const noexcept {
		return this->exists(ec) && !this->is_regular_file(ec) && !this->is_directory(ec) && !this->is_symlink(ec);
	}

	/**
	 * @brief Checks whether the associating path refers to a regular file.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_regular_file() const {
		return this->get_type_() == std::filesystem::file_type::regular;
	}

	/**
	 * @brief Checks whether the associating path refers to a regular file.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_regular_file(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::regular;
	}

	/**
	 * @brief Checks whether the associating path refers to a socket.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_socket() const {
		return this->get_type_() == std::filesystem::file_type::socket;
	}

	/**
	 * @brief Checks whether the associating path refers to a socket.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_socket(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::socket;
	}

	/**
	 * @brief Checks whether the associating path refers to a symbolic lik.
	 * 
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	[[nodiscard]] bool is_symlink() const {
		return this->get_type_() == std::filesystem::file_type::symlink;
	}

	/**
	 * @brief Checks whether the associating path refers to a symbolic lik.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `true` if file or directory exists, `false` otherwise.
	 */
	bool is_symlink(std::error_code& ec) const noexcept {
		return this->get_type_(ec) == std::filesystem::file_type::symlink;
	}

	/**
	 * @brief Retrieves the size of a file referred to by associated path.
	 * 
	 * @return Size of the file in bytes.
	 */
	[[nodiscard]] std::uintmax_t file_size() const {
		return this->fs_->file_size(this->path_);
	}

	/**
	 * @brief Retrieves the size of a file referred to by associated path.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Size of the file in bytes.
	 */
	std::uintmax_t file_size(std::error_code& ec) const noexcept {
		return this->fs_->file_size(this->path_, ec);
	}

	/**
	 * @brief Retrieves the number of hard links to a file referred to by associated path.
	 * 
	 * @return Number of hard links to the file.
	 */
	[[nodiscard]] std::uintmax_t hard_link_count() const {
		return this->fs_->hard_link_count(this->path_);
	}

	/**
	 * @brief Retrieves the number of hard links to a file referred to by associated path.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Number of hard links to the file.
	 */
	std::uintmax_t hard_link_count(std::error_code& ec) const noexcept {
		return this->fs_->hard_link_count(this->path_, ec);
	}

	/**
	 * @brief Retrieves the last write time of a file referred to by associated path.
	 * 
	 * @return Last write time of the file.
	 */
	[[nodiscard]] std::filesystem::file_time_type last_write_time() const {
		return this->fs_->last_write_time(this->path_);
	}

	/**
	 * @brief Retrieves the last write time of a file referred to by associated path.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Last write time of the file.
	 */
	std::filesystem::file_time_type last_write_time(std::error_code& ec) const noexcept {
		return this->fs_->last_write_time(this->path_, ec);
	}

	/**
	 * @brief Retrieves the status of a file or directory referred to by associated path.
	 * 
	 * @return Status of the file.
	 */
	[[nodiscard]] std::filesystem::file_status status() const {
		return this->fs_->status(this->path_);
	}

	/**
	 * @brief Retrieves the status of a file or directory referred to by associated path.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Status of the file.
	 */
	std::filesystem::file_status status(std::error_code& ec) const noexcept {
		return this->fs_->status(this->path_);
	}

	/**
	 * @brief Retrieves the status of a file or directory referred to by associated path.
	 *   If the associating path is a symbolic link, it will not be followed and status of symbolic link is retrieved.
	 * 
	 * @return Status of the file or directory.
	 */
	[[nodiscard]] std::filesystem::file_status symlink_status() const {
		return this->fs_->symlink_status(this->path_);
	}

	/**
	 * @brief Retrieves the status of a file or directory referred to by associated path.
	 *   If the associating path is a symbolic link, it will not be followed and status of symbolic link is retrieved.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return Status of the file or directory.
	 */
	std::filesystem::file_status symlink_status(std::error_code& ec) const noexcept {
		return this->fs_->symlink_status(this->path_, ec);
	}

	/**
	 * @brief Compares the associating paths. 
	 * 
	 * @param rhs Directory entry to compare.
	 * @return `true` if the paths are equal, `false` otherwise. 
	 */
	bool operator==(directory_entry const& rhs) const noexcept {
		return this->path_ == rhs.path_;
	}

	/**
	 * @brief Compares the associating paths. 
	 * 
	 * @param rhs Directory entry to compare.
	 * @return Ordering of the paths.
	 */
	std::strong_ordering operator<=>(directory_entry const& rhs) const noexcept {
		return this->path_ <=> rhs.path_;
	}

   private:
	[[nodiscard]] std::filesystem::file_type get_type_() const {
		if(this->type_ != std::filesystem::file_type::none && this->type_ != std::filesystem::file_type::symlink) {
			return this->type_;
		}
		return this->status().type();
	}

	std::filesystem::file_type get_type_(std::error_code& ec) const {
		if(this->type_ != std::filesystem::file_type::none && this->type_ != std::filesystem::file_type::symlink) {
			ec.clear();
			return this->type_;
		}
		return this->status().type();
	}

	std::shared_ptr<Fs const>  fs_;
	std::filesystem::path      path_;
	std::filesystem::file_type type_ = std::filesystem::file_type::none;
};

}  // namespace vfs
