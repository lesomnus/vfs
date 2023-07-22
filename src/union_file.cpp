#include "vfs/impl/union_file.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

namespace {

std::uintmax_t count_files_(UnionDirectory::Context& context, File const& file) {
	auto const* d = dynamic_cast<Directory const*>(&file);
	if(d == nullptr) {
		return 1;
	}

	std::uintmax_t cnt = 1;
	for(auto const& [name, f]: *d) {
		if(context.hidden.contains(name)) {
			continue;
		}

		auto const& ctx = context.at(name);
		cnt += count_files_(*ctx, *f);
	}

	return cnt;
}

class Anchor_ {
   public:
	Anchor_(std::shared_ptr<Directory> upper, std::filesystem::path crumbs)
	    : upper_(std::move(upper))
	    , crumbs_(std::move(crumbs)) { }

	Anchor_(std::shared_ptr<Directory> upper)
	    : upper_(std::move(upper)) { }

	[[nodiscard]] std::shared_ptr<Directory> const& target() const {
		return this->upper_;
	}

	[[nodiscard]] Anchor_ next(std::string const& name) const {
		return {this->upper_, this->crumbs_ / name};
	}

	// Creates directories on `upper` by following `crumbs`.
	std::shared_ptr<Directory> const& pull() {
		if(this->crumbs_.empty()) {
			return this->upper_;
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
		return this->upper_;
	}

   private:
	std::shared_ptr<Directory> upper_;
	std::filesystem::path      crumbs_;
};

class RegularFileOnLower_: public FileProxy<RegularFile> {
   public:
	RegularFileOnLower_(std::string name, std::shared_ptr<RegularFile> target, Anchor_ anchor)
	    : FileProxy<RegularFile>(std::move(target))
	    , name_(std::move(name))
	    , anchor_(std::move(anchor)) { }

	[[nodiscard]] std::uintmax_t size() const override {
		return this->target_->size();
	}

	void resize(std::uintmax_t new_size) override {
		this->pull_(std::ios_base::app)->resize(new_size);
	}

	[[nodiscard]] std::shared_ptr<std::istream> open_read(std::ios_base::openmode mode) const override {
		return this->target_->open_read(mode);
	}

	std::shared_ptr<std::ostream> open_write(std::ios_base::openmode mode) override {
		return this->pull_(mode)->open_write(mode);
	}

   private:
	std::shared_ptr<RegularFile>& pull_(std::ios_base::openmode mode) {
		if(!this->anchor_.has_value()) {
			return this->target_;
		}

		auto [target, _] = this->anchor_->pull()->emplace_regular_file(this->name_);
		this->anchor_.reset();

		if((mode & std::ios_base::app) == std::ios_base::app) {
			// Copy content from lower.
			target->copy_from(*this->target_);
		}

		this->target_ = std::move(target);
		return this->target_;
	}

	std::string name_;

	std::optional<Anchor_> anchor_;
};

class Branch_: virtual public Directory {
   public:
	Branch_(std::shared_ptr<UnionDirectory::Context> context)
	    : context_(std::move(context)) {
		assert(nullptr != this->context_);
	}

	[[nodiscard]] std::shared_ptr<UnionDirectory::Context> const& context() const {
		return this->context_;
	}

