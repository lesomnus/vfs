#include "vfs/impl/vfile.hpp"
#include "vfs/impl/storage.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "vfs/impl/entry.hpp"
#include "vfs/impl/file.hpp"
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
    : VFile(owner, group, perms) { }

std::shared_ptr<File> VDirectory::next(std::string const& name) const {
	auto it = this->files.find(name);
	if(it == this->files.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<Entry> VDirectory::next_entry(std::string const& name, std::shared_ptr<DirectoryEntry> const& prev) const {
	auto f = this->next(name);
	if(!f) {
		return nullptr;
	}

	using fs::file_type;
	switch(f->type()) {
	case file_type::regular:
		return EntryTypeOf<RegularFile>::make(std::move(name), prev, std::dynamic_pointer_cast<RegularFile>(std::move(f)));
	case file_type::directory:
		return EntryTypeOf<Directory>::make(std::move(name), prev, std::dynamic_pointer_cast<Directory>(std::move(f)));
	case file_type::symlink:
		return EntryTypeOf<Symlink>::make(std::move(name), prev, std::dynamic_pointer_cast<Symlink>(std::move(f)));

	default:
		throw std::logic_error("unexpected type of file");
	}
}

std::uintmax_t VDirectory::erase(std::string const& name) {
	auto node = this->files.extract(name);
	if(node.empty()) {
		return 0;
	}

	auto f = node.mapped();
	auto d = std::dynamic_pointer_cast<Directory>(f);
	if(!d) {
		return 1;
	}

	std::uintmax_t cnt = 1;
	for(auto const& [_, next_f]: *d) {
		++cnt;
		auto next_d = std::dynamic_pointer_cast<Directory>(next_f);
		if(!next_d) {
			continue;
		}

		std::vector<std::string> names;
		for(auto const& [name, _]: *next_d) {
			names.push_back(name);
		}
		for(auto const& name: names) {
			cnt += next_d->erase(name);
		}
	}

	return cnt;
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
