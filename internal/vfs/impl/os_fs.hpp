#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <type_traits>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/fs.hpp"
#include "vfs/impl/fs_proxy.hpp"
#include "vfs/impl/os_file.hpp"
#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfs.hpp"

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

template<typename T>
requires std::same_as<std::remove_const_t<T>, FsBase>
class OsFsProxy: public BasicFsProxy<T> {
   public:
	using BasicFsProxy<T>::BasicFsProxy;

	void mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) override;

   protected:
	[[nodiscard]] std::shared_ptr<BasicFsProxy<T const> const> make_proxy_(std::shared_ptr<Fs const> fs) const override {
		return std::make_shared<OsFsProxy<T const>>(*fs);
	}

	[[nodiscard]] std::shared_ptr<BasicFsProxy<T>> make_proxy_(std::shared_ptr<Fs> fs) override {
		return std::make_shared<OsFsProxy<T>>(*fs);
	}
};

class OsFs: public FsBase {
   public:
	[[nodiscard]] virtual std::shared_ptr<Vfs> make_mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) = 0;

	[[nodiscard]] std::shared_ptr<Directory const> cwd() const override {
		return std::make_shared<OsDirectory>(this->current_os_path());
	}

	[[nodiscard]] std::shared_ptr<Directory> cwd() override {
		return std::const_pointer_cast<Directory>(static_cast<OsFs const*>(this)->cwd());
	}

	[[nodiscard]] virtual std::filesystem::path base_path() const {
		return "/";
	}

	[[nodiscard]] std::filesystem::path current_os_path() const {
		return this->base_path() / this->current_path().relative_path();
	}

	[[nodiscard]] std::filesystem::path temp_directory_os_path() const {
		return this->base_path() / this->temp_directory_path();
	}

	[[nodiscard]] std::filesystem::path temp_directory_os_path(std::error_code ec) const {
		return this->base_path() / this->temp_directory_path(ec);
	}

	[[nodiscard]] virtual std::filesystem::path os_path_of(std::filesystem::path const& p) const = 0;

   private:
	void mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) final {
		throw std::logic_error("use make_mount");
	}

	void unmount(std::filesystem::path const& target) final;
};

class StdFs: public OsFs {
   public:
	StdFs(std::filesystem::path cwd)
	    : cwd_(std::move(cwd)) { }

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override {
		return std::make_shared<std::ifstream>(this->os_path_of(filename), mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override {
		return std::make_shared<std::ofstream>(this->os_path_of(filename), mode);
	}

	[[nodiscard]] std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override;

	[[nodiscard]] std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		return std::const_pointer_cast<Fs>(static_cast<StdFs const*>(this)->change_root(p, temp_dir));
	}

	[[nodiscard]] std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return std::filesystem::canonical(this->os_path_of(p));
	}

	[[nodiscard]] std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		if(p.empty()) {
			return p;
		}
		if(p.is_relative() && !this->exists(this->cwd_ / *p.begin())) {
			return p.lexically_normal();
		}

