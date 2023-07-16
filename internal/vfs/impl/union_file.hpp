#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/fs.hpp"
#include "vfs/impl/vfile.hpp"

namespace vfs {
namespace impl {

class UnionDirectory: public FileProxy<Directory> {
   public:
	struct Context {
		// Always return a value; create new one if one does not exist.
		[[nodiscard]] std::shared_ptr<Context> at(std::string const& name);

		std::unordered_map<std::string, std::shared_ptr<Context>> child_context;

		std::unordered_set<std::string> hidden;
	};

	class Anchor {
	   public:
		Anchor(std::shared_ptr<Directory> upper, std::filesystem::path crumbs)
		    : upper_(std::move(upper))
		    , crumbs_(std::move(crumbs)) { }

		Anchor(std::shared_ptr<Directory> upper)
		    : upper_(std::move(upper)) { }

		[[nodiscard]] std::shared_ptr<Directory> const& target() const {
			return this->upper_;
		}

		[[nodiscard]] Anchor next(std::string const& name) const {
			return {this->upper_, this->crumbs_ / name};
		}

		// Creates directories on `upper` by following `crumbs`.
		void pull() {
			if(this->crumbs_.empty()) {
				return;
			}

			std::shared_ptr<Directory> d = std::move(this->upper_);
			for(auto const& name: this->crumbs_) {
				auto [next_d, _] = d->emplace_directory(name);
				if(!next_d) {
					throw std::runtime_error("file is created while navigating?");
				}

				d = std::move(next_d);
			}

			this->upper_ = std::move(d);
		}

	   private:
		std::shared_ptr<Directory> upper_;
		std::filesystem::path      crumbs_;
	};

	UnionDirectory(std::shared_ptr<Directory> upper, std::shared_ptr<Directory> lower);
	UnionDirectory(std::shared_ptr<Context> context, std::shared_ptr<Directory> upper, std::shared_ptr<Directory> lower);

	UnionDirectory(UnionDirectory const& other) = default;
	UnionDirectory(UnionDirectory&& other)      = default;

	[[nodiscard]] bool empty() const override;

	[[nodiscard]] bool contains(std::string const& name) const override;

	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override;

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

	bool link(std::string const& name, std::shared_ptr<File> file) override;

	bool unlink(std::string const& name) override;

	void mount(std::string const& name, std::shared_ptr<File> file) override { }

	void unmount(std::string const& name) override { }

	std::uintmax_t erase(std::string const& name) override;

	std::uintmax_t clear() override;

	[[nodiscard]] std::shared_ptr<Cursor> cursor() const override;

	class SupBranch: public FileProxy<Directory> {
	   public:
		SupBranch(std::shared_ptr<Context> context, std::shared_ptr<Directory> upper);

		SupBranch(SupBranch const& other) = default;
		SupBranch(SupBranch&& other)      = default;

		[[nodiscard]] bool empty() const override;

		[[nodiscard]] bool contains(std::string const& name) const override;

		[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override;

		std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

		std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

		std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

		bool link(std::string const& name, std::shared_ptr<File> file) override;

		bool unlink(std::string const& name) override;

		void mount(std::string const& name, std::shared_ptr<File> file) override { }

		void unmount(std::string const& name) override { }

		std::uintmax_t erase(std::string const& name) override;

		std::uintmax_t clear() override;

		[[nodiscard]] std::shared_ptr<Cursor> cursor() const override;

	   private:
		std::shared_ptr<Context> context_;
	};

	class SubBranch: public FileProxy<Directory> {
	   public:
		SubBranch(std::shared_ptr<Context> context, std::shared_ptr<Directory> lower, Anchor anchor);

		SubBranch(SubBranch const& other) = default;
		SubBranch(SubBranch&& other)      = default;

		[[nodiscard]] bool empty() const override;

		[[nodiscard]] bool contains(std::string const& name) const override;

		[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override;

		std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override;

		std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override;

		std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override;

		bool link(std::string const& name, std::shared_ptr<File> file) override;

		bool unlink(std::string const& name) override;

		void mount(std::string const& name, std::shared_ptr<File> file) override { }

		void unmount(std::string const& name) override { }

		std::uintmax_t erase(std::string const& name) override;

		std::uintmax_t clear() override;

		[[nodiscard]] std::shared_ptr<Cursor> cursor() const override;

	   private:
		std::shared_ptr<Context> context_;

		Anchor anchor_;
	};

   private:
	[[nodiscard]] std::shared_ptr<File> lower_next_(std::string const& name) const;

	std::shared_ptr<Context>   context_;
	std::shared_ptr<Directory> lower_;
};

}  // namespace impl
}  // namespace vfs
