#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/mount_point.hpp"
#include "vfs/impl/utils.hpp"

namespace vfs {
namespace impl {

class OsFile: virtual public File {
   public:
	struct Context {
		std::unordered_map<std::filesystem::path, std::shared_ptr<MountPoint>, PathHash> mount_points;
	};

	OsFile(std::shared_ptr<Context> context, std::filesystem::path p)
	    : context_(std::move(context))
	    , path_(std::move(p)) { }

	OsFile(std::filesystem::path p)
	    : context_(std::make_shared<Context>())
	    , path_(std::move(p)) { }

	OsFile(OsFile const& other) = default;
	OsFile(OsFile&& other)      = default;

	[[nodiscard]] std::filesystem::perms perms() const override {
		return std::filesystem::status(this->path_).permissions();
	}

	void perms(std::filesystem::perms perms, std::filesystem::perm_options opts) override {
		std::filesystem::permissions(this->path_, perms, opts);
	}

	bool operator==(File const& other) const override {
		auto const* f = dynamic_cast<OsFile const*>(&other);
		if(f == nullptr) {
			return false;
		}

		return std::filesystem::equivalent(this->path_, f->path_);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return std::filesystem::last_write_time(this->path_);
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		std::filesystem::last_write_time(this->path_, new_time);
	}

	[[nodiscard]] std::filesystem::path const& path() const {
		return this->path_;
	}

	void move_to(std::filesystem::path const& p) {
		std::filesystem::rename(this->path_, p);
		this->path_ = p;
	}

	[[nodiscard]] std::shared_ptr<Context> const& context() const {
		return this->context_;
	}

   protected:
	std::shared_ptr<Context> context_;
	std::filesystem::path    path_;
};

class UnkownOsFile: public OsFile {
   public:
	UnkownOsFile(std::filesystem::path p)
	    : OsFile(std::move(p)) { }

	[[nodiscard]] std::filesystem::file_type type() const override {
		return std::filesystem::status(this->path_).type();
	}
};

class OsRegularFile
    : public OsFile
    , public RegularFile {
   public:
	OsRegularFile(std::shared_ptr<Context> context, std::filesystem::path p)
	    : OsFile(std::move(context), std::move(p)) { }

	OsRegularFile(std::filesystem::path p)
	    : OsFile(std::move(p)) { }

	[[nodiscard]] std::uintmax_t size() const override {
		return std::filesystem::file_size(this->path_);
	}

	void resize(std::uintmax_t new_size) override {
		std::filesystem::resize_file(this->path_, new_size);
	}

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override {
		return std::make_shared<std::ifstream>(this->path_, mode | std::ios_base::in);
	}

	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override {
		return std::make_shared<std::ofstream>(this->path_, mode | std::ios_base::out);
	}
};

class OsSymlink
    : public OsFile
    , public Symlink {
   public:
	OsSymlink(std::shared_ptr<Context> context, std::filesystem::path p)
	    : OsFile(std::move(context), std::move(p)) { }

	OsSymlink(std::filesystem::path p)
	    : OsFile(std::move(p)) { }

	[[nodiscard]] std::filesystem::path target() const override {
		return std::filesystem::read_symlink(this->path_);
	}
};

class OsDirectory
    : public OsFile
    , public Directory {
   public:
	OsDirectory(std::shared_ptr<Context> context, std::filesystem::path p)
	    : OsFile(std::move(context), std::move(p)) { }

	OsDirectory(std::filesystem::path p)
	    : OsFile(std::move(p)) { }

	[[nodiscard]] bool exists(std::filesystem::path const& p) const;

	[[nodiscard]] bool empty() const override {
		return std::filesystem::is_empty(this->path_);
	}

	[[nodiscard]] bool contains(std::string const& name) const override {
		return this->exists(this->path_ / name);
	}

	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override;

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

	bool link(std::string const& name, std::shared_ptr<File> file) override;

	bool unlink(std::string const& name) override {
		return this->erase(name) > 0;
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override;

	void unmount(std::string const& name) override;

	std::uintmax_t erase(std::string const& name) override;

	std::uintmax_t clear() override;

	[[nodiscard]] std::shared_ptr<Cursor> cursor() const override;
};

class TempRegularFile: public OsRegularFile {
   public:
	TempRegularFile();
	TempRegularFile(TempRegularFile const& other) = delete;
	TempRegularFile(TempRegularFile&& other)      = default;

	~TempRegularFile() override;
};

class TempDirectory: public OsDirectory {
   public:
	TempDirectory();
	TempDirectory(TempDirectory const& other) = delete;
	TempDirectory(TempDirectory&& other)      = default;

	~TempDirectory() override;
};

class TempSymlink: public OsSymlink {
   public:
	TempSymlink(std::filesystem::path const& target);
	TempSymlink(TempSymlink const& other) = delete;
	TempSymlink(TempSymlink&& other)      = default;

	~TempSymlink() override;
};

}  // namespace impl
}  // namespace vfs
