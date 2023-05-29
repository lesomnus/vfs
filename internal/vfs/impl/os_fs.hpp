#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

#include "vfs/impl/utils.hpp"

#include "vfs/directory_entry.hpp"
#include "vfs/fs.hpp"

namespace vfs {
namespace impl {

class OsFs: public Fs {
   public:
	OsFs(std::filesystem::path const& cwd)
	    : cwd_(std::filesystem::canonical(cwd / "")) { }

	std::shared_ptr<std::istream> open_read(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::in) const override {
		return std::make_shared<std::ifstream>(this->normal_(filename), mode);
	}

	std::shared_ptr<std::ostream> open_write(std::filesystem::path const& filename, std::ios_base::openmode mode = std::ios_base::out) override {
		return std::make_shared<std::ofstream>(this->normal_(filename), mode);
	}

	std::shared_ptr<Fs const> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const override {
		throw std::runtime_error("not implemented");
	}

	std::shared_ptr<Fs> change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) override {
		throw std::runtime_error("not implemented");
	}

	std::filesystem::path canonical(std::filesystem::path const& p) const override {
		return std::filesystem::canonical(this->normal_(p));
	}

	std::filesystem::path canonical(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::canonical(this->normal_(p), ec);
	}

	std::filesystem::path weakly_canonical(std::filesystem::path const& p) const override {
		if(p.empty()) {
			return p;
		} else if(p.is_relative() && !std::filesystem::exists(this->cwd_ / *p.begin())) {
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
		return std::make_shared<OsFs>(this->normal_(p));
	}

	std::shared_ptr<Fs> current_path(std::filesystem::path const& p) && override {
		return std::make_shared<OsFs>(this->normal_(p));
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
		// return std::filesystem::equivalent(this->normal_(p1), this->normal_(p2), ec);
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
		return std::filesystem::temp_directory_path(ec);
	}

	bool is_empty(std::filesystem::path const& p) const override {
		return std::filesystem::is_empty(this->normal_(p));
	}

	bool is_empty(std::filesystem::path const& p, std::error_code& ec) const override {
		return std::filesystem::is_empty(this->normal_(p), ec);
	}

   protected:
	virtual std::filesystem::path normal_(std::filesystem::path const& p) const {
		return p.is_absolute() ? p : (this->cwd_ / p);
	}

	template<typename C, typename It>
	class BasicCursor: public C {
	   public:
		BasicCursor(OsFs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
		    : it_(fs.absolute(p), opts)
		    , entry_(fs) {
			if(!this->at_end()) {
				this->entry_.assign(this->it_->path());
			}
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
				this->entry_.assign(this->it_->path());
			}
		}

		It                                 it_;
		std::filesystem::directory_options opts_;

		directory_entry entry_;
	};

	using Cursor = BasicCursor<Fs::Cursor, std::filesystem::directory_iterator>;

	class RecursiveCursor: public BasicCursor<Fs::RecursiveCursor, std::filesystem::recursive_directory_iterator> {
	   public:
		RecursiveCursor(OsFs const& fs, std::filesystem::path const& p, std::filesystem::directory_options opts)
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
		return std::make_shared<OsFs::Cursor>(*this, p, opts);
	}

	std::shared_ptr<Fs::RecursiveCursor> recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts) const override {
		return std::make_shared<OsFs::RecursiveCursor>(*this, p, opts);
	}

   private:
	std::filesystem::path cwd_;
};

}  // namespace impl
}  // namespace vfs
