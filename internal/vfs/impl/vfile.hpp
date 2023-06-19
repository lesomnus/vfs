#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/os_file.hpp"

namespace vfs {
namespace impl {

class VFile: virtual public File {
   public:
	VFile(
	    std::intmax_t          owner,
	    std::intmax_t          group,
	    std::filesystem::perms perms)
	    : owner_(owner)
	    , group_(group)
	    , perms_(perms) { }

	VFile(VFile const& other) = default;
	VFile(VFile&& other)      = default;

	std::intmax_t owner() const override {
		return this->owner_;
	}

	void owner(std::intmax_t owner) override {
		this->owner_ = owner;
	}

	std::intmax_t group() const override {
		return this->group_;
	}

	void group(std::intmax_t group) override {
		this->group_ = group;
	}

	std::filesystem::perms perms() const override {
		return this->perms_;
	}

	void perms(std::filesystem::perms perms, std::filesystem::perm_options opts) override;

	bool operator==(File const& other) const override {
		return this == &other;
	}

   protected:
	std::intmax_t          owner_;
	std::intmax_t          group_;
	std::filesystem::perms perms_;
};

class NilFile
    : public VFile
    , public RegularFile {
   public:
	NilFile(
	    std::intmax_t          owner,
	    std::intmax_t          group,
	    std::filesystem::perms perms = DefaultPerms)
	    : VFile(owner, group, perms) { }

	NilFile(NilFile const& other) = default;
	NilFile(NilFile&& other)      = default;

	void last_write_time(std::filesystem::file_time_type new_time) override { }

	void resize(std::uintmax_t new_size) override { }

	std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override;
	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override;
};

class VRegularFile
    : public VFile
    , public TempRegularFile {
   public:
	VRegularFile(
	    std::intmax_t          owner,
	    std::intmax_t          group,
	    std::filesystem::perms perms = DefaultPerms);

	VRegularFile(VRegularFile const& other) = delete;
	VRegularFile(VRegularFile&& other)      = default;

	std::intmax_t owner() const override {
		return VFile::owner();
	}

	void owner(std::intmax_t owner) override {
		VFile::owner(owner);
	}

	std::intmax_t group() const override {
		return VFile::group();
	}

	void group(std::intmax_t group) override {
		VFile::group(group);
	}

	std::filesystem::perms perms() const override {
		return VFile::perms();
	}

	void perms(std::filesystem::perms perms, std::filesystem::perm_options opts) override {
		VFile::perms(perms, opts);
	}

	bool operator==(File const& other) const override {
		return VFile::operator==(other);
	}
};

class VSymlink
    : public VFile
    , public Symlink {
   public:
	VSymlink(std::filesystem::path target)
	    : VFile(0, 0, std::filesystem::perms::all)
	    , target_(std::move(target)) { }

	std::filesystem::path target() const override {
		return this->target_;
	}

   private:
	std::filesystem::path target_;
};

class VDirectory
    : public VFile
    , public Directory {
   public:
	VDirectory(std::intmax_t owner, std::intmax_t group, std::filesystem::perms perms = DefaultPerms)
	    : VFile(owner, group, perms) { }

	VDirectory(VDirectory const& other) = default;
	VDirectory(VDirectory&& other)      = default;

	bool empty() const override {
		return this->files_.empty();
	}

	bool contains(std::string const& name) const override {
		return this->files_.contains(name);
	}

	std::shared_ptr<File> next(std::string const& name) const override;

	bool insert(std::string const& name, std::shared_ptr<File> file) override;

	bool insert_or_assign(std::string const& name, std::shared_ptr<File> file) override;

	bool insert(std::string const& name, RemovableFile& file) override;

	bool insert_or_assign(std::string const& name, RemovableFile& file) override;

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

	std::shared_ptr<RemovableFile> removable(std::string const& name) override;

	bool unlink(std::string const& name) override {
		return !this->files_.extract(name).empty();
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override;

	void unmount(std::string const& name) override;

	std::uintmax_t erase(std::string const& name) override;

	std::uintmax_t clear() override;

	std::shared_ptr<Cursor> cursor() const override;

   private:
	std::unordered_map<std::string, std::shared_ptr<File>> files_;
};

}  // namespace impl
}  // namespace vfs
