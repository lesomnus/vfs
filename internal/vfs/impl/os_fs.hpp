#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

#include "vfs/impl/fs_proxy.hpp"
#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfs.hpp"

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class OsFs: public Fs {
   public:
	virtual std::shared_ptr<Vfs> make_mount(std::filesystem::path const& target, Fs& other) = 0;

	virtual std::filesystem::path base_path() const {
		return "/";
	}

	std::filesystem::path current_os_path() const {
		return this->base_path() / this->current_path();
	}

	std::filesystem::path temp_directory_os_path() const {
		return this->base_path() / this->temp_directory_path();
	}

	std::filesystem::path temp_directory_os_path(std::error_code ec) const {
		return this->base_path() / this->temp_directory_path(ec);
	}

   private:
	void mount(std::filesystem::path const& target, Fs& other) final {
		throw std::logic_error("use make_mount");
	}

	void unmount(std::filesystem::path const& target) final;
};

class OsFsProxy: public FsProxy {
   public:
	OsFsProxy(std::shared_ptr<Fs> fs)
	    : FsProxy(std::move(fs)) { }

	void mount(std::filesystem::path const& target, Fs& other) override;

   protected:
	std::shared_ptr<FsProxy> make_proxy_(std::shared_ptr<Fs> fs) const override {
		return std::make_shared<OsFsProxy>(std::move(fs));
	}
};

class StdFs: public OsFs {
   public:
	StdFs(std::filesystem::path const& cwd)
	    : cwd_(cwd) { }

