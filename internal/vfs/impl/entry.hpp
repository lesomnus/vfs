#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/utils.hpp"

namespace vfs {
namespace impl {

class DirectoryEntry;

class Entry: public std::enable_shared_from_this<Entry> {
   public:
	virtual ~Entry() = default;

	std::string const& name() const {
		return this->name_;
	}

	[[nodiscard]] bool holds(File const& file) const {
		return *this->file() == file;
	}

	[[nodiscard]] bool holds_same_file_with(Entry const& entry) const {
		return this->holds(*entry.file());
	}

	[[nodiscard]] virtual std::shared_ptr<File const> file() const = 0;
	[[nodiscard]] virtual std::shared_ptr<File>       file()       = 0;

	[[nodiscard]] virtual std::shared_ptr<DirectoryEntry const> prev() const;

	[[nodiscard]] std::shared_ptr<DirectoryEntry> prev() {
		return std::const_pointer_cast<DirectoryEntry>(static_cast<Entry const*>(this)->prev());
	}

	[[nodiscard]] std::shared_ptr<DirectoryEntry const> root() const;

	[[nodiscard]] std::shared_ptr<DirectoryEntry> root() {
		return std::const_pointer_cast<DirectoryEntry>(static_cast<Entry const*>(this)->root());
	}

	[[nodiscard]] std::filesystem::path path() const;

	template<std::derived_from<Entry> E>
	std::shared_ptr<E const> must_be() const {
		auto entry = std::dynamic_pointer_cast<E const>(this->shared_from_this());
		if(!entry) [[unlikely]] {
			auto const p  = this->path();
			auto const te = to_string(this->file()->type());
			auto const ta = to_string(E::FileType::Type);
			throw std::logic_error("expect " + p.string() + " to be type of " + te + " but was " + ta);
		}
		return entry;
	}

	template<std::derived_from<Entry> E>
	std::shared_ptr<E> must_be() {
		return std::const_pointer_cast<E>(static_cast<Entry const*>(this)->must_be<E>());
	}

	[[nodiscard]] virtual std::shared_ptr<Entry const> follow() const {
		return this->shared_from_this();
	}

	[[nodiscard]] std::shared_ptr<Entry> follow() {
		return std::const_pointer_cast<Entry>(static_cast<Entry const*>(this)->follow());
	}

	[[nodiscard]] virtual std::shared_ptr<Entry const> follow_chain() const {
		return this->shared_from_this();
	}

	[[nodiscard]] std::shared_ptr<Entry> follow_chain() {
		return std::const_pointer_cast<Entry>(static_cast<Entry const*>(this)->follow_chain());
	}

   private:
	std::string name_;

   protected:
	Entry(std::string name, std::shared_ptr<DirectoryEntry> prev)
	    : name_(std::move(name))
	    , prev_(std::move(prev)) { }

	std::shared_ptr<DirectoryEntry> prev_;
};

template<std::derived_from<File> F>
class TypedEntry: public Entry {
   public:
	using FileType = F;

	[[nodiscard]] std::shared_ptr<File const> file() const override {
		return std::static_pointer_cast<File const>(this->file_);
	}

	[[nodiscard]] std::shared_ptr<File> file() override {
		return std::static_pointer_cast<File>(this->file_);
	}

	[[nodiscard]] std::shared_ptr<F const> typed_file() const {
		return std::static_pointer_cast<F const>(this->file_);
	}

	[[nodiscard]] std::shared_ptr<F> typed_file() {
		return std::static_pointer_cast<F>(this->file_);
	}

   protected:
	TypedEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<F> file)
	    : Entry(std::move(name), std::move(prev))
	    , file_(std::move(file)) { }

	using Entry::follow;
	using Entry::follow_chain;

	std::shared_ptr<F> file_;
};

class RegularFileEntry: public TypedEntry<RegularFile> {
   public:
	RegularFileEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<RegularFile> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }
};

class SymlinkEntry: public TypedEntry<Symlink> {
   public:
	SymlinkEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Symlink> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }

	[[nodiscard]] std::shared_ptr<Entry const> follow() const override;

	[[nodiscard]] std::shared_ptr<Entry> follow() {
		return std::const_pointer_cast<Entry>(static_cast<SymlinkEntry const*>(this)->follow());
	}

	[[nodiscard]] std::shared_ptr<Entry const> follow_chain() const override;

	[[nodiscard]] std::shared_ptr<Entry> follow_chain() {
		return std::const_pointer_cast<Entry>(static_cast<SymlinkEntry const*>(this)->follow_chain());
	}
};

class DirectoryEntry: public TypedEntry<Directory> {
   public:
	DirectoryEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Directory> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }

	static std::shared_ptr<DirectoryEntry> make_root();

	using Entry::prev;

	[[nodiscard]] std::shared_ptr<DirectoryEntry const> prev() const override;

	[[nodiscard]] bool is_root() const {
		return this->prev_ == nullptr;
	}

	[[nodiscard]] std::shared_ptr<Entry const> next(std::string const& name) const;

	[[nodiscard]] std::shared_ptr<Entry> next(std::string const& name) {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->next(name));
	}

	[[nodiscard]] std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) const;

	[[nodiscard]] std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) {
		auto [entry, it] = static_cast<DirectoryEntry const*>(this)->navigate(first, last, ec);
		return std::make_pair(std::const_pointer_cast<Entry>(std::move(entry)), it);
	}

	[[nodiscard]] std::shared_ptr<Entry const> navigate(std::filesystem::path const& p) const;

	[[nodiscard]] std::shared_ptr<Entry> navigate(std::filesystem::path const& p) {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->navigate(p));
	}

	[[nodiscard]] std::shared_ptr<Entry const> navigate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->navigate(p.begin(), p.end(), ec).first;
	}

	[[nodiscard]] std::shared_ptr<Entry> navigate(std::filesystem::path& p, std::error_code& ec) const {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->navigate(p, ec));
	}

	std::shared_ptr<RegularFile> emplace_regular_file(std::string const& name);

	std::shared_ptr<Directory> emplace_directory(std::string const& name);

	std::shared_ptr<Symlink> emplace_symlink(std::string const& name, std::string target);
};

class UnknownTypeEntry: public Entry {
   public:
	UnknownTypeEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<File> file)
	    : Entry(std::move(name), std::move(prev))
	    , file_(std::move(file)) { }

	[[nodiscard]] std::shared_ptr<File const> file() const override {
		return this->file_;
	}

	[[nodiscard]] std::shared_ptr<File> file() override {
		return this->file_;
	}

   protected:
	using Entry::follow;
	using Entry::follow_chain;

	std::shared_ptr<File> file_;
};

}  // namespace impl
}  // namespace vfs
