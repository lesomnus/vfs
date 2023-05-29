#include "vfs/impl/entry.hpp"

#include <filesystem>
#include <memory>
#include <system_error>
#include <unordered_set>
#include <utility>

#include "vfs/impl/file.hpp"
#include "vfs/impl/vfile.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

namespace {

template<std::derived_from<File> F>
inline std::shared_ptr<EntryTypeOf<F>> make_entry(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<F> file) {
	return EntryTypeOf<F>::make(std::move(name), std::move(prev), std::move(file));
}

template<>
inline std::shared_ptr<Entry> make_entry<File>(std::string name, std::shared_ptr<DirectoryEntry> prev, std::shared_ptr<File> file) {
	using fs::file_type;

	switch(file->type()) {
	case file_type::regular:
		return EntryTypeOf<RegularFile>::make(std::move(name), std::move(prev), std::dynamic_pointer_cast<RegularFile>(std::move(file)));
	case file_type::directory:
		return EntryTypeOf<Directory>::make(std::move(name), std::move(prev), std::dynamic_pointer_cast<Directory>(std::move(file)));
	case file_type::symlink:
		return EntryTypeOf<Symlink>::make(std::move(name), std::move(prev), std::dynamic_pointer_cast<Symlink>(std::move(file)));

	default:
		throw std::logic_error("unexpected type of file");
	}
}

template<std::derived_from<File> F>
inline std::shared_ptr<Entry> make_entry(std::string name, std::shared_ptr<DirectoryEntry const> prev, std::shared_ptr<F> file) {
	return make_entry(std::move(name), std::const_pointer_cast<DirectoryEntry>(std::move(prev)), std::move(file));
}

}  // namespace

std::shared_ptr<DirectoryEntry const> Entry::prev() const {
	if(this->prev_) {
		return this->prev_;
	} else {
		throw fs::filesystem_error("file not mounted", "", std::make_error_code(std::errc::bad_file_descriptor));
	}
}

std::shared_ptr<DirectoryEntry const> Entry::root() const {
	auto curr = this->prev();
	do {
		if(curr->prev_) {
			curr = curr->prev_;
		} else {
			return curr;
		}
	} while(true);
}

std::filesystem::path Entry::path() const {
	if(this->prev_) {
		return this->prev_->path() / this->name_;
	} else {
		return "/";
	}
}

std::shared_ptr<DirectoryEntry> DirectoryEntry::make_root() {
	auto d = std::make_shared<VDirectory>(0, 0);
	return DirectoryEntry::make("/", nullptr, std::move(d));
}

std::shared_ptr<DirectoryEntry const> DirectoryEntry::prev() const {
	if(this->prev_) {
		return this->prev_;
	} else {
		return std::static_pointer_cast<DirectoryEntry const>(this->shared_from_this());
	}
}

std::shared_ptr<Entry const> DirectoryEntry::next(std::string name) const {
	auto next = this->typed_file()->next(name);
	if(next == nullptr) {
		throw fs::filesystem_error("", this->path(), name, std::make_error_code(std::errc::no_such_file_or_directory));
	}

	return make_entry(std::move(name), this->shared_from_this()->must_be<DirectoryEntry>(), std::move(next));
}

void navigate(
    std::shared_ptr<Entry const>& entry,
    fs::path::const_iterator&     first,
    fs::path::const_iterator      last) {
	do {
		if(first == last) {
			break;
		}

		if(!first->is_absolute()) {
			break;
		}

		++first;
		entry = entry->root();
	} while(true);

	for(; first != last; ++first) {
		if(auto l = std::dynamic_pointer_cast<SymlinkEntry const>(entry); l) {
			entry = l->follow_chain();
		}

		auto d = std::dynamic_pointer_cast<DirectoryEntry const>(entry);
		if(!d) {
			throw fs::filesystem_error("", entry->path(), std::make_error_code(std::errc::not_a_directory));
		}

		auto const& name = *first;
		if(name == "." || name.empty()) {
			continue;
		}
		if(name == "..") {
			entry = entry->prev();
			continue;
		}

		entry = d->next(name);
	}
}

std::pair<std::shared_ptr<Entry const>, fs::path::const_iterator> DirectoryEntry::navigate(fs::path::const_iterator first, fs::path::const_iterator last, std::error_code& ec) const {
	auto rst = std::make_pair(this->shared_from_this(), first);
	try {
		impl::navigate(rst.first, rst.second, last);
		ec.clear();
	} catch(fs::filesystem_error const& err) {
		ec = err.code();
	}

	return rst;
}

std::shared_ptr<Entry const> DirectoryEntry::navigate(std::filesystem::path const& p) const {
	auto entry = this->shared_from_this();
	auto it    = p.begin();
	impl::navigate(entry, it, p.end());

	return entry;
}

void DirectoryEntry::insert(std::string const& name, std::shared_ptr<File> file) {
	auto const ok = this->typed_file()->insert(name, std::move(file));
	if(!ok) {
		throw fs::filesystem_error("", this->path(), name, std::make_error_code(std::errc::file_exists));
	}
}

std::shared_ptr<Entry> DirectoryEntry::next_insert(std::string name, std::shared_ptr<File> file) {
	this->insert(name, std::move(file));

	return impl::make_entry(std::move(name), std::static_pointer_cast<DirectoryEntry>(this->shared_from_this()), std::move(file));
}

std::shared_ptr<Storage> DirectoryEntry::resolve_storage() const {
	if(this->storage) {
		return this->storage;
	} else if(this->is_root()) {
		// Fallback storage.
		return std::make_shared<VStorage>();
	} else {
		return this->prev()->resolve_storage();
	}
}

std::shared_ptr<Entry const> SymlinkEntry::follow() const {
	try {
		return this->prev()->navigate(this->typed_file()->target());
	} catch(fs::filesystem_error const& err) {
		throw fs::filesystem_error(this->path().string() + " -> ", err.path1(), err.path2(), err.code());
	}
}

std::shared_ptr<Entry const> SymlinkEntry::follow_chain() const {
	std::unordered_set<std::shared_ptr<File const>> visited;

	auto f = this->shared_from_this();
	do {
		auto l = std::dynamic_pointer_cast<SymlinkEntry const>(f);
		if(!l) {
			return f;
		}

		auto const& [_, ok] = visited.insert(l->file_);
		if(!ok) {
			throw fs::filesystem_error("circular symlinks", this->path(), std::make_error_code(std::errc::too_many_symbolic_link_levels));
		}

		f = l->follow();
	} while(true);
}

}  // namespace impl
}  // namespace vfs