   protected:
	std::shared_ptr<UnionDirectory::Context> context_;
};

class SupBranch_
    : public Branch_
    , public DirectoryProxy<Directory> {
   public:
	SupBranch_(std::shared_ptr<UnionDirectory::Context> context, std::shared_ptr<Directory> upper)
	    : Branch_(std::move(context))
	    , DirectoryProxy<Directory>(std::move(upper)) { }

	using Branch_::type;

	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override {
		auto next_f = this->target_->next(name);
		if(!next_f) {
			return nullptr;
		}

		auto next_d = std::dynamic_pointer_cast<Directory>(std::move(next_f));
		if(!next_d) {
			return next_f;
		}

		auto ctx = this->context_->at(name);
		return std::make_shared<SupBranch_>(std::move(ctx), std::move(next_d));
	}

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override {
		auto it = this->target_->emplace_directory(name);
		if(!it.first) {
			return it;
		}

		auto ctx = this->context_->at(name);
		return std::make_pair(std::make_shared<SupBranch_>(std::move(ctx), it.first), it.second);
	}

	bool unlink(std::string const& name) override {
		auto const ok = this->target_->unlink(name);
		if(ok) {
			this->context_->hidden.insert(name);
		}

		return ok;
	}

	std::uintmax_t erase(std::string const& name) override {
		auto const cnt = this->target_->erase(name);
		if(cnt > 0) {
			this->context_->hidden.insert(name);
		}

		return cnt;
	}

	std::uintmax_t clear() override {
		for(auto const& [name, _]: *this->target_) {
			this->context_->hidden.insert(name);
		}

		return this->target_->clear();
	}

	[[nodiscard]] std::shared_ptr<Cursor> cursor() const override {
		std::unordered_map<std::string, std::shared_ptr<File>> files;
		for(auto const& [name, next_f]: *this->target_) {
			auto next_f_ = next_f;
			if(auto next_d = std::dynamic_pointer_cast<Directory>(std::move(next_f_)); next_d) {
				auto ctx = this->context_->at(name);
				next_f_  = std::make_shared<SupBranch_>(std::move(ctx), std::move(next_d));
			}

			files.insert(std::make_pair(name, std::move(next_f_)));
		}

		return std::make_shared<StaticCursor>(std::move(files));
	}
};

class SubBranch_
    : public Branch_
    , public DirectoryProxy<Directory const> {
   public:
	SubBranch_(std::shared_ptr<UnionDirectory::Context> context, std::shared_ptr<Directory const> lower, Anchor_ anchor)
	    : Branch_(std::move(context))
	    , DirectoryProxy(std::move(lower))
	    , anchor_(std::move(anchor)) {
		assert(nullptr != this->anchor_.target());
	}

	using Branch_::type;

	[[nodiscard]] bool empty() const override {
		if(this->target_->empty()) {
			return true;
		}

		for(auto const& [name, _]: *this->target_) {
			if(this->context_->hidden.contains(name)) {
				continue;
			}

			return false;
		}

		return true;
	}

	[[nodiscard]] bool contains(std::string const& name) const override {
		if(this->context_->hidden.contains(name)) {
			return false;
		}

		return this->target_->contains(name);
	}

	[[nodiscard]] std::shared_ptr<File> next(std::string const& name) const override {
		if(this->context_->hidden.contains(name)) {
			return nullptr;
		}

		auto next_f = this->target_->next(name);
		if(!next_f) {
			return nullptr;
		}
		if(auto next_d = std::dynamic_pointer_cast<Directory>(std::move(next_f)); next_d) {
			auto ctx = this->context_->at(name);
			return std::make_shared<SubBranch_>(std::move(ctx), this->target_, this->anchor_.next(name));
		}

		auto next_r = std::dynamic_pointer_cast<RegularFile>(std::move(next_f));
		if(!next_r) {
			return next_f;
		}

		return std::make_shared<RegularFileOnLower_>(name, std::move(next_r), this->anchor_);
	}

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override {
		if(auto next = this->target_->next(name); next) {
			return std::make_pair(std::dynamic_pointer_cast<RegularFile>(std::move(next)), false);
		}

		return this->anchor_.pull()->emplace_regular_file(name);
	}

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override {
		if(auto next = this->target_->next(name); next) {
			return std::make_pair(std::dynamic_pointer_cast<Directory>(std::move(next)), false);
		}

		return this->anchor_.pull()->emplace_directory(name);
	}

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override {
		if(auto next = this->target_->next(name); next) {
			return std::make_pair(std::dynamic_pointer_cast<Symlink>(std::move(next)), false);
		}

		return this->anchor_.pull()->emplace_symlink(name, target);
	}

	bool unlink(std::string const& name) override {
		if(this->context_->hidden.contains(name)) {
			return false;
		}
		if(!this->target_->contains(name)) {
			return false;
		}

		auto const [_, ok] = this->context_->hidden.insert(name);
		assert(ok);
		return true;
	}

	void mount(std::string const& name, std::shared_ptr<File> file) override {
		this->anchor_.pull()->mount(name, std::move(file));
	}

	std::uintmax_t erase(std::string const& name) override {
		if(this->context_->hidden.contains(name)) {
			return 0;
		}

		auto next_f = this->target_->next(name);
		if(!next_f) {
			return 0;
		}

		auto const [_, ok] = this->context_->hidden.insert(name);
		assert(ok);

		auto const ctx = this->context_->at(name);
		auto const cnt = count_files_(*ctx, *next_f);
		return cnt;
	}

	std::uintmax_t clear() override {
		std::uintmax_t cnt = 0;
		for(auto const& [name, next_f]: *this->target_) {
			auto [_, ok] = this->context_->hidden.insert(name);
			if(!ok) {
				continue;
			}

			auto const ctx = this->context_->at(name);
			cnt += count_files_(*ctx, *next_f);
		}

		return cnt;
	}

	[[nodiscard]] std::shared_ptr<Cursor> cursor() const override {
		std::unordered_map<std::string, std::shared_ptr<File>> files;
		for(auto it: *this->target_) {
			if(this->context_->hidden.contains(it.first)) {
				continue;
			}

			files.insert(it);
		}

		return std::make_shared<Directory::StaticCursor>(std::move(files));
	}

	[[nodiscard]] Anchor_ const& anchor() const {
		return this->anchor_;
	}

	[[nodiscard]] std::shared_ptr<Directory const> lower() const& {
		return this->target_;
	}

	[[nodiscard]] std::shared_ptr<Directory const> lower() && {
		return std::move(this->target_);
	}

   private:
	Anchor_ anchor_;
};

class SubBranchHolder: public DirectoryProxy<Directory> {
   public:
	using DirectoryProxy::DirectoryProxy;