		return std::filesystem::weakly_canonical(this->os_path_of(p));
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		std::filesystem::copy(this->os_path_of(src), this->os_path_of(dst), opts);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		std::filesystem::copy(this->os_path_of(src), this->os_path_of(dst), opts, ec);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		return std::filesystem::copy_file(this->os_path_of(src), this->os_path_of(dst), opts);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		return std::filesystem::copy_file(this->os_path_of(src), this->os_path_of(dst), opts, ec);
	}

	bool create_directory(std::filesystem::path const& p) override {
		return std::filesystem::create_directory(this->os_path_of(p));
	}

	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->os_path_of(p), ec);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) override {
		return std::filesystem::create_directory(this->os_path_of(p), this->os_path_of(attr));
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->os_path_of(p), this->os_path_of(attr), ec);
	}

	bool create_directories(std::filesystem::path const& p) override {
		return std::filesystem::create_directories(this->os_path_of(p));
	}

	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::create_directories(this->os_path_of(p), ec);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_hard_link(this->os_path_of(target), this->os_path_of(link));
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_hard_link(this->os_path_of(target), this->os_path_of(link), ec);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_symlink(target, this->os_path_of(link));
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_symlink(target, this->os_path_of(link), ec);
	}

	[[nodiscard]] std::filesystem::path current_path() const override {
		return this->cwd_;
	}

	[[nodiscard]] std::filesystem::path current_path(std::error_code& ec) const override {
		return this->cwd_;
	}

	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p) const override {
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<StdFs>(this->canonical(p));
	}

	[[nodiscard]] std::shared_ptr<Fs> current_path(std::filesystem::path const& p) override {
		return std::const_pointer_cast<Fs>(static_cast<StdFs const*>(this)->current_path(p));
	}

	[[nodiscard]] bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override {
		return std::filesystem::equivalent(this->os_path_of(p1), this->os_path_of(p2));
	}

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p) const override {
		return std::filesystem::file_size(this->os_path_of(p));
	}

	[[nodiscard]] std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::file_size(this->os_path_of(p), ec);
	}

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p) const override {
		return std::filesystem::hard_link_count(this->os_path_of(p));
	}

	[[nodiscard]] std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::hard_link_count(this->os_path_of(p), ec);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override {
		return std::filesystem::last_write_time(this->os_path_of(p));
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::last_write_time(this->os_path_of(p), ec);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override {
		std::filesystem::last_write_time(this->os_path_of(p), t);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override {
		std::filesystem::last_write_time(this->os_path_of(p), t, ec);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override {
		std::filesystem::permissions(this->os_path_of(p), prms, opts);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override {
		std::filesystem::permissions(this->os_path_of(p), prms, opts, ec);
	}

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p) const override {
		return std::filesystem::read_symlink(this->os_path_of(p));
	}

	[[nodiscard]] std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::read_symlink(this->os_path_of(p), ec);
	}

	bool remove(std::filesystem::path const& p) override {
		return std::filesystem::remove(this->os_path_of(p));
	}

	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::remove(this->os_path_of(p), ec);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p) override {
		return std::filesystem::remove_all(this->os_path_of(p));
	}

	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::remove_all(this->os_path_of(p), ec);
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override {
		std::filesystem::rename(this->os_path_of(src), this->os_path_of(dst));
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override {
		std::filesystem::rename(this->os_path_of(src), this->os_path_of(dst), ec);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override {
		std::filesystem::resize_file(this->os_path_of(p), n);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override {
		std::filesystem::resize_file(this->os_path_of(p), n, ec);
	}

	[[nodiscard]] std::filesystem::space_info space(std::filesystem::path const& p) const override {
		return std::filesystem::space(this->os_path_of(p));
	}

	std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::space(this->os_path_of(p), ec);
	}

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p) const override {
		return std::filesystem::status(this->os_path_of(p));
	}

	[[nodiscard]] std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::status(this->os_path_of(p), ec);
	}

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override {
		return std::filesystem::symlink_status(this->os_path_of(p));
	}

	[[nodiscard]] std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::symlink_status(this->os_path_of(p), ec);
	}

	[[nodiscard]] std::filesystem::path temp_directory_path() const override {
		return std::filesystem::temp_directory_path();
	}

	[[nodiscard]] bool is_empty(std::filesystem::path const& p) const override {
		return std::filesystem::is_empty(this->os_path_of(p));
	}

	[[nodiscard]] std::shared_ptr<File const> file_at(std::filesystem::path const& p) const override;

	[[nodiscard]] std::shared_ptr<File> file_at(std::filesystem::path const& p) override {
		return std::const_pointer_cast<File>(static_cast<StdFs const*>(this)->file_at(p));
	}

	[[nodiscard]] std::shared_ptr<File const> file_at_followed(std::filesystem::path const& p) const override;

	[[nodiscard]] std::shared_ptr<File> file_at_followed(std::filesystem::path const& p) override {
		return std::const_pointer_cast<File>(static_cast<StdFs const*>(this)->file_at_followed(p));
	}

	[[nodiscard]] std::shared_ptr<Vfs> make_mount(std::filesystem::path const& target, Fs& other, std::filesystem::path const& source) override;

	[[nodiscard]] std::filesystem::path os_path_of(std::filesystem::path const& p) const override {
		return this->cwd_ / p;
	}

   protected:
	template<typename C, typename It>
	class Cursor_;

	class RecursiveCursor_;

	void copy_(std::filesystem::path const& src, Fs& other, std::filesystem::path const& dst, std::filesystem::copy_options opts) const override;

	[[nodiscard]] std::shared_ptr<Fs::Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

	[[nodiscard]] std::shared_ptr<Fs::RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override;

	std::filesystem::path cwd_;
};

class ChRootedStdFs: public StdFs {
   public:
	ChRootedStdFs(
	    std::filesystem::path const& base,
	    std::filesystem::path const& cwd,
	    std::filesystem::path const& temp_dir)
	    : StdFs(cwd)
	    , base_(std::filesystem::canonical(base))
	    , temp_dir_(("/" / temp_dir).lexically_normal()) { }

	[[nodiscard]] std::filesystem::path base_path() const override {
		return this->base_;
	}

	[[nodiscard]] std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return this->confine_(StdFs::canonical(p));
	}

	[[nodiscard]] std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		if(p.empty()) {
			return p;
		}

		return this->confine_(StdFs::weakly_canonical(p));
	}

	[[nodiscard]] std::shared_ptr<Fs const> current_path(std::filesystem::path const& p) const override {
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<ChRootedStdFs>(this->base_, std::move(c), this->temp_dir_);
	}

	[[nodiscard]] std::filesystem::path temp_directory_path() const override {
		if(this->temp_dir_.empty()) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::no_such_file_or_directory));
		}
		return this->temp_dir_;
	}

	[[nodiscard]] std::filesystem::path os_path_of(std::filesystem::path const& p) const override;

   protected:
	[[nodiscard]] std::filesystem::path confine_(std::filesystem::path const& normal) const {
		if(normal.is_relative()) {
			return normal;
		}

		return ("/" / normal.lexically_relative(this->base_)).lexically_normal();
	}

	std::filesystem::path base_;
	std::filesystem::path temp_dir_;
};

}  // namespace impl
}  // namespace vfs
