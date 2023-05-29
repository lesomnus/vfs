#pragma once

#include <filesystem>
#include <memory>

#include "vfs/impl/file.hpp"

namespace vfs {
namespace impl {

class Storage {
   public:
	virtual std::shared_ptr<RegularFile> make_regular_file() const                        = 0;
	virtual std::shared_ptr<Directory>   make_directory() const                           = 0;
	virtual std::shared_ptr<Symlink>     make_symlink(std::filesystem::path target) const = 0;
};

class VStorage: public Storage {
   public:
	std::shared_ptr<RegularFile> make_regular_file() const override;
	std::shared_ptr<Directory>   make_directory() const override;
	std::shared_ptr<Symlink>     make_symlink(std::filesystem::path target) const override;
};

}  // namespace impl
}  // namespace vfs
