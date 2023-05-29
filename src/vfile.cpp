#include "vfs/impl/vfile.hpp"
#include "vfs/impl/storage.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "vfs/impl/utils.hpp"

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
    : VFile(owner, group, perms) {
	auto const d = fs::temp_directory_path() / "vfs";
	fs::create_directories(d);

	this->sys_path_ = d / random_string(32, Alphanumeric);
	do {
		auto* f = std::fopen(this->sys_path_.c_str(), "wx");
		if(f != nullptr) {
			std::fclose(f);
			break;
		}

		this->sys_path_.replace_filename(random_string(32, Alphanumeric));
	} while(true);
}

VRegularFile::~VRegularFile() {
	if(this->sys_path_.empty()) {
		return;
	}

	fs::remove(this->sys_path_);
}

std::shared_ptr<std::istream> VRegularFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::ifstream>(this->sys_path_, mode | std::ios_base::in);
}

std::shared_ptr<std::ostream> VRegularFile::open_write(std::ios_base::openmode mode) {
	return std::make_shared<std::ofstream>(this->sys_path_, mode | std::ios_base::out);
}

std::shared_ptr<File> VDirectory::next(std::string const& name) const {
	auto it = this->files.find(name);
	if(it == this->files.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<RegularFile> VStorage::make_regular_file() const {
	return std::make_shared<VRegularFile>(0, 0);
}

std::shared_ptr<Directory> VStorage::make_directory() const {
	return std::make_shared<VDirectory>(0, 0);
}

std::shared_ptr<Symlink> VStorage::make_symlink(std::filesystem::path target) const {
	return std::make_shared<VSymlink>(std::move(target));
}

}  // namespace impl
}  // namespace vfs
