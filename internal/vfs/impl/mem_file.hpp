#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>

#include "vfs/impl/file.hpp"
#include "vfs/impl/vfile.hpp"

namespace vfs {
namespace impl {

class MemRegularFile
    : public VFile
    , public RegularFile
    , public std::enable_shared_from_this<MemRegularFile> {
   public:
	MemRegularFile(std::filesystem::perms perms);

	MemRegularFile()
	    : MemRegularFile(RegularFile::DefaultPerms) { }

	MemRegularFile(MemRegularFile const& other);
	MemRegularFile(MemRegularFile&& other) = default;

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return this->last_write_time_;
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		this->last_write_time_ = new_time;
	}

	[[nodiscard]] std::uintmax_t size() const override;

	void resize(std::uintmax_t new_size) override;

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override;

	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override;

	MemRegularFile& operator=(MemRegularFile const& other);
	MemRegularFile& operator=(MemRegularFile&& other) = default;

   private:
	std::shared_ptr<std::string>    data_;
	std::filesystem::file_time_type last_write_time_ = std::filesystem::file_time_type::clock::now();
};

class MemDirectory
    : public VDirectory {
   public:
	MemDirectory(std::filesystem::perms perms)
	    : VDirectory(perms) { }

	MemDirectory()
	    : VDirectory(DefaultPerms) { }

	MemDirectory(MemDirectory const& other) = default;
	MemDirectory(MemDirectory&& other)      = default;

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;
};

}  // namespace impl
}  // namespace vfs
