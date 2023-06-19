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

namespace {

class Removable_: public Directory::RemovableFile {
   public:
	Removable_(VDirectory* dir, std::string name, std::shared_ptr<File> file)
	    : dir(dir)
	    , name(std::move(name))
	    , file(std::move(file)) { }

	std::shared_ptr<File> value() override {
		return this->file;
	}

	void commit() override {
		this->dir->unlink(this->name);
	}

	VDirectory* dir;
	std::string name;

	std::shared_ptr<File> file;
};

class Cursor_: public Directory::Cursor {
   public:
	Cursor_(std::unordered_map<std::string, std::shared_ptr<File>> const& files)
	    : it(files.cbegin())
	    , end(files.cend()) { }

	std::string const& name() const override {
		return this->it->first;
	}

	std::shared_ptr<File> const& file() const override {
		return this->it->second;
	}

	void increment() override {
		if(this->at_end()) {
			return;
		}

		++this->it;
	}

	bool at_end() const override {
		return this->it == this->end;
	}

	std::unordered_map<std::string, std::shared_ptr<File>>::const_iterator it;
	std::unordered_map<std::string, std::shared_ptr<File>>::const_iterator end;
};

std::shared_ptr<File> conform_to_vfs_(std::shared_ptr<File> file) {
	auto f = std::dynamic_pointer_cast<OsFile>(file);
	if(!f) {
		return file;
	}

	if(auto r = std::dynamic_pointer_cast<VRegularFile>(std::move(f)); r) {
		return r;
	}

	auto const type = f->type();
	switch(type) {
	case fs::file_type::regular: {
		auto r  = std::dynamic_pointer_cast<OsRegularFile>(std::move(f));
		auto vr = std::make_shared<VRegularFile>(0, 0);

		r->copy_content_to(*vr);
		return vr;
	}

	case fs::file_type::symlink: {
		auto s = std::dynamic_pointer_cast<OsSymlink>(std::move(f));
		return std::make_shared<VSymlink>(s->target());
	}

	case fs::file_type::directory: {
		// TODO: use copy.
		throw std::runtime_error("not implemented");
	}

	default:
		throw std::runtime_error("file type not supported for Vfs");
	}
}

}  // namespace

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

bool VDirectory::insert(std::string const& name, std::shared_ptr<File> file) {
	if(this->contains(name)) {
		return false;
	}

	file = conform_to_vfs_(std::move(file));
	return this->files_.insert(std::make_pair(name, std::move(file))).second;
}

bool VDirectory::insert_or_assign(std::string const& name, std::shared_ptr<File> file) {
	file = conform_to_vfs_(std::move(file));
	return this->files_.insert_or_assign(name, std::move(file)).second;
}

bool VDirectory::insert(std::string const& name, Directory::RemovableFile& file) {
	// TODO: optimize.
	auto const inserted = this->insert(name, file.value());
	if(inserted) {
		file.commit();
	}

	return inserted;
}

bool VDirectory::insert_or_assign(std::string const& name, Directory::RemovableFile& file) {
	// TODO: optimize.
	file.commit();
	return this->insert_or_assign(name, file.value());
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

std::shared_ptr<Directory::RemovableFile> VDirectory::removable(std::string const& name) {
	auto f = this->next(name);
	if(!f) {
		return nullptr;
	}

	return std::make_shared<Removable_>(this, name, std::move(f));
}

std::shared_ptr<Directory::Cursor> VDirectory::cursor() const {
	return std::make_shared<Cursor_>(this->files_);
}

}  // namespace impl
}  // namespace vfs
