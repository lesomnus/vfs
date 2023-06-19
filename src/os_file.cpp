#include "vfs/impl/os_file.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

namespace {

fs::path temp_directory_() {
	return std::filesystem::temp_directory_path() / "vfs";
}

bool is_in_temp_directory_(fs::path const& p) {
	return p.parent_path() == temp_directory_();
}

std::shared_ptr<File> make_file_(fs::file_type type, std::shared_ptr<OsFile::Context> context, fs::path const& p) {
	switch(type) {
	case fs::file_type::regular:
		return std::make_shared<OsRegularFile>(std::move(context), p);
	case fs::file_type::directory:
		return std::make_shared<OsDirectory>(std::move(context), p);
	case fs::file_type::symlink:
		return std::make_shared<OsSymlink>(std::move(context), p);

	default:
		return std::make_shared<UnkownOsFile>(p);
	}
}

class RemovableFile_: public Directory::RemovableFile {
   public:
	RemovableFile_(std::shared_ptr<File> file)
	    : file(std::move(file)) { }

	std::shared_ptr<File> value() override {
		return this->file;
	}

	void commit() override { }

	std::shared_ptr<File> file;
};

class Cursor_: public Directory::Cursor {
   public:
	Cursor_(std::shared_ptr<OsFile::Context> context, fs::path const& p)
	    : context_(std::move(context))
	    , it_(p) {
		this->refresh();
	}

	std::string const& name() const override {
		return this->name_;
	}

	std::shared_ptr<File> const& file() const override {
		return this->file_;
	}

	void increment() override {
		if(this->at_end()) {
			return;
		}

		this->it_++;
		this->refresh();
	}

	bool at_end() const override {
		return this->it_ == this->end_;
	}

	void refresh() {
		if(this->at_end()) {
			this->name_ = "";
			this->file_ = nullptr;
		} else {
			this->name_ = this->it_->path().filename();
			this->file_ = make_file_(this->it_->status().type(), this->context_, this->it_->path());
		}
	}

   private:
	std::shared_ptr<OsFile::Context> context_;

	fs::directory_iterator it_;
	fs::directory_iterator end_;

	std::string           name_;
	std::shared_ptr<File> file_;
};

}  // namespace

TempRegularFile::TempRegularFile()
    : OsRegularFile("") {
	auto const d = temp_directory_();

	this->path_ = d / "foo";
	do {
		fs::create_directories(d);
		this->path_.replace_filename(random_string(32, Alphanumeric));

		auto* f = std::fopen(this->path_.c_str(), "wx");
		if(f != nullptr) {
			std::fclose(f);
			break;
		}
	} while(true);
}

TempRegularFile::~TempRegularFile() {
	if(!is_in_temp_directory_(this->path_)) {
		return;
	}

	fs::remove(this->path_);
}

bool OsDirectory::exists(std::filesystem::path const& p) const {
	return this->context_->mount_points.contains(p) || std::filesystem::exists(p);
}

std::shared_ptr<File> OsDirectory::next(std::string const& name) const {
	auto const next_p = this->path_ / name;
	if(auto it = this->context_->mount_points.find(next_p); it != this->context_->mount_points.end()) {
		return it->second;
	}

	auto const status = fs::symlink_status(next_p);
	if(not fs::exists(status)) {
		return nullptr;
	}

	return make_file_(status.type(), this->context_, next_p);
}

bool OsDirectory::insert(std::string const& name, std::shared_ptr<File> file) {
	if(auto const f = std::dynamic_pointer_cast<OsFile>(std::move(file)); f) {
		auto const next_p = this->path_ / name;
		if(this->exists(next_p)) {
			return false;
		}

		fs::copy(f->path(), next_p);
		return true;
	}

	auto const type = file->type();
	switch(type) {
	case fs::file_type::regular: {
		auto [dst, ok] = this->emplace_regular_file(name);
		if(ok) {
			auto r = std::dynamic_pointer_cast<RegularFile>(std::move(file));
			r->copy_content_to(*dst);
		}

		return ok;
	}
	case fs::file_type::symlink: {
		auto s = std::dynamic_pointer_cast<Symlink>(std::move(file));

		auto [_, ok] = this->emplace_symlink(name, s->target());
		return ok;
	}
	case fs::file_type::directory: {
		// TODO:
		throw std::runtime_error("not implemented");
	}

	default:
		throw std::runtime_error("unexpected type of file");
	}

	// unreachable.
}

