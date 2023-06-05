#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>

namespace vfs {
namespace impl {

class Entry;
class DirectoryEntry;

class File {
   public:
	virtual ~File() = default;

	virtual std::filesystem::file_type type() const = 0;

	virtual std::filesystem::space_info space() const {
		return std::filesystem::space_info{
		    .capacity  = static_cast<std::uintmax_t>(-1),
		    .free      = static_cast<std::uintmax_t>(-1),
		    .available = static_cast<std::uintmax_t>(-1),
		};
	}

	virtual std::intmax_t owner() const = 0;

	virtual void owner(std::intmax_t owner) = 0;

	virtual std::intmax_t group() const = 0;

	virtual void group(std::intmax_t group) = 0;

	virtual std::filesystem::perms perms() const = 0;

	virtual void perms(std::filesystem::perms perms, std::filesystem::perm_options opts) = 0;
};

class RegularFile: virtual public File {
   public:
	static constexpr auto DefaultPerms = std::filesystem::perms::owner_write
	    | std::filesystem::perms::owner_read
	    | std::filesystem::perms::group_read
	    | std::filesystem::perms::others_read;

	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::regular;
	}

	RegularFile& operator=(RegularFile const& other);

	virtual std::uintmax_t size() const {
		return static_cast<std::uintmax_t>(-1);
	}

	virtual std::filesystem::file_time_type last_write_time() const {
		return std::filesystem::file_time_type::min();
	}

	virtual void last_write_time(std::filesystem::file_time_type new_time) = 0;

	virtual void resize(std::uintmax_t new_size) = 0;

	virtual std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode = std::ios_base::in) const = 0;
	virtual std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode = std::ios_base::out)     = 0;
};

class TempFile: public RegularFile {
   public:
	static std::filesystem::path temp_directory() {
		return std::filesystem::temp_directory_path() / "vfs";
	}

	TempFile();

	TempFile(TempFile const& other) = delete;
	TempFile(TempFile&& other)      = default;

	~TempFile();

	std::uintmax_t size() const override {
		return std::filesystem::file_size(this->path_);
	}

	std::filesystem::file_time_type last_write_time() const {
		return std::filesystem::last_write_time(this->path_);
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		std::filesystem::last_write_time(this->path_, new_time);
	}

	void resize(std::uintmax_t new_size) override {
		std::filesystem::resize_file(this->path_, new_size);
	}

	std::filesystem::space_info space() const {
		return std::filesystem::space(this->path_);
	}

	std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode = std::ios_base::out) override;

   private:
	std::filesystem::path path_;
};

class Directory: virtual public File {
   public:
	static constexpr auto DefaultPerms = std::filesystem::perms::all
	    & ~std::filesystem::perms::group_write
	    & ~std::filesystem::perms::others_write;

	class Cursor {
	   public:
		virtual ~Cursor() = default;

		virtual std::string const& name() const = 0;

		virtual std::shared_ptr<File> const& file() const = 0;

		virtual void increment() = 0;

		virtual bool at_end() const = 0;
	};

	class NilCursor: public Cursor {
	   public:
		NilCursor() = default;

		std::string const& name() const override {
			throw std::runtime_error("invalid cursor");
		}

		std::shared_ptr<File> const& file() const override {
			throw std::runtime_error("invalid cursor");
		}

		void increment() override {
			return;
		}

		bool at_end() const override {
			return true;
		}
	};

	struct Iterator {
		Iterator() { }

		Iterator(std::shared_ptr<Cursor> cursor)
		    : cursor_(std::move(cursor)) {
			if(this->cursor_->at_end()) {
				return;
			}

			this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
		}

		Iterator& operator=(Iterator const&) = default;
		Iterator& operator=(Iterator&&)      = default;

		std::pair<std::string, std::shared_ptr<File>> const& operator*() {
			return this->value_;
		}

		std::pair<std::string, std::shared_ptr<File>> const* operator->() {
			return &this->value_;
		}

		Iterator& operator++() {
			if(!this->cursor_) {
				return *this;
			}

			this->cursor_->increment();
			if(this->cursor_->at_end()) {
				this->cursor_.reset();
			} else {
				this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
			}

			return *this;
		}

		bool operator==(Iterator const& rhs) const noexcept {
			return this->cursor_ == rhs.cursor_;
		}

		std::strong_ordering operator<=>(Iterator const& rhs) const noexcept {
			return this->cursor_ <=> rhs.cursor_;
		}

	   private:
		std::shared_ptr<Cursor>                       cursor_;
		std::pair<std::string, std::shared_ptr<File>> value_;
	};

	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::directory;
	}

	virtual bool empty() const = 0;

	virtual bool contains(std::string const& name) const = 0;

	// returns nullptr if not exists.
	virtual std::shared_ptr<File> next(std::string const& name) const = 0;

	virtual std::shared_ptr<Entry> next_entry(std::string const& name, std::shared_ptr<DirectoryEntry> const& prev) const = 0;

	virtual bool insert(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual bool insert_or_assign(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual std::uintmax_t erase(std::string const& name) = 0;

	virtual std::shared_ptr<Cursor> cursor() const = 0;

	Iterator begin() const {
		if(this->empty()) {
			return Iterator();
		} else {
			return Iterator(this->cursor());
		}
	}

	Iterator end() const {
		return Iterator();
	}
};

class Symlink: virtual public File {
   public:
	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::symlink;
	}

	virtual std::filesystem::path target() const = 0;
};

}  // namespace impl
}  // namespace vfs
