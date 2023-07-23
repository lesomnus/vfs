#include "vfs/impl/vfile.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "vfs/impl/file.hpp"
#include "vfs/impl/file_proxy.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

void VFile::perms(fs::perms prms, fs::perm_options opts) {
	switch(opts) {
	case fs::perm_options::replace: {
		this->perms_ = prms & fs::perms::mask;
		break;
	}
	case fs::perm_options::add: {
		this->perms_ |= (prms & fs::perms::mask);
		break;
	}
	case fs::perm_options::remove: {
		this->perms_ &= ~(prms & fs::perms::mask);
		break;
	}

	case fs::perm_options::nofollow: {
		break;
	}

	default: {
		auto const v = static_cast<std::underlying_type_t<fs::perm_options>>(opts);
		throw std::invalid_argument("unexpected value of \"std::filesystem::perm_options\": " + std::to_string(v));
	}
	}
}

VRegularFile::VRegularFile(fs::perms perms)
    : VFile(perms) { }

std::shared_ptr<File> VDirectory::next(std::string const& name) const {
	auto it = this->files_.find(name);
	if(it == this->files_.end()) {
		return nullptr;
	}

	return it->second;
}

std::uintmax_t VDirectory::erase(std::string const& name) {
	auto node = this->files_.extract(name);
	if(node.empty()) {
		return 0;
	}

	auto f = node.mapped();
	if(auto m = std::dynamic_pointer_cast<MountPoint>(std::move(f)); m) {
		node.mapped() = std::move(m);
		this->files_.insert(std::move(node));

		throw fs::filesystem_error("", "", std::make_error_code(std::errc::device_or_resource_busy));
	}

	auto d = std::dynamic_pointer_cast<Directory>(f);
	if(!d) {
		return 1;
	}

	return d->clear() + 1;
}

std::uintmax_t VDirectory::clear() {
	auto files = std::move(this->files_);
	for(auto const& [_, f]: files) {
		if(auto m = std::dynamic_pointer_cast<MountPoint>(f); m) {
			this->files_ = std::move(files);

			throw fs::filesystem_error("", "", std::make_error_code(std::errc::device_or_resource_busy));
		}
	}

	std::uintmax_t n = 0;
	for(auto const& [_, f]: files) {
		auto d = std::dynamic_pointer_cast<Directory>(f);
		if(d) {
			n += d->clear() + 1;
		} else {
			n += 1;
		}
	}

	return n;
}

std::pair<std::shared_ptr<RegularFile>, bool> VDirectory::emplace_regular_file(std::string const& name) {
	auto [it, ok] = this->files_.emplace(name, std::make_shared<VRegularFile>());
	return std::make_pair(std::dynamic_pointer_cast<RegularFile>(it->second), ok);
}

std::pair<std::shared_ptr<Directory>, bool> VDirectory::emplace_directory(std::string const& name) {
	auto [it, ok] = this->files_.emplace(name, std::make_shared<VDirectory>());
	return std::make_pair(std::dynamic_pointer_cast<Directory>(it->second), ok);
}

std::pair<std::shared_ptr<Symlink>, bool> VDirectory::emplace_symlink(std::string const& name, std::filesystem::path target) {
	auto [it, ok] = this->files_.emplace(name, std::make_shared<VSymlink>(std::move(target)));
	return std::make_pair(std::dynamic_pointer_cast<Symlink>(it->second), ok);
}

bool VDirectory::link(std::string const& name, std::shared_ptr<File> file) {
	if(auto proxy = std::dynamic_pointer_cast<FileProxyBase>(std::move(file)); proxy) {
		file = std::move(*proxy).target();
	}

	auto f = std::dynamic_pointer_cast<VFile>(std::move(file));
	if(!f) {
		throw fs::filesystem_error("cannot create link to different type of filesystem", std::make_error_code(std::errc::cross_device_link));
	}

	auto const [_, ok] = this->files_.insert(std::make_pair(name, std::move(f)));
	return ok;
}

std::shared_ptr<Directory::Cursor> VDirectory::cursor() const {
	return std::make_shared<StaticCursor>(this->files_);
}

}  // namespace impl
}  // namespace vfs