bool OsDirectory::insert_or_assign(std::string const& name, std::shared_ptr<File> file) {
	auto const next_p = this->path_ / name;
	if(!this->exists(next_p)) {
		fs::remove_all(next_p);
	}

	return this->insert(name, std::move(file));
}

bool OsDirectory::insert(std::string const& name, Directory::RemovableFile& file) {
	auto const next_p = this->path_ / name;

	auto f = std::dynamic_pointer_cast<OsFile>(file.value());
	if(!f) {
		auto const ok = this->insert(name, file);
		if(ok) {
			file.commit();
		}
		return ok;
	}

	if(this->exists(next_p)) {
		return false;
	}

	f->move_to(next_p);
	file.commit();
	return true;
}

bool OsDirectory::insert_or_assign(std::string const& name, Directory::RemovableFile& file) {
	auto const next_p = this->path_ / name;

	auto f = std::dynamic_pointer_cast<OsFile>(file.value());
	if(!f) {
		auto const ok = this->insert(name, file);
		file.commit();
		return ok;
	}

	f->move_to(next_p);
	file.commit();
	return true;
}

std::pair<std::shared_ptr<RegularFile>, bool> OsDirectory::emplace_regular_file(std::string const& name) {
	auto const next_p = this->path_ / name;

	auto*      f  = std::fopen(next_p.c_str(), "wx");
	auto const ok = f != nullptr;
	if(ok) {
		std::fclose(f);
	} else if(not fs::is_regular_file(next_p)) {
		return std::make_pair(nullptr, false);
	}

	return std::make_pair(std::make_shared<OsRegularFile>(this->context_, next_p), ok);
}

std::pair<std::shared_ptr<Directory>, bool> OsDirectory::emplace_directory(std::string const& name) {
	auto const next_p = this->path_ / name;
	auto const ok     = fs::create_directory(next_p);
	if(not ok && not fs::is_directory(next_p)) {
		return std::make_pair(nullptr, false);
	}

	return std::make_pair(std::make_shared<OsDirectory>(this->context_, next_p), ok);
}

std::pair<std::shared_ptr<Symlink>, bool> OsDirectory::emplace_symlink(std::string const& name, std::filesystem::path target) {
	auto const next_p = this->path_ / name;

	auto ok = false;
	try {
		fs::create_symlink(target, next_p);
		ok = true;
	} catch(std::filesystem::filesystem_error const& error) {
		if(error.code() != std::errc::file_exists) {
			throw error;
		} else if(not fs::is_symlink(next_p)) {
			return std::make_pair(nullptr, false);
		}
	}

	return std::make_pair(std::make_shared<OsSymlink>(this->context_, next_p), ok);
}

std::shared_ptr<Directory::RemovableFile> OsDirectory::removable(std::string const& name) {
	auto f = this->next(name);
	if(!f) {
		return nullptr;
	}

	return std::make_shared<RemovableFile_>(std::move(f));
}

std::uintmax_t OsDirectory::clear() {
	std::uintmax_t cnt = 0;
	for(auto const& dir_entry: fs::directory_iterator{this->path_}) {
		cnt += fs::remove_all(dir_entry);
	}

	return cnt;
}

std::shared_ptr<Directory::Cursor> OsDirectory::cursor() const {
	return std::make_shared<Cursor_>(this->context_, this->path_);
}

TempDirectory::TempDirectory()
    : OsDirectory("") {
	this->path_ = temp_directory_() / "foo";
	do {
		this->path_.replace_filename(random_string(32, Alphanumeric));
	} while(!fs::create_directories(this->path_));
}

TempDirectory::~TempDirectory() {
	if(!is_in_temp_directory_(this->path_)) {
		return;
	}

	fs::remove_all(this->path_);
}

TempSymlink::TempSymlink(fs::path const& target)
    : OsSymlink("") {
	auto const d = temp_directory_();

	this->path_ = d / "foo";
	do {
		fs::create_directories(d);
		this->path_.replace_filename(random_string(32, Alphanumeric));

		try {
			fs::create_symlink(target, this->path_);
		} catch(std::filesystem::filesystem_error const& error) {
			if(error.code() != std::errc::file_exists) {
				throw error;
			}
		}
	} while(true);
}

TempSymlink::~TempSymlink() {
	if(!is_in_temp_directory_(this->path_)) {
		return;
	}

	fs::remove(this->path_);
}

}  // namespace impl
}  // namespace vfs