#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>

namespace vfs {
namespace impl {

class File {
   public:
	File(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms)
	    : owner(owner)
	    , group(group)
	    , perms(perms) { }

	File(File const& other) = default;
	File(File&& other)      = default;

	virtual ~File() = default;

	virtual std::filesystem::file_type type() const = 0;

	virtual std::uintmax_t count() const {
		return 1;
	}

	std::intmax_t          owner;
	std::intmax_t          group;
	std::filesystem::perms perms;
};

class Directory: public File {
   public:
	static constexpr auto DefaultPerms = std::filesystem::perms::all
	    & ~std::filesystem::perms::group_write
	    & ~std::filesystem::perms::others_write;

	static std::shared_ptr<Directory> make_temp() {
		using std::filesystem::perms;
		return std::make_shared<Directory>(0, 0, perms::all | perms::sticky_bit);
	}

	Directory(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms = DefaultPerms)
	    : File(owner, group, perms) { }

	Directory(Directory const& other) = default;
	Directory(Directory&& other)      = default;

	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::directory;
	}

	std::uintmax_t count() const override;

	std::unordered_map<std::string, std::shared_ptr<File>> files;
};

class RegularFile: public File {
   public:
	static constexpr auto DefaultPerms = std::filesystem::perms::owner_write
	    | std::filesystem::perms::owner_read
	    | std::filesystem::perms::group_read
	    | std::filesystem::perms::others_read;

	RegularFile(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms = DefaultPerms)
	    : File(owner, group, perms) { }

	// RegularFile(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms)
	//     : File(owner, group, perms) { }

	RegularFile(RegularFile const& other) = default;
	RegularFile(RegularFile&& other)      = default;

	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::regular;
	}

	virtual std::uintmax_t size() const = 0;

	virtual std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode = std::ios_base::in) const = 0;
	virtual std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode = std::ios_base::out)     = 0;
};

class EmptyFile: public RegularFile {
   public:
	EmptyFile(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms = DefaultPerms)
	    : RegularFile(owner, group, perms) { }

	EmptyFile(EmptyFile const& other) = default;
	EmptyFile(EmptyFile&& other)      = default;

	std::uintmax_t size() const override {
		return 0;
	}

	std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode = std::ios_base::out) override;
};

class TempFile: public RegularFile {
   public:
	static std::filesystem::path temp_directory() {
		return std::filesystem::temp_directory_path() / "vfs";
	}

	TempFile(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms = DefaultPerms);

	TempFile(TempFile const& other) = delete;
	TempFile(TempFile&& other);

	~TempFile();

	std::uintmax_t size() const override;

	std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode = std::ios_base::in) const override;
	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode = std::ios_base::out) override;

	std::filesystem::path sys_path() const {
		return this->sys_path_;
	}

   private:
	std::filesystem::path sys_path_;
};

class Symlink: public File {
   public:
	Symlink(std::filesystem::path target)
	    : File(0, 0, std::filesystem::perms::all)
	    , target_(std::move(target)) { }

	std::filesystem::file_type type() const final {
		return std::filesystem::file_type::symlink;
	}

	std::filesystem::path target() const {
		return this->target_;
	}

   private:
	std::filesystem::path target_;
};

}  // namespace impl
}  // namespace vfs