	std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override {
		return std::make_shared<std::ifstream>(this->normal_(filename), mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override {
		return std::make_shared<std::ofstream>(this->normal_(filename), mode);
	}

	std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override;

	std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		return std::const_pointer_cast<Fs>(static_cast<StdFs const*>(this)->change_root(p, temp_dir));
	}

	std::shared_ptr<Vfs> make_mount(std::filesystem::path const& target, Fs& other) override;

	std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return std::filesystem::canonical(this->normal_(p));
	}

	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->canonical(p); }, ec);
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		if(p.empty()) {
			return p;
		}
		if(p.is_relative() && !this->exists(this->cwd_ / *p.begin())) {
			return p.lexically_normal();
		}

		return std::filesystem::weakly_canonical(this->normal_(p));
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->weakly_canonical(p); }, ec);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		std::filesystem::copy(this->normal_(src), this->normal_(dst), opts);
	}

	void copy(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		std::filesystem::copy(this->normal_(src), this->normal_(dst), opts, ec);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts) override {
		return std::filesystem::copy_file(this->normal_(src), this->normal_(dst), opts);
	}

	bool copy_file(std::filesystem::path const& src, std::filesystem::path const& dst, std::filesystem::copy_options opts, std::error_code& ec) override {
		return std::filesystem::copy_file(this->normal_(src), this->normal_(dst), opts, ec);
	}

	bool create_directory(std::filesystem::path const& p) override {
		return std::filesystem::create_directory(this->normal_(p));
	}

	bool create_directory(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->normal_(p), ec);
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr) override {
		return std::filesystem::create_directory(this->normal_(p), this->normal_(attr));
	}

	bool create_directory(std::filesystem::path const& p, std::filesystem::path const& attr, std::error_code& ec) noexcept override {
		return std::filesystem::create_directory(this->normal_(p), this->normal_(attr), ec);
	}

	bool create_directories(std::filesystem::path const& p) override {
		return std::filesystem::create_directories(this->normal_(p));
	}

	bool create_directories(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::create_directories(this->normal_(p), ec);
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_hard_link(this->normal_(target), this->normal_(link));
	}

	void create_hard_link(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_hard_link(this->normal_(target), this->normal_(link), ec);
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link) override {
		std::filesystem::create_symlink(target, this->normal_(link));
	}

	void create_symlink(std::filesystem::path const& target, std::filesystem::path const& link, std::error_code& ec) noexcept override {
		std::filesystem::create_symlink(target, this->normal_(link), ec);
	}

	std::filesystem::path current_path() const override {
		return this->cwd_;
	}

	std::filesystem::path current_path(std::error_code& ec) const override {
		return this->cwd_;
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& override {
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<StdFs>(this->canonical(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override {
		// TODO: move
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<StdFs>(this->canonical(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) const& noexcept override {
		return handle_error([&] { return this->current_path(p); }, ec);
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p, std::error_code& ec) && noexcept override {
		return handle_error([&] { return std::move(*this).current_path(p); }, ec);
	}

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2) const override {
		return std::filesystem::equivalent(this->normal_(p1), this->normal_(p2));
	}

	bool equivalent(std::filesystem::path const& p1, std::filesystem::path const& p2, std::error_code& ec) const noexcept override {
		return handle_error([&] { return this->equivalent(p1, p2); }, ec);
	}

	std::uintmax_t file_size(std::filesystem::path const& p) const override {
		return std::filesystem::file_size(this->normal_(p));
	}

	std::uintmax_t file_size(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::file_size(this->normal_(p), ec);
	}

	std::uintmax_t hard_link_count(std::filesystem::path const& p) const override {
		return std::filesystem::hard_link_count(this->normal_(p));
	}

	std::uintmax_t hard_link_count(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::hard_link_count(this->normal_(p), ec);
	}

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p) const override {
		return std::filesystem::last_write_time(this->normal_(p));
	}

	std::filesystem::file_time_type last_write_time(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::last_write_time(this->normal_(p), ec);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t) override {
		std::filesystem::last_write_time(this->normal_(p), t);
	}

	void last_write_time(std::filesystem::path const& p, std::filesystem::file_time_type t, std::error_code& ec) noexcept override {
		std::filesystem::last_write_time(this->normal_(p), t, ec);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts = std::filesystem::perm_options::replace) override {
		std::filesystem::permissions(this->normal_(p), prms, opts);
	}

	void permissions(std::filesystem::path const& p, std::filesystem::perms prms, std::filesystem::perm_options opts, std::error_code& ec) override {
		std::filesystem::permissions(this->normal_(p), prms, opts, ec);
	}

	std::filesystem::path read_symlink(std::filesystem::path const& p) const override {
		return std::filesystem::read_symlink(this->normal_(p));
	}

	std::filesystem::path read_symlink(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::read_symlink(this->normal_(p), ec);
	}

	bool remove(std::filesystem::path const& p) override {
		return std::filesystem::remove(this->normal_(p));
	}

	bool remove(std::filesystem::path const& p, std::error_code& ec) noexcept override {
		return std::filesystem::remove(this->normal_(p), ec);
	}

	std::uintmax_t remove_all(std::filesystem::path const& p) override {
		return std::filesystem::remove_all(this->normal_(p));
	}

	std::uintmax_t remove_all(std::filesystem::path const& p, std::error_code& ec) override {
		return std::filesystem::remove_all(this->normal_(p), ec);
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst) override {
		std::filesystem::rename(this->normal_(src), this->normal_(dst));
	}

	void rename(std::filesystem::path const& src, std::filesystem::path const& dst, std::error_code& ec) noexcept override {
		std::filesystem::rename(this->normal_(src), this->normal_(dst), ec);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n) override {
		std::filesystem::resize_file(this->normal_(p), n);
	}

	void resize_file(std::filesystem::path const& p, std::uintmax_t n, std::error_code& ec) noexcept override {
		std::filesystem::resize_file(this->normal_(p), n, ec);
	}

	std::filesystem::space_info space(std::filesystem::path const& p) const override {
		return std::filesystem::space(this->normal_(p));
	}

	std::filesystem::space_info space(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::space(this->normal_(p), ec);
	}

	std::filesystem::file_status status(std::filesystem::path const& p) const override {
		return std::filesystem::status(this->normal_(p));
	}

	std::filesystem::file_status status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::status(this->normal_(p), ec);
	}

	std::filesystem::file_status symlink_status(std::filesystem::path const& p) const override {
		return std::filesystem::symlink_status(this->normal_(p));
	}

	std::filesystem::file_status symlink_status(std::filesystem::path const& p, std::error_code& ec) const noexcept override {
		return std::filesystem::symlink_status(this->normal_(p), ec);
	}

	std::filesystem::path temp_directory_path() const override {
		return std::filesystem::temp_directory_path();
	}

	std::filesystem::path temp_directory_path(std::error_code& ec) const override {
		return handle_error([&] { return this->temp_directory_path(); }, ec);
	}

	bool is_empty(std::filesystem::path const& p) const override {
		return std::filesystem::is_empty(this->normal_(p));
	}

	bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override {
		return handle_error([&] { return this->is_empty(p); }, ec);
	}

   protected:
	virtual std::filesystem::path normal_(std::filesystem::path const& p) const {
		return this->cwd_ / p;
	}

	template<typename C, typename It>
	class BasicCursor: public C {
	   public:
		BasicCursor(StdFs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
		    : path_(p)
		    , normal_path_(fs.normal_(p))
		    , it_(this->normal_path_, opts)
		    , entry_(fs) {
			this->refresh_();
		}

		directory_entry const& value() const override {
			return this->entry_;
		}

		bool at_end() const override {
			return this->it_ == std::filesystem::end(this->it_);
		}

		void increment() override {
			if(this->at_end()) {
				return;
			}

			this->it_++;
			this->refresh_();
			return;
		}

	   protected:
		void refresh_() {
			if(this->at_end()) {
				this->entry_ = directory_entry();
			} else {
				auto const p = this->it_->path();
				auto const r = p.lexically_relative(this->normal_path_);

				this->entry_.assign(this->path_ / r);
			}
		}

		std::filesystem::path path_;
		std::filesystem::path normal_path_;

		It                                 it_;
		std::filesystem::directory_options opts_;

		directory_entry entry_;
	};

	using Cursor = BasicCursor<Fs::Cursor, std::filesystem::directory_iterator>;

	class RecursiveCursor: public BasicCursor<Fs::RecursiveCursor, std::filesystem::recursive_directory_iterator> {
	   public:
		RecursiveCursor(StdFs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
		    : BasicCursor(fs, p, opts) { }

		std::filesystem::directory_options options() override {
			return this->it_.options();
		}

		int depth() const override {
			return this->it_.depth();
		}

		bool recursion_pending() const override {
			return this->it_.recursion_pending();
		}

		void pop() override {
			this->it_.pop();
			this->refresh_();
		}

		void disable_recursion_pending() override {
			this->it_.disable_recursion_pending();
		}
	};

	std::shared_ptr<Fs::Cursor> cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return std::make_shared<StdFs::Cursor>(*this, p, opts);
	}

	std::shared_ptr<Fs::RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return std::make_shared<StdFs::RecursiveCursor>(*this, p, opts);
	}

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

	std::filesystem::path base_path() const override {
		return this->base_;
	}

	std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return this->confine_(StdFs::canonical(p));
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		if(p.empty()) {
			return p;
		}

		return this->confine_(StdFs::weakly_canonical(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) const& override {
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<ChRootedStdFs>(this->base_, std::move(c), this->temp_dir_);
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override {
		// TODO: move
		auto c = this->canonical(p);
		if(!this->is_directory(c)) {
			throw std::filesystem::filesystem_error("", c, std::make_error_code(std::errc::not_a_directory));
		}

		return std::make_shared<ChRootedStdFs>(this->base_, std::move(c), this->temp_dir_);
	}

	std::filesystem::path temp_directory_path() const override {
		if(this->temp_dir_.empty()) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::no_such_file_or_directory));
		} else {
			return this->temp_dir_;
		}
	}

   protected:
	std::filesystem::path normal_(std::filesystem::path const& p) const override;

	std::filesystem::path confine_(std::filesystem::path const& normal) const {
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
