#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "vfs/impl/file.hpp"

#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class FsBase: public Fs {
   public:
	[[nodiscard]] virtual std::shared_ptr<File const> file_at(std::filesystem::path const& p) const = 0;

	virtual std::shared_ptr<File> file_at(std::filesystem::path const& p) = 0;

	[[nodiscard]] virtual std::shared_ptr<File const> file_at_followed(std::filesystem::path const& p) const = 0;

	virtual std::shared_ptr<File> file_at_followed(std::filesystem::path const& p) = 0;

	[[nodiscard]] virtual std::shared_ptr<Directory const> cwd() const = 0;

	virtual std::shared_ptr<Directory> cwd() = 0;
};

namespace {

std::logic_error err_unknown_fs_() {
	return std::logic_error("unknown fs");
}

}  // namespace

template<typename T>
requires std::same_as<std::remove_const_t<T>, Fs>
inline auto& fs_base(T& fs) {
	auto* b = dynamic_cast<std::conditional_t<std::is_const_v<T>, FsBase const, FsBase>*>(&fs);
	if(b == nullptr) {
		throw err_unknown_fs_();
	}
	return *b;
}

template<typename T>
requires std::same_as<std::remove_const_t<T>, Fs>
inline auto fs_base(std::shared_ptr<T> fs) {
	auto b = std::dynamic_pointer_cast<std::conditional_t<std::is_const_v<T>, FsBase const, FsBase>>(std::move(fs));
	if(!b) {
		throw err_unknown_fs_();
	}
	return b;
}

}  // namespace impl
}  // namespace vfs
