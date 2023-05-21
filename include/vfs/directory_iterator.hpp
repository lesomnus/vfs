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

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options options)
	    : cursor_(fs.iterate_directory_(p, options)) {
		if(this->cursor_->is_end()) {
			this->cursor_.reset();
		}
	}

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::error_code& ec)
	    : directory_iterator(fs, p, std::filesystem::directory_options::none, ec) { }

	directory_iterator(Fs const& fs, std::filesystem::path const& p, std::filesystem::directory_options options, std::error_code& ec)
	    : cursor_(fs.iterate_directory_(p, options, ec)) { }

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
		if(this->cursor_->increment().is_end()) {
			this->cursor_.reset();
		}
		return *this;
	}

	directory_iterator& increment(std::error_code& ec) {
		if(this->cursor_->increment(ec).is_end()) {
			this->cursor_.reset();
		}
		return *this;
	}

	bool operator==(directory_iterator const& rhs) const noexcept = default;

   private:
	std::shared_ptr<Fs::Cursor> cursor_;
};

directory_iterator begin(directory_iterator iter) noexcept {
	return iter;
}

directory_iterator end(directory_iterator) noexcept {
	return directory_iterator();
}

}  // namespace vfs
