#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>

#include "vfs/impl/file.hpp"

namespace vfs {
namespace impl {

class FileProxyBase {
   public:
	virtual ~FileProxyBase() = default;

	[[nodiscard]] virtual std::shared_ptr<File const> target() const& = 0;

	[[nodiscard]] virtual std::shared_ptr<File> target() & {
		return std::const_pointer_cast<File>(static_cast<FileProxyBase const*>(this)->target());
	}

	[[nodiscard]] virtual std::shared_ptr<File> target() && {
		return std::const_pointer_cast<File>(static_cast<FileProxyBase const*>(this)->target());
	}
};

template<std::derived_from<File> F, std::derived_from<F> Storage = F>
class FileProxy
    : public FileProxyBase
    , virtual public F {
   public:
	FileProxy(std::shared_ptr<Storage> target)
	    : target_(std::move(target)) { }

	[[nodiscard]] std::shared_ptr<File const> target() const& override {
		if(auto const* proxy = dynamic_cast<FileProxyBase const*>(this->target_.get()); proxy != nullptr) {
			return proxy->target();
		}

		return this->target_;
	}

	[[nodiscard]] std::shared_ptr<File> target() & override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return FileProxy::target();
		}
	}

	[[nodiscard]] std::shared_ptr<File> target() && override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			if(auto proxy = std::dynamic_pointer_cast<FileProxyBase>(std::move(this->target_)); proxy) {
				return std::move(*proxy).target();
			}

			return std::move(this->target_);
		}
	}

	[[nodiscard]] std::intmax_t owner() const override {
		return this->target_->owner();
	}

	void owner(std::intmax_t owner) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->owner(owner);
		}
	}

	[[nodiscard]] std::intmax_t group() const override {
		return this->target_->group();
	}

	void group(std::intmax_t group) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->group(group);
		}
	}

	[[nodiscard]] std::filesystem::perms perms() const override {
		return this->target_->perms();
	}

	void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->perms(prms, opts);
		}
	}

	bool operator==(File const& other) const override {
		if(auto const* proxy = dynamic_cast<FileProxyBase const*>(&other); proxy != nullptr) {
			return this->operator==(*proxy->target());
		}

		return this->target_->operator==(other);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return this->target_->last_write_time();
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->last_write_time(new_time);
		}
	}

   protected:
	std::shared_ptr<Storage> target_;
};

template<std::derived_from<Directory> Storage = Directory>
class DirectoryProxy: public FileProxy<Directory, Storage> {
   public:
	DirectoryProxy(std::shared_ptr<Storage> target)
	    : FileProxy<Directory, Storage>(std::move(target)) { }

	[[nodiscard]] bool empty() const override {
		return this->target_->empty();
	}

	[[nodiscard]] bool contains(std::string const& name) const override {
		return this->target_->contains(name);
	}

	// returns nullptr if not exists.
	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override {
		return this->target_->next(name);
	}

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->emplace_regular_file(name);
		}
	}

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->emplace_directory(name);
		}
	}

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->emplace_symlink(name, std::move(target));
		}
	}

	bool link(std::string const& name, std::shared_ptr<File> file) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->link(name, std::move(file));
		}
	}

	bool unlink(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->unlink(name);
		}
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->mount(name, std::move(file));
		}
	}

	void unmount(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->unmount(name);
		}
	}

	std::uintmax_t erase(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->erase(name);
		}
	}

	std::uintmax_t clear() override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->target_->clear();
		}
	}

	[[nodiscard]] std::shared_ptr<Directory::Cursor> cursor() const override {
		return this->target_->cursor();
	}
};
}  // namespace impl
}  // namespace vfs
