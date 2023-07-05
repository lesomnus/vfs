#include "vfs/impl/os_file.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfile.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

namespace {

std::string random_filename_() {
	constexpr std::uint8_t TempFilenameLength = 32;
	return random_string(TempFilenameLength, Alphanumeric);
}

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

class Cursor_: public Directory::Cursor {
   public:
	Cursor_(std::shared_ptr<OsFile::Context> context, fs::path const& p)
	    : context_(std::move(context))
	    , it_(p) {
		this->refresh();
	}

	[[nodiscard]] std::string const& name() const override {
		return this->name_;
	}

	[[nodiscard]] std::shared_ptr<File> const& file() const override {
		return this->file_;
	}

	void increment() override {
		if(this->at_end()) {
			return;
		}

		this->it_++;
		this->refresh();
	}

	[[nodiscard]] bool at_end() const override {
		return this->it_ == this->end_;
	}

	void refresh() {
		if(Cursor_::at_end()) {
			this->name_ = "";
			this->file_ = nullptr;
		} else {
			this->name_ = this->it_->path().filename();
			this->file_ = make_file_(this->it_->symlink_status().type(), this->context_, this->it_->path());
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
		this->path_.replace_filename(random_filename_());

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

std::pair<std::shared_ptr<RegularFile>, bool> OsDirectory::emplace_regular_file(std::string const& name) {
	auto const next_p = this->path_ / name;
	if(auto const m = this->context_->mount_points.find(next_p); m != this->context_->mount_points.end()) {
		return std::make_pair(std::dynamic_pointer_cast<RegularFile>(m->second), false);
	}

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
	if(auto const m = this->context_->mount_points.find(next_p); m != this->context_->mount_points.end()) {
		return std::make_pair(std::dynamic_pointer_cast<Directory>(m->second), false);
	}

	try {
		// No exception is thrown if next_p is an existing directory and false is returned.
		auto const ok = fs::create_directory(next_p);
		if(!ok && fs::is_symlink(next_p)) {
			// If next_p is a symbolic link that linked to directory, no exception is thrown.
			return std::make_pair(nullptr, false);
		}

		return std::make_pair(std::make_shared<OsDirectory>(this->context_, next_p), ok);
	} catch(fs::filesystem_error const& error) {
		if(error.code() != std::errc::file_exists) {
			throw error;
		}

		// next_p exists but not a directory.
		return std::make_pair(nullptr, false);
	}
}

std::pair<std::shared_ptr<Symlink>, bool> OsDirectory::emplace_symlink(std::string const& name, std::filesystem::path target) {
	auto const next_p = this->path_ / name;
	if(auto const m = this->context_->mount_points.find(next_p); m != this->context_->mount_points.end()) {
		// Symbolic link cannot be mounted.
		return std::make_pair(nullptr, false);
	}

	auto ok = false;
	try {
		fs::create_symlink(target, next_p);
		ok = true;
	} catch(std::filesystem::filesystem_error const& error) {
		if(error.code() != std::errc::file_exists) {
			throw error;
		}
		if(not fs::is_symlink(next_p)) {
			return std::make_pair(nullptr, false);
		}
	}

	return std::make_pair(std::make_shared<OsSymlink>(this->context_, next_p), ok);
}

bool OsDirectory::link(std::string const& name, std::shared_ptr<File> file) {
	if(auto f = std::dynamic_pointer_cast<VFile>(std::move(file)); f) {
		throw fs::filesystem_error("cannot create link to different type of filesystem", std::make_error_code(std::errc::cross_device_link));
	}

	auto f = std::dynamic_pointer_cast<OsFile>(std::move(file));
	assert(nullptr != f);

	try {
		fs::create_hard_link(f->path(), this->path_ / name);
		return true;
	} catch(fs::filesystem_error const& error) {
		if(error.code() != std::errc::file_exists) {
			throw error;
		}

		return false;
	}
}

std::uintmax_t OsDirectory::erase(std::string const& name) {
	auto const target = this->path_ / name;
	for(auto const [p, _]: this->context_->mount_points) {
		auto const entry = *target.lexically_relative(p).begin();
		if(entry == "." || entry == "..") {
			throw fs::filesystem_error("", p, std::make_error_code(std::errc::device_or_resource_busy));
		}
	}

	return fs::remove_all(target);
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
		this->path_.replace_filename(random_filename_());
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
		this->path_.replace_filename(random_filename_());

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
