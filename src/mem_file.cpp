#include "vfs/impl/mem_file.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

MemRegularFile::MemRegularFile(
    std::intmax_t owner,
    std::intmax_t group,
    fs::perms     perms)
    : VFile(owner, group, perms)
    , data_(std::make_shared<std::string>()) { }

MemRegularFile::MemRegularFile(MemRegularFile const& other)
    : VFile(other)
    , data_(std::make_shared<std::string>(*other.data_)) { }

std::uintmax_t MemRegularFile::size() const {
	assert(nullptr != this->data_);
	return this->data_->size();
}

void MemRegularFile::resize(std::uintmax_t new_size) {
	assert(nullptr != this->data_);
	return this->data_->resize(new_size);
}

std::shared_ptr<std::istream> MemRegularFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::istringstream>(*this->data_, mode);
}

std::shared_ptr<std::ostream> MemRegularFile::open_write(std::ios_base::openmode mode) {
	using ios = std::ios_base;

	mode &= ios::out | ios::trunc | ios::app;
	switch(int(mode)) {
	case int(ios::out):
	case int(ios::trunc):
	case int(ios::app):
	case int(ios::out | ios::trunc):
	case int(ios::out | ios::app): {
		break;
	}

	default: {
		auto s = std::make_shared<std::stringstream>();
		s->setstate(ios::failbit);
		return s;
	}
	}

	return std::shared_ptr<std::ostringstream>(new std::ostringstream(), [mode, self = std::weak_ptr(this->shared_from_this())](std::ostringstream* p) {
		auto data = std::move(*p).str();
		delete p;

		auto f = self.lock();
		if(!f) {
			return;
		}

		switch(int(mode)) {
		case int(ios::out):
		case int(ios::trunc):
		case int(ios::out | ios::trunc): {
			f->data_ = std::make_shared<std::string>(std::move(data));
			break;
		}

		case int(ios::app):
		case int(ios::out | ios::app): {
			f->data_->append(std::move(data));
			break;
		}

		default: {
			throw std::logic_error("unreachable");
		}
		}

		f->last_write_time_ = fs::file_time_type::clock::now();
	});
}

MemRegularFile& MemRegularFile::operator=(MemRegularFile const& other) {
	assert(nullptr != this->data_);
	assert(nullptr != other.data_);

	*this->data_           = *other.data_;
	this->last_write_time_ = fs::file_time_type::clock::now();
	return *this;
}

std::pair<std::shared_ptr<RegularFile>, bool> MemDirectory::emplace_regular_file(std::string const& name) {
	auto [it, ok] = this->files_.emplace(name, std::make_shared<MemRegularFile>(0, 0));
	return std::make_pair(std::dynamic_pointer_cast<RegularFile>(it->second), ok);
}

std::pair<std::shared_ptr<Directory>, bool> MemDirectory::emplace_directory(std::string const& name) {
	auto [it, ok] = this->files_.emplace(name, std::make_shared<MemDirectory>(0, 0));
	return std::make_pair(std::dynamic_pointer_cast<Directory>(it->second), ok);
}

}  // namespace impl
}  // namespace vfs
