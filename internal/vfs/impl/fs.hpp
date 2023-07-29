#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "vfs/impl/file.hpp"
#include "vfs/impl/utils.hpp"

#include "vfs/fs.hpp"

#define VFS_FS_METHOD_NAMES      \
	Fs::canonical,               \
	    Fs::weakly_canonical,    \
	    Fs::copy,                \
	    Fs::copy_file,           \
	    Fs::create_directory,    \
	    Fs::create_directories,  \
	    Fs::create_hard_link,    \
	    Fs::create_symlink,      \
	    Fs::current_path,        \
	    Fs::equivalent,          \
	    Fs::file_size,           \
	    Fs::hard_link_count,     \
	    Fs::last_write_time,     \
	    Fs::permissions,         \
	    Fs::read_symlink,        \
	    Fs::remove,              \
	    Fs::remove_all,          \
	    Fs::rename,              \
	    Fs::resize_file,         \
	    Fs::space,               \
	    Fs::status,              \
	    Fs::symlink_status,      \
	    Fs::temp_directory_path, \
	    Fs::is_empty

namespace vfs {
namespace impl {

class FsBase: public Fs {
   public:
	using VFS_FS_METHOD_NAMES;

	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->canonical(p); }, ec);
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->weakly_canonical(p); }, ec);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		handle_error([&] { this->copy(src, dst, opts); return 0; }, ec);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		return handle_error([&] { return this->copy_file(src, dst, opts); }, ec);
	}

	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return handle_error([&] { return this->create_directory(p); }, ec);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept override {
		return handle_error([&] { return this->create_directory(p, attr); }, ec);
	}

	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override {
		return handle_error([&] { return this->create_directories(p); }, ec);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		handle_error([&] { this->create_hard_link(target, link); return 0; }, ec);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		handle_error([&] { this->create_symlink(target, link); return 0; }, ec);
	}

	[[nodiscard]] std::filesystem::path current_path(std::error_code& ec) const override {
		return handle_error([&] { return this->current_path(); }, ec);
	}

	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->current_path(p); }, ec);
	}

	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return handle_error([&] { return this->current_path(p); }, ec);
	}

	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->equivalent(p1, p2); }, ec);
	}

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->file_size(p); }, ec, static_cast<std::uintmax_t>(-1));
	}

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->hard_link_count(p); }, ec, static_cast<std::uintmax_t>(-1));
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->last_write_time(p); }, ec);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override {
		handle_error([&] { this->last_write_time(p, t); return 0; }, ec);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override {
		handle_error([&] { this->permissions(p, prms, opts); return 0; }, ec);
	}

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->read_symlink(p); }, ec);
	}

	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return handle_error([&] { return this->remove(p); }, ec);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override {
		return handle_error([&] { return this->remove_all(p); }, ec, static_cast<std::uintmax_t>(-1));
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override {
		handle_error([&] { this->rename(src, dst); return 0; }, ec);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override {
		handle_error([&] { this->resize_file(p, n); return 0; }, ec);
	}

	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->space(p); }, ec);
	}

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->status(p); }, ec);
	}

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->symlink_status(p); }, ec);
	}

	[[nodiscard]] std::filesystem::path temp_directory_path(std::error_code& ec) const override {
		return handle_error([&] { return this->temp_directory_path(); }, ec);
	}

	[[nodiscard]] bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->is_empty(p); }, ec);
	}

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
