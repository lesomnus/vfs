#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace vfs {
namespace impl {

class File {
   public:
	virtual ~File() = default;

	[[nodiscard]] virtual std::filesystem::file_type type() const = 0;

	[[nodiscard]] virtual std::filesystem::space_info space() const {
		return std::filesystem::space_info{
		    .capacity  = static_cast<std::uintmax_t>(-1),
		    .free      = static_cast<std::uintmax_t>(-1),
		    .available = static_cast<std::uintmax_t>(-1),
		};
	}

	[[nodiscard]] virtual std::filesystem::perms perms() const = 0;

	virtual void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) = 0;

	void perms(std::filesystem::perms prms) {
		this->perms(prms, std::filesystem::perm_options::replace);
	}

	virtual bool operator==(File const& other) const = 0;

	[[nodiscard]] virtual std::filesystem::file_time_type last_write_time() const {
		return std::filesystem::file_time_type::min();
	}

	virtual void last_write_time(std::filesystem::file_time_type new_time) = 0;
};

class RegularFile: virtual public File {
   public:
	static constexpr auto Type = std::filesystem::file_type::regular;

	static constexpr auto DefaultPerms = std::filesystem::perms::owner_write
	    | std::filesystem::perms::owner_read
	    | std::filesystem::perms::group_read
	    | std::filesystem::perms::others_read;

	[[nodiscard]] std::filesystem::file_type type() const final {
		return Type;
	}

	void copy_from(RegularFile const& other);

	[[nodiscard]] virtual std::uintmax_t size() const {
		return static_cast<std::uintmax_t>(-1);
	}

	virtual void resize(std::uintmax_t new_size) = 0;

	[[nodiscard]] virtual std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const = 0;

	[[nodiscard]] std::shared_ptr<std::istream> open_read() const {
		return this->open_read(std::ios_base::in);
	}

	virtual std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) = 0;

	std::shared_ptr<std::ostream> open_write() {
		return this->open_write(std::ios_base::out);
	}
};

class Symlink: virtual public File {
   public:
	static constexpr auto Type = std::filesystem::file_type::symlink;

	[[nodiscard]] std::filesystem::file_type type() const final {
		return Type;
	}

	[[nodiscard]] virtual std::filesystem::path target() const = 0;
};

class Directory: virtual public File {
   public:
	static constexpr auto Type = std::filesystem::file_type::directory;

	static constexpr auto DefaultPerms = std::filesystem::perms::all
	    & ~std::filesystem::perms::group_write
	    & ~std::filesystem::perms::others_write;

	class Cursor {
	   public:
		virtual ~Cursor() = default;

		[[nodiscard]] virtual std::string const& name() const = 0;

		[[nodiscard]] virtual std::shared_ptr<File> const& file() const = 0;

		virtual void increment() = 0;

		[[nodiscard]] virtual bool at_end() const = 0;
	};

	class NilCursor: public Cursor {
	   public:
		NilCursor() = default;

		[[nodiscard]] std::string const& name() const override {
			throw std::runtime_error("invalid cursor");
		}

		[[nodiscard]] std::shared_ptr<File> const& file() const override {
			throw std::runtime_error("invalid cursor");
		}

		void increment() override {
		}

		[[nodiscard]] bool at_end() const override {
			return true;
		}
	};

	class StaticCursor: public Cursor {
	   public:
		StaticCursor(std::unordered_map<std::string, std::shared_ptr<File>> const& files);
		StaticCursor(std::unordered_map<std::string, std::shared_ptr<File>>&& files);

		[[nodiscard]] std::string const& name() const override;

		[[nodiscard]] std::shared_ptr<File> const& file() const override;

		void increment() override;

		[[nodiscard]] bool at_end() const override;

	   private:
		std::unordered_map<std::string, std::shared_ptr<File>> files_;

		std::unordered_map<std::string, std::shared_ptr<File>>::const_iterator it_;
		std::unordered_map<std::string, std::shared_ptr<File>>::const_iterator end_;
	};

	struct Iterator {
		Iterator() = default;

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

	[[nodiscard]] std::filesystem::file_type type() const final {
		return Type;
	}

	[[nodiscard]] virtual bool empty() const = 0;

	[[nodiscard]] virtual bool contains(std::string const& name) const = 0;

	// returns nullptr if not exists.
	[[nodiscard]] virtual std::shared_ptr<File> next(std::string const& name) const = 0;

	virtual std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) = 0;

	virtual std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) = 0;

	virtual std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) = 0;

	virtual bool link(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual bool unlink(std::string const& name) = 0;

	virtual void mount(std::string const& name, std::shared_ptr<File> file) = 0;

	virtual void unmount(std::string const& name) = 0;

	virtual std::uintmax_t erase(std::string const& name) = 0;

	virtual std::uintmax_t clear() = 0;

	[[nodiscard]] virtual std::shared_ptr<Cursor> cursor() const = 0;

	[[nodiscard]] Iterator begin() const {
		return this->empty()
		    ? Iterator()
		    : Iterator(this->cursor());
	}

	[[nodiscard]] static Iterator end() {
		return {};
	}
};

}  // namespace impl
}  // namespace vfs
