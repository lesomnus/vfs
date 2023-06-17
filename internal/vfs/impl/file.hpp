#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>

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

	virtual void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) = 0;

	void perms(std::filesystem::perms prms) {
		this->perms(prms, std::filesystem::perm_options::replace);
	}

	virtual bool operator==(File const& other) const = 0;
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

	virtual RegularFile& operator=(RegularFile const& other);

	virtual std::uintmax_t size() const {
		return static_cast<std::uintmax_t>(-1);
	}

	virtual std::filesystem::file_time_type last_write_time() const {
		return std::filesystem::file_time_type::min();
	}

	virtual void last_write_time(std::filesystem::file_time_type new_time) = 0;

	virtual void resize(std::uintmax_t new_size) = 0;

	virtual std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const = 0;

	std::shared_ptr<std::istream> open_read() const {
		return this->open_read(std::ios_base::in);
	}

	virtual std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) = 0;

	std::shared_ptr<std::ostream> open_write() {
		return this->open_write(std::ios_base::out);
	}
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

		Iterator(std::shared_ptr<Cursor> cursor);

		Iterator& operator=(Iterator const&) = default;
		Iterator& operator=(Iterator&&)      = default;

		std::pair<std::string, std::shared_ptr<File>> const& operator*() {
			return this->value_;
		}

		std::pair<std::string, std::shared_ptr<File>> const* operator->() {
			return &this->value_;
		}

		Iterator& operator++();

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

	virtual bool insert(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual bool insert_or_assign(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual bool unlink(std::string const& name) = 0;

	virtual void mount(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual void unmount(std::string const& name) = 0;

	virtual std::uintmax_t erase(std::string const& name) = 0;

	virtual std::uintmax_t clear() = 0;

	virtual std::shared_ptr<Cursor> cursor() const = 0;

	Iterator begin() const {
		return this->empty()
		    ? Iterator()
		    : Iterator(this->cursor());
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

class MountPoint: virtual public File {
   public:
	std::intmax_t owner() const override {
		return this->attachment()->owner();
	}

	void owner(std::intmax_t owner) override {
		this->attachment()->owner(owner);
	}

	std::intmax_t group() const override {
		return this->attachment()->group();
	}

	void group(std::intmax_t group) override {
		this->attachment()->group(group);
	}

	std::filesystem::perms perms() const override {
		return this->attachment()->perms();
	}

	void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) override {
		this->attachment()->perms(prms, opts);
	}

	bool operator==(File const& other) const override {
		return this->attachment()->operator==(other);
	}

	virtual std::shared_ptr<File> attachment() const = 0;

	virtual std::shared_ptr<File> original() const = 0;
};

template<std::derived_from<File> F>
class TypedMountPoint
    : public MountPoint
    , public F {
   public:
	TypedMountPoint(
	    std::shared_ptr<F> attachment,
	    std::shared_ptr<F> original)
	    : attachment_(std::move(attachment))
	    , original_(std::move(original)) { }

	std::shared_ptr<File> attachment() const override {
		return this->attachment_;
	}

	std::shared_ptr<File> original() const override {
		return this->original_;
	}

   protected:
	std::shared_ptr<F> attachment_;
	std::shared_ptr<F> original_;
};

class MountedRegularFile: public TypedMountPoint<RegularFile> {
   public:
	MountedRegularFile(
	    std::shared_ptr<RegularFile> attachment,
	    std::shared_ptr<RegularFile> original)
	    : TypedMountPoint(std::move(attachment), std::move(original)) { }

	RegularFile& operator=(RegularFile const& other) override {
		return this->attachment_->operator=(other);
	}

	std::uintmax_t size() const override {
		return this->attachment_->size();
	}

	std::filesystem::file_time_type last_write_time() const override {
		return this->attachment_->last_write_time();
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		return this->attachment_->last_write_time(new_time);
	}

	void resize(std::uintmax_t new_size) override {
		return this->attachment_->resize(new_size);
	}

	std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override {
		return this->attachment_->open_read(mode);
	}

	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override {
		return this->attachment_->open_write(mode);
	}
};

class MountedDirectory: public TypedMountPoint<Directory> {
   public:
	MountedDirectory(
	    std::shared_ptr<Directory> attachment,
	    std::shared_ptr<Directory> original)
	    : TypedMountPoint(std::move(attachment), std::move(original)) { }

	bool empty() const override {
		return this->attachment_->empty();
	}

	bool contains(std::string const& name) const override {
		return this->attachment_->contains(name);
	}

	// returns nullptr if not exists.
	std::shared_ptr<File> next(std::string const& name) const override {
		return this->attachment_->next(name);
	}

	bool insert(std::string const& name, std::shared_ptr<File> file) override {
		return this->attachment_->insert(name, std::move(file));
	}

	bool insert_or_assign(std::string const& name, std::shared_ptr<File> file) override {
		return this->attachment_->insert_or_assign(name, std::move(file));
	}

	bool unlink(std::string const& name) override {
		return this->attachment_->unlink(name);
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override {
		this->attachment_->mount(name, std::move(file));
	}

	void unmount(std::string const& name) override {
		this->attachment_->unmount(name);
	}

	std::uintmax_t erase(std::string const& name) override {
		return this->attachment_->erase(name);
	}

	std::uintmax_t clear() override {
		return this->attachment_->clear();
	}

	std::shared_ptr<Cursor> cursor() const override {
		return this->attachment_->cursor();
	}
};

}  // namespace impl
}  // namespace vfs
