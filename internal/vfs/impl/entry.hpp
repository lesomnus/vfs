#pragma once

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <system_error>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/storage.hpp"

namespace vfs {
namespace impl {

class DirectoryEntry;

class Entry: public std::enable_shared_from_this<Entry> {
   public:
	virtual ~Entry() = default;

	std::string const& name() const {
		return this->name_;
	}

	void name(std::string name) {
		this->name_ = std::move(name);
	}

	bool holds(std::shared_ptr<File> const& file) const {
		return this->file() == file;
	}

	bool holds_same_file_with(Entry const& other) const {
		return this->file() == other.file();
	}

	virtual std::shared_ptr<File const> file() const = 0;
	virtual std::shared_ptr<File>       file()       = 0;

	virtual std::shared_ptr<DirectoryEntry const> prev() const;

	std::shared_ptr<DirectoryEntry> prev() {
		return std::const_pointer_cast<DirectoryEntry>(static_cast<Entry const*>(this)->prev());
	}

	std::shared_ptr<DirectoryEntry const> root() const;

	std::shared_ptr<DirectoryEntry> root() {
		return std::const_pointer_cast<DirectoryEntry>(static_cast<Entry const*>(this)->root());
	}

	std::filesystem::path path() const;

	template<std::derived_from<Entry> E>
	std::shared_ptr<E const> must_be() const {
		auto entry = std::dynamic_pointer_cast<E const>(this->shared_from_this());
		if(!entry) [[unlikely]] {
			throw std::logic_error("TODO: descriptive");
		}
		return entry;
	}

	template<std::derived_from<Entry> E>
	std::shared_ptr<E> must_be() {
		return std::const_pointer_cast<E>(static_cast<Entry const*>(this)->must_be<E>());
	}

	virtual std::shared_ptr<Entry const> follow() const {
		return this->shared_from_this();
	}

	std::shared_ptr<Entry> follow() {
		return std::const_pointer_cast<Entry>(static_cast<Entry const*>(this)->follow());
	}

	virtual std::shared_ptr<Entry const> follow_chain() const {
		return this->shared_from_this();
	}

	std::shared_ptr<Entry> follow_chain() {
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
struct EntryTypeOfImpl {
	using Type = Entry;
};

template<std::derived_from<File> F>
using EntryTypeOf = typename EntryTypeOfImpl<F>::Type;

template<std::derived_from<File> F>
class TypedEntry: public Entry {
   public:
	using FileType = F;

	std::shared_ptr<File const> file() const override {
		return std::static_pointer_cast<File const>(this->file_);
	}

	std::shared_ptr<File> file() override {
		return std::static_pointer_cast<File>(this->file_);
	}

	std::shared_ptr<F const> typed_file() const {
		return std::static_pointer_cast<F const>(this->file_);
	}

	std::shared_ptr<F> typed_file() {
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
	static std::shared_ptr<RegularFileEntry> make(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<RegularFile> file) {
		return std::shared_ptr<RegularFileEntry>(new RegularFileEntry(std::move(name), std::move(prev), std::move(file)));
	}

   protected:
	RegularFileEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<RegularFile> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }
};

template<>
struct EntryTypeOfImpl<RegularFile> {
	using Type = RegularFileEntry;
};

class DirectoryEntry: public TypedEntry<Directory> {
   public:
	static std::shared_ptr<DirectoryEntry> make(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Directory> file) {
		return std::shared_ptr<DirectoryEntry>(new DirectoryEntry(std::move(name), std::move(prev), std::move(file)));
	}

	static std::shared_ptr<DirectoryEntry> make_root();

	std::shared_ptr<DirectoryEntry const> prev() const override;

	bool is_root() const {
		return this->prev_ == nullptr;
	}

	std::shared_ptr<Entry const> next(std::string name) const;

	std::shared_ptr<Entry> next(std::string name) {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->next(std::move(name)));
	}

	template<std::derived_from<Entry> E>
	std::shared_ptr<E const> next(std::string name) const {
		return this->next(std::move(name))->must_be<E>();
	}

	template<std::derived_from<Entry> E>
	std::shared_ptr<E> next(std::string name) {
		return this->next(std::move(name))->must_be<E>();
	}

	std::pair<std::shared_ptr<Entry const>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) const;

	std::pair<std::shared_ptr<Entry>, std::filesystem::path::const_iterator> navigate(
	    std::filesystem::path::const_iterator first,
	    std::filesystem::path::const_iterator last,
	    std::error_code&                      ec) {
		auto [entry, it] = static_cast<DirectoryEntry const*>(this)->navigate(first, last, ec);
		return std::make_pair(std::const_pointer_cast<Entry>(std::move(entry)), it);
	}

	std::shared_ptr<Entry const> navigate(std::filesystem::path const& p) const;

	std::shared_ptr<Entry> navigate(std::filesystem::path const& p) {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->navigate(p));
	}

	std::shared_ptr<Entry const> navigate(std::filesystem::path const& p, std::error_code& ec) const {
		return this->navigate(p.begin(), p.end(), ec).first;
	}

	std::shared_ptr<Entry> navigate(std::filesystem::path& p, std::error_code& ec) const {
		return std::const_pointer_cast<Entry>(static_cast<DirectoryEntry const*>(this)->navigate(p, ec));
	}

	void insert(std::string const& name, std::shared_ptr<File> file);

	std::shared_ptr<Entry> next_insert(std::string name, std::shared_ptr<File> file);

	template<std::derived_from<File> F>
	std::shared_ptr<EntryTypeOf<F>> next_insert(std::string name, std::shared_ptr<F> file) {
		return std::static_pointer_cast<EntryTypeOf<F>>(this->next_insert(std::move(name), std::static_pointer_cast<File>(std::move(file))));
	}

	std::shared_ptr<Storage> resolve_storage() const;

	std::shared_ptr<Storage> storage;

   protected:
	DirectoryEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Directory> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }
};

template<>
struct EntryTypeOfImpl<Directory> {
	using Type = DirectoryEntry;
};

class SymlinkEntry: public TypedEntry<Symlink> {
   public:
	static std::shared_ptr<SymlinkEntry> make(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Symlink> file) {
		return std::shared_ptr<SymlinkEntry>(new SymlinkEntry(std::move(name), std::move(prev), std::move(file)));
	}

	std::shared_ptr<Entry const> follow() const override;

	std::shared_ptr<Entry> follow() {
		return std::const_pointer_cast<Entry>(static_cast<SymlinkEntry const*>(this)->follow());
	}

	std::shared_ptr<Entry const> follow_chain() const override;

	std::shared_ptr<Entry> follow_chain() {
		return std::const_pointer_cast<Entry>(static_cast<SymlinkEntry const*>(this)->follow_chain());
	}

   protected:
	SymlinkEntry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<Symlink> file)
	    : TypedEntry(std::move(name), std::move(prev), std::move(file)) { }
};

template<>
struct EntryTypeOfImpl<Symlink> {
	using Type = SymlinkEntry;
};

}  // namespace impl
}  // namespace vfs
