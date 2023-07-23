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
	VFile(std::filesystem::perms perms)
	    : perms_(perms) { }

	VFile(VFile const& other) = default;
	VFile(VFile&& other)      = default;

	[[nodiscard]] std::filesystem::perms perms() const override {
		return this->perms_;
	}

	void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) override;

	bool operator==(File const& other) const override {
		return this == &other;
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return this->last_write_time_;
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		this->last_write_time_ = new_time;
	}

	VFile& operator=(VFile const& other) = default;
	VFile& operator=(VFile&& other)      = default;

   protected:
	std::filesystem::perms          perms_;
	std::filesystem::file_time_type last_write_time_;
};

class VRegularFile
    : public VFile
    , public TempRegularFile {
   public:
	VRegularFile(std::filesystem::perms perms = DefaultPerms);

	VRegularFile(VRegularFile const& other) = delete;
	VRegularFile(VRegularFile&& other)      = default;

	[[nodiscard]] std::filesystem::perms perms() const override {
		return VFile::perms();
	}

	void perms(std::filesystem::perms perms, std::filesystem::perm_options opts) override {
		VFile::perms(perms, opts);
	}

	bool operator==(File const& other) const override {
		return VFile::operator==(other);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return TempRegularFile::last_write_time();
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		TempRegularFile::last_write_time(new_time);
	}
};

class VSymlink
    : public VFile
    , public Symlink {
   public:
	VSymlink(std::filesystem::path target)
	    : VFile(std::filesystem::perms::all)
	    , target_(std::move(target)) { }

	[[nodiscard]] std::filesystem::path target() const override {
		return this->target_;
	}

   private:
	std::filesystem::path target_;
};

class VDirectory
    : public VFile
    , public Directory {
   public:
	VDirectory(std::filesystem::perms perms = DefaultPerms)
	    : VFile(perms) { }

	VDirectory(VDirectory const& other) = default;
	VDirectory(VDirectory&& other)      = default;

	[[nodiscard]] bool empty() const override {
		return this->files_.empty();
	}

	[[nodiscard]] bool contains(std::string const& name) const override {
		return this->files_.contains(name);
	}

	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override;

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

	bool link(std::string const& name, std::shared_ptr<File> file) override;

	bool unlink(std::string const& name) override {
		return !this->files_.extract(name).empty();
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override;

	void unmount(std::string const& name) override;

	std::uintmax_t erase(std::string const& name) override;

	std::uintmax_t clear() override;

	[[nodiscard]] std::shared_ptr<Cursor> cursor() const override;

   protected:
	std::unordered_map<std::string, std::shared_ptr<File>> files_;
};

}  // namespace impl
}  // namespace vfs
