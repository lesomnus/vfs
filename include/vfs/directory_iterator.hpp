#pragma once

#include <cstddef>
#include <filesystem>
#include <iterator>
#include <memory>
#include <system_error>

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {

class directory_iterator {
   public:
	using value_type        = vfs::directory_entry;
	using difference_type   = std::ptrdiff_t;
	using pointer           = vfs::directory_entry const*;
	using reference         = vfs::directory_entry const&;
	using iterator_category = std::input_iterator_tag;

	directory_iterator() noexcept = default;

	explicit directory_iterator(Fs const& fs, std::filesystem::path const& p)
	    : directory_iterator(fs, p, std::filesystem::directory_options::none) { }

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
	    : cursor_(fs.cursor_(p, opts)) {
		if(this->cursor_->at_end()) {
			this->cursor_.reset();
		}
	}

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::error_code& ec)
	    : directory_iterator(fs, p, std::filesystem::directory_options::none, ec) { }

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec)
	    : cursor_(fs.cursor_(p, opts, ec)) { }

	directory_iterator(directory_iterator const&) = default;
	directory_iterator(directory_iterator&&)      = default;

	directory_iterator& operator=(directory_iterator const&) = default;
	directory_iterator& operator=(directory_iterator&&)      = default;

	directory_entry const& operator*() const {
		return this->cursor_->value();
	}

	directory_entry const* operator->() const {
		return &this->cursor_->value();
	}

	directory_iterator& operator++() {
		this->cursor_->increment();
		if(this->cursor_->at_end()) {
			this->cursor_.reset();
		}
		return *this;
	}

	/**
	 * @brief Advances the directory iterator to the next entry.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `*this`.
	 */
	directory_iterator& increment(std::error_code& ec);

	bool operator==(directory_iterator const& rhs) const noexcept = default;

   private:
	std::shared_ptr<Fs::Cursor> cursor_;
};

directory_iterator begin(directory_iterator iter) noexcept {
	return iter;
}

directory_iterator end(directory_iterator /*unused*/) noexcept {
	return {};
}

class recursive_directory_iterator {
   public:
	recursive_directory_iterator() noexcept = default;

	explicit recursive_directory_iterator(Fs const& fs, std::filesystem::path const& p)
	    : recursive_directory_iterator(fs, p, std::filesystem::directory_options::none) { }

	recursive_directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
	    : cursor_(fs.recursive_cursor_(p, opts)) { }

	recursive_directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec)
	    : cursor_(fs.recursive_cursor_(p, opts, ec)) { }

	recursive_directory_iterator(Fs const& fs, std::filesystem::path const& p, std::error_code& ec)
	    : recursive_directory_iterator(fs, p, std::filesystem::directory_options::none, ec) { }

	recursive_directory_iterator(recursive_directory_iterator const& rhs)     = default;
	recursive_directory_iterator(recursive_directory_iterator&& rhs) noexcept = default;

	recursive_directory_iterator& operator=(recursive_directory_iterator const& other) = default;
	recursive_directory_iterator& operator=(recursive_directory_iterator&& other)      = default;

	directory_entry const& operator*() const {
		return this->cursor_->value();
	}

	directory_entry const* operator->() const {
		return &this->cursor_->value();
	}

	/**
	 * @brief Retrieves the holding directory options.
	 * 
	 * @return Directory options.
	 */
	std::filesystem::directory_options options();

	/**
	 * @brief Retrieves the current depth level of the recursive directory iterator.
	 * 
	 * @return Current depth level.
	 */
	[[nodiscard]] int depth() const {
		return this->cursor_->depth();
	}

	/**
	 * @brief Checks whether the recursion is pending.
	 * 
	 * @return `true` if the next increment will iterate into the currently referred directory, `false` otherwise.
	 */
	[[nodiscard]] bool recursion_pending() const {
		return this->cursor_->recursion_pending();
	}

	recursive_directory_iterator& operator++() {
		this->cursor_->increment();
		if(this->cursor_->at_end()) {
			this->cursor_.reset();
		}
		return *this;
	}

	/**
	 * @brief Advances the directory iterator to the next entry.
	 * 
	 * @param[out] ec Error code to store error status to.
	 * @return `*this`.
	 */
	recursive_directory_iterator& increment(std::error_code& ec);

	/**
	 * @brief Moves the iterator one level up in the directory hierarchy.
	 * 
	 */
	void pop() {
		this->cursor_->pop();
		if(this->cursor_->at_end()) {
			this->cursor_.reset();
		}
	}

	/**
	 * @brief Moves the iterator one level up in the directory hierarchy.
	 * 
	 * @param[out] ec Error code to store error status to.
	 */
	void pop(std::error_code& ec);

	/**
	 * @brief Disables recursion until the next increment.
	 * 
	 */
	void disable_recursion_pending() {
		this->cursor_->disable_recursion_pending();
	}

	bool operator==(recursive_directory_iterator const& rhs) const noexcept = default;

   private:
	std::shared_ptr<Fs::RecursiveCursor> cursor_;
};

recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept {
	return iter;
}

recursive_directory_iterator end(recursive_directory_iterator /*unused*/) noexcept {
	return {};
}

}  // namespace vfs
