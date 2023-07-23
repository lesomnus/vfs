#pragma once

#include <concepts>
#include <memory>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/file_proxy.hpp"
#include "vfs/impl/mount_point.hpp"

namespace vfs {
namespace impl {

class MountPoint: virtual public File {
   public:
	MountPoint(std::shared_ptr<File> original)
	    : original_(std::move(original)) { }

	[[nodiscard]] virtual std::shared_ptr<File> attachment() const = 0;

	[[nodiscard]] std::shared_ptr<File> const& original() const& {
		return this->original_;
	}

	[[nodiscard]] std::shared_ptr<File>&& original() && {
		return std::move(this->original_);
	}

   private:
	std::shared_ptr<File> original_;
};

template<std::derived_from<FileProxy> Proxy>
class TypedMountPoint
    : public MountPoint
    , public Proxy {
   public:
	using TargetType = Proxy::TargetType;

	TypedMountPoint(std::shared_ptr<TargetType> attachment, std::shared_ptr<TargetType> original)
	    : MountPoint(std::move(original))
	    , Proxy(std::move(attachment)) { }

	[[nodiscard]] std::shared_ptr<File> attachment() const override {
		return Proxy::origin_;
	}
};

using MountedRegularFile = TypedMountPoint<RegularFileProxy<>>;
using MountedDirectory   = TypedMountPoint<DirectoryProxy<>>;

}  // namespace impl
}  // namespace vfs