	void mount(std::string const& name, std::shared_ptr<File> file) override {
		this->target_->mount(name, std::move(file));
		this->upgrade();
	}

	std::pair<std::shared_ptr<RegularFile>, bool> emplace_regular_file(std::string const& name) override {
		auto it = this->target_->emplace_regular_file(name);
		if(!it.second) {
			return it;
		}

		this->upgrade();

		return it;
	}

	std::pair<std::shared_ptr<Directory>, bool> emplace_directory(std::string const& name) override {
		auto it = this->target_->emplace_directory(name);
		if(!it.second) {
			return it;
		}

		this->upgrade();

		return it;
	}

	std::pair<std::shared_ptr<Symlink>, bool> emplace_symlink(std::string const& name, std::filesystem::path target) override {
		auto it = this->target_->emplace_symlink(name, std::move(target));
		if(!it.second) {
			return it;
		}

		this->upgrade();

		return it;
	}

   private:
	void upgrade() {
		auto sub = std::dynamic_pointer_cast<SubBranch_>(this->target_);
		if(!sub) {
			return;
		}

		this->target_ = std::make_shared<UnionDirectory>(sub->context(), sub->anchor().target(), std::move(*sub).lower());
	}
};

std::shared_ptr<Directory> make_sup_branch_(std::shared_ptr<UnionDirectory::Context> context, std::shared_ptr<Directory> upper) {
	return std::static_pointer_cast<Directory>(std::make_shared<SupBranch_>(std::move(context), std::move(upper)));
}

std::shared_ptr<Directory> make_sub_branch_(std::shared_ptr<UnionDirectory::Context> context, std::shared_ptr<Directory const> lower, Anchor_ anchor) {
	return std::make_shared<SubBranchHolder>(std::make_shared<SubBranch_>(std::move(context), std::move(lower), std::move(anchor)));
}

}  // namespace

UnionDirectory::UnionDirectory(std::shared_ptr<Directory> upper, std::shared_ptr<Directory const> lower)
    : FileProxy(std::move(upper))
    , lower_(std::move(lower))
    , context_(std::make_shared<Context>()) {
	assert(nullptr != this->target_);
	assert(nullptr != this->lower_);
}

UnionDirectory::UnionDirectory(std::shared_ptr<Context> context, std::shared_ptr<Directory> upper, std::shared_ptr<Directory const> lower)
    : FileProxy(std::move(upper))
    , context_(std::move(context))
    , lower_(std::move(lower)) {
	assert(nullptr != this->target_);
	assert(nullptr != this->context_);
	assert(nullptr != this->lower_);
}

bool UnionDirectory::empty() const {
	if(!this->target_->empty()) {
		return false;
	}

	for(auto cursor = this->lower_->cursor(); !cursor->at_end(); cursor->increment()) {
		if(!this->context_->hidden.contains(cursor->name())) {
			return false;
		}
	}

	return true;
}

bool UnionDirectory::contains(std::string const& name) const {
	if(this->target_->contains(name)) {
		return true;
	}
	if(this->context_->hidden.contains(name)) {
		return false;
	}

	return this->lower_->contains(name);
}

std::shared_ptr<File> UnionDirectory::next(std::string const& name) const {
	auto up_next   = this->target_->next(name);
	auto up_next_d = std::dynamic_pointer_cast<Directory>(std::move(up_next));
	if(up_next) {
		// (F-D):?
		return up_next;
	}

	// (0|D):?

	auto lo_next   = this->lower_next_(name);
	auto lo_next_d = std::dynamic_pointer_cast<Directory>(std::move(lo_next));
	if(up_next_d) {
		// D:?

		auto ctx = this->context_->at(name);
		if(!lo_next_d) {
			// D:!D
			return make_sup_branch_(std::move(ctx), std::move(up_next_d));
		}

		// D:D
		return std::make_shared<UnionDirectory>(std::move(ctx), std::move(up_next_d), std::move(lo_next_d));
	}

	// 0:?
	if(lo_next_d) {
		// 0:D
		return make_sub_branch_(this->context_->at(name), std::move(lo_next_d), Anchor_(this->target_, name));
	}

	// 0:!D
	if(!lo_next) {
		// 0:0
		return nullptr;
	}

	// 0:(F-D)
	auto lo_next_r = std::dynamic_pointer_cast<RegularFile>(std::move(lo_next));
	if(!lo_next_r) {
		// 0:(F-D-R)
		return lo_next;
	}

	// 0:R
	return std::make_shared<RegularFileOnLower_>(name, std::move(lo_next_r), Anchor_(this->target_));
}

std::pair<std::shared_ptr<RegularFile>, bool> UnionDirectory::emplace_regular_file(std::string const& name) {
	if(auto up_next = this->target_->next(name); up_next) {
		// F:?
		return std::make_pair(std::dynamic_pointer_cast<RegularFile>(std::move(up_next)), false);
	}

	// 0:?
	auto lo_next = this->lower_next_(name);
	if(!lo_next) {
		// 0:0
		return this->target_->emplace_regular_file(name);
	}

	// 0:F
	auto lo_next_r = std::dynamic_pointer_cast<RegularFile>(std::move(lo_next));
	if(!lo_next_r) {
		// 0:(F-R)
		return std::make_pair(nullptr, false);
	}

	// 0:R
	return std::make_pair(std::make_shared<RegularFileOnLower_>(name, std::move(lo_next_r), Anchor_(this->target_)), false);
}

std::pair<std::shared_ptr<Directory>, bool> UnionDirectory::emplace_directory(std::string const& name) {
	auto lo_next = this->lower_next_(name);
	if(!lo_next) {
		// ?:0
		auto it = this->target_->emplace_directory(name);
		if(!it.first) {
			return it;
		}

		auto& [up_next_d, ok] = it;
		return std::make_pair(make_sup_branch_(this->context_->at(name), std::move(up_next_d)), ok);
	}

	// ?:F

	auto up_next = this->target_->next(name);
	if(!up_next) {
		// 0:F
		auto lo_next_d = std::dynamic_pointer_cast<Directory>(std::move(lo_next));
		if(!lo_next_d) {
			// 0:(F-D)
			return std::make_pair(nullptr, false);
		}

		// 0:D
		return std::make_pair(make_sub_branch_(this->context_->at(name), std::move(lo_next_d), Anchor_{this->target_, name}), false);
	}

	// F:F

	auto up_next_d = std::dynamic_pointer_cast<Directory>(std::move(up_next));
	if(!up_next_d) {
		// (F-D):F
		return std::make_pair(nullptr, false);
	}

	// D:F

	auto lo_next_d = std::dynamic_pointer_cast<Directory>(std::move(lo_next));
	if(!lo_next_d) {
		// D:(F-D)
		return std::make_pair(make_sup_branch_(this->context_->at(name), std::move(up_next_d)), false);
	}

	// D:D
	return std::make_pair(std::make_shared<UnionDirectory>(this->context_->at(name), std::move(up_next_d), std::move(lo_next_d)), false);
}

std::pair<std::shared_ptr<Symlink>, bool> UnionDirectory::emplace_symlink(std::string const& name, std::filesystem::path target) {
	if(auto up_next = this->target_->next(name); up_next) {
		// F:?
		return std::make_pair(std::dynamic_pointer_cast<Symlink>(std::move(up_next)), false);
	}

	// 0:?
	auto lo_next = this->lower_next_(name);
	if(!lo_next) {
		// 0:0
		return this->target_->emplace_symlink(name, std::move(target));
	}

	// 0:F
	return std::make_pair(std::dynamic_pointer_cast<Symlink>(std::move(lo_next)), false);
}

bool UnionDirectory::link(std::string const& name, std::shared_ptr<File> file) {
	auto up_next = this->target_->link(name, file);
	if(up_next) {
		return true;
	}

	auto lo_next = this->lower_next_(name);
	if(lo_next) {
		return false;
	}

	return this->target_->link(name, std::move(file));
}

bool UnionDirectory::unlink(std::string const& name) {
	if(this->target_->unlink(name)) {
		this->context_->hidden.insert(name);
		return true;
	}

	if(this->context_->hidden.contains(name)) {
		return false;
	}
	if(!this->lower_->contains(name)) {
		return false;
	}

	auto const [_, ok] = this->context_->hidden.insert(name);
	return ok;
}

std::uintmax_t UnionDirectory::erase(std::string const& name) {
	auto& hidden = this->context_->hidden;

	if(auto const cnt = this->target_->erase(name); cnt > 0) {
		hidden.insert(name);
		return cnt;
	}

	if(hidden.contains(name)) {
		return 0;
	}
	if(!this->lower_->contains(name)) {
		return 0;
	}

	auto const [_, ok] = hidden.insert(name);
	assert(ok);

	auto const cnt = count_files_(*this->context_, *this->lower_);
	return cnt;
}

std::uintmax_t UnionDirectory::clear() {
	auto& hidden = this->context_->hidden;

	for(auto const& [name, _]: *this->target_) {
		hidden.insert(name);
	}

	std::size_t cnt = 0;
	for(auto const& [name, file]: *this->lower_) {
		auto const [_, ok] = hidden.insert(name);
		if(!ok) {
			continue;
		}

		cnt += count_files_(*this->context_->at(name), *file);
	}

	cnt += this->target_->clear();
	return cnt;
}

std::shared_ptr<Directory::Cursor> UnionDirectory::cursor() const {
	std::unordered_map<std::string, std::shared_ptr<File>> files;
	for(auto const& [name, next_f]: *this->target_) {
		if(next_f) {
			files.insert(std::make_pair(name, next_f));
		}
	}

	for(auto const& [name, next_f]: *this->lower_) {
		if(files.contains(name)) {
			continue;
		}
		if(next_f) {
			files.insert(std::make_pair(name, next_f));
		}
	}

	return std::make_shared<StaticCursor>(std::move(files));
}

std::shared_ptr<File> UnionDirectory::lower_next_(std::string const& name) const {
	if(this->context_->hidden.contains(name)) {
		return nullptr;
	}

	return this->lower_->next(name);
}

std::shared_ptr<UnionDirectory::Context> UnionDirectory::Context::at(std::string const& name) {
	auto& child_ctx = this->child_context;

	auto it = child_ctx.find(name);
	if(it != child_ctx.end()) {
		return it->second;
	}

	auto ctx = std::make_shared<Context>();
	child_ctx.insert(std::make_pair(name, ctx));
	return ctx;
}

}  // namespace impl
}  // namespace vfs
