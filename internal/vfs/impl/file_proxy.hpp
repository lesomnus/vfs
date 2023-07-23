#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>

#include "vfs/impl/file.hpp"

namespace vfs {
namespace impl {

class FileProxy: virtual public File {
   public:
	[[nodiscard]] virtual std::shared_ptr<File const> origin() const = 0;

	[[nodiscard]] virtual std::shared_ptr<File> origin() {
		return std::const_pointer_cast<File>(static_cast<FileProxy const*>(this)->origin());
	}
};

template<std::derived_from<File> F, std::derived_from<F> Storage = F>
class TypedFileProxy
    : public FileProxy
    , virtual public F {
   public:
	using TargetType = F;

	TypedFileProxy(std::shared_ptr<Storage> origin)
	    : origin_(std::move(origin)) { }

	[[nodiscard]] std::shared_ptr<File const> origin() const override {
		if(auto const* proxy = dynamic_cast<FileProxy const*>(this->origin_.get()); proxy != nullptr) {
			return proxy->origin();
		}

		return this->origin_;
	}

	[[nodiscard]] std::shared_ptr<File> origin() override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			if(auto* proxy = dynamic_cast<FileProxy*>(this->origin_.get()); proxy != nullptr) {
				return proxy->origin();
			}

			return this->origin_;
		}
	}

	[[nodiscard]] std::filesystem::perms perms() const override {
		return this->origin_->perms();
	}

	void perms(std::filesystem::perms prms, std::filesystem::perm_options opts) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->perms(prms, opts);
		}
	}

	bool operator==(File const& other) const override {
		if(auto const* proxy = dynamic_cast<FileProxy const*>(&other); proxy != nullptr) {
			return this->operator==(*proxy->origin());
		}

		return this->origin_->operator==(other);
	}

	[[nodiscard]] std::filesystem::file_time_type last_write_time() const override {
		return this->origin_->last_write_time();
	}

	void last_write_time(std::filesystem::file_time_type new_time) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->last_write_time(new_time);
		}
	}

   protected:
	std::shared_ptr<Storage> origin_;
};

template<std::derived_from<RegularFile> Storage = RegularFile>
class RegularFileProxy: public TypedFileProxy<RegularFile, Storage> {
   public:
	RegularFileProxy(std::shared_ptr<Storage> origin)
	    : TypedFileProxy<RegularFile, Storage>(std::move(origin)) { }

	[[nodiscard]] std::uintmax_t size() const override {
		return this->origin_->size();
	}

	void resize(std::uintmax_t new_size) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->resize(new_size);
		}
	}

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override {
		return this->origin_->open_read(mode);
	}

	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->open_write(mode);
		}
	}
};

template<std::derived_from<Directory> Storage = Directory>
class DirectoryProxy: public TypedFileProxy<Directory, Storage> {
   public:
	DirectoryProxy(std::shared_ptr<Storage> origin)
	    : TypedFileProxy<Directory, Storage>(std::move(origin)) { }

	[[nodiscard]] bool empty() const override {
		return this->origin_->empty();
	}

	[[nodiscard]] bool contains(std::string const& name) const override {
		return this->origin_->contains(name);
	}

	// returns nullptr if not exists.
	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override {
		return this->origin_->next(name);
	}

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->emplace_regular_file(name);
		}
	}

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->emplace_directory(name);
		}
	}

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path origin) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->emplace_symlink(name, std::move(origin));
		}
	}

	bool link(std::string const& name, std::shared_ptr<File> file) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->link(name, std::move(file));
		}
	}

	bool unlink(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->unlink(name);
		}
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->mount(name, std::move(file));
		}
	}

	void unmount(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->unmount(name);
		}
	}

	std::uintmax_t erase(std::string const& name) override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->erase(name);
		}
	}

	std::uintmax_t clear() override {
		if constexpr(std::is_const_v<Storage>) {
			throw std::filesystem::filesystem_error("", std::make_error_code(std::errc::read_only_file_system));
		} else {
			return this->origin_->clear();
		}
	}

	[[nodiscard]] std::shared_ptr<Directory::Cursor> cursor() const override {
		return this->origin_->cursor();
	}
};

}  // namespace impl
}  // namespace vfs
