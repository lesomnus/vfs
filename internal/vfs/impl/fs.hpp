#pragma once

#include <filesystem>
#include <memory>
#include <stdexcept>

#include "vfs/impl/file.hpp"

#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class FsBase: public Fs {
   public:
	virtual std::shared_ptr<File const> file_at(std::filesystem::path const& p) const = 0;

	virtual std::shared_ptr<File> file_at(std::filesystem::path const& p) = 0;

	virtual std::shared_ptr<Directory const> cwd() const = 0;

	virtual std::shared_ptr<Directory> cwd() = 0;
};

namespace {

std::logic_error err_unknown_fs_() {
	return std::logic_error("unknown fs");
}

}  // namespace

inline FsBase& fs_base(Fs& fs) {
	if(auto* b = dynamic_cast<FsBase*>(&fs); !b) {
		throw err_unknown_fs_();
	} else {
		return *b;
	}
}

inline FsBase const& fs_base(Fs const& fs) {
	if(auto const* b = dynamic_cast<FsBase const*>(&fs); !b) {
		throw err_unknown_fs_();
	} else {
		return *b;
	}
}

inline std::shared_ptr<FsBase> fs_base(std::shared_ptr<Fs>&& fs) {
	if(auto b = std::dynamic_pointer_cast<FsBase>(std::move(fs)); !b) {
		throw err_unknown_fs_();
	} else {
		return b;
	}
}

inline std::shared_ptr<FsBase> fs_base(std::shared_ptr<Fs> const& fs) {
	if(auto b = std::dynamic_pointer_cast<FsBase>(fs); !b) {
		throw err_unknown_fs_();
	} else {
		return b;
	}
}

}  // namespace impl
}  // namespace vfs
