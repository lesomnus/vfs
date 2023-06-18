#include "vfs/impl/vfile.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

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

std::shared_ptr<std::istream> NilFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::ifstream>();
}

std::shared_ptr<std::ostream> NilFile::open_write(std::ios_base::openmode mode) {
	return std::make_shared<std::ofstream>();
}

VRegularFile::VRegularFile(
    std::intmax_t owner,
    std::intmax_t group,
    fs::perms     perms)
    : VFile(owner, group, perms) { }

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
	auto d = std::dynamic_pointer_cast<Directory>(f);
	if(!d) {
		return 1;
	}

	return d->clear() + 1;
}

std::uintmax_t VDirectory::clear() {
	auto files = std::move(this->files_);

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
	auto [it, ok] = this->files_.emplace(std::make_pair(name, std::make_shared<VRegularFile>(0, 0)));
	return std::make_pair(std::dynamic_pointer_cast<RegularFile>(it->second), ok);
}

std::pair<std::shared_ptr<Directory>, bool> VDirectory::emplace_directory(std::string const& name) {
	auto [it, ok] = this->files_.emplace(std::make_pair(name, std::make_shared<VDirectory>(0, 0)));
	return std::make_pair(std::dynamic_pointer_cast<Directory>(it->second), ok);
}

std::pair<std::shared_ptr<Symlink>, bool> VDirectory::emplace_symlink(std::string const& name, std::filesystem::path target) {
	auto [it, ok] = this->files_.emplace(std::make_pair(name, std::make_shared<VSymlink>(std::move(target))));
	return std::make_pair(std::dynamic_pointer_cast<Symlink>(it->second), ok);
}

}  // namespace impl
}  // namespace vfs
