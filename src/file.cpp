#include "vfs/impl/file.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

void RegularFile::copy_from(RegularFile const& other) {
	*this->open_write() << other.open_read()->rdbuf();
	this->perms(other.perms(), fs::perm_options::replace);
}

Directory::StaticCursor::StaticCursor(std::unordered_map<std::string, std::shared_ptr<File>> const& files)
    : files_(files)
    , it_(this->files_.cbegin())
    , end_(this->files_.cend()) { }

Directory::StaticCursor::StaticCursor(std::unordered_map<std::string, std::shared_ptr<File>>&& files)
    : files_(std::move(files))
    , it_(this->files_.cbegin())
    , end_(this->files_.cend()) { }

std::string const& Directory::StaticCursor::name() const {
	return this->it_->first;
}

std::shared_ptr<File> const& Directory::StaticCursor::file() const {
	return this->it_->second;
}

void Directory::StaticCursor::increment() {
	if(this->at_end()) {
		return;
	}

	++this->it_;
}

bool Directory::StaticCursor::at_end() const {
	return this->it_ == this->end_;
}

Directory::Iterator::Iterator(std::shared_ptr<Cursor> cursor)
    : cursor_(std::move(cursor)) {
	if(this->cursor_->at_end()) {
		return;
	}

	this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
}

Directory::Iterator& Directory::Iterator::operator++() {
	if(!this->cursor_) {
		return *this;
	}

	this->cursor_->increment();
	if(this->cursor_->at_end()) {
		this->cursor_.reset();
	} else {
		this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
	}

	return *this;
}

}  // namespace impl
}  // namespace vfs
