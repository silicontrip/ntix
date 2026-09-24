#include "npath.hpp"
#include "NtixObjectLib.hpp"

namespace ntix {

	// oa_/uc_/elements_ intentionally omitted from the init list -- default member
	// initializers give fresh, uncomputed caches rather than the source's stale/self pointers.
	npath::npath(const npath& other) : path_(other.path_) {}
	npath::npath(npath&& other) noexcept : path_(std::move(other.path_)) {}
	npath& npath::operator= (const npath& other)
	{
		path_ = other.path_;
		oa_ = OBJECT_ATTRIBUTES{};
		uc_ = UNICODE_STRING{};
		elements_.clear();
		return *this;
	}

	npath& npath::operator= (npath&& other) noexcept
	{
		path_ = std::move(other.path_);
		oa_ = OBJECT_ATTRIBUTES{};
		uc_ = UNICODE_STRING{};
		elements_.clear();
		return *this;
	}

	const npath npath::normalise() const
	{
		if (!absolute())
			return *this;

		size_t n = size();
		for (size_t i = 2; i <= n; i++) {
			try {

				//std::cout << "ntix::npath::normalise subpath(0," << i << ") " << subpath(0,i) << std::endl;

				npath resolved = NtixObjectLib::get_instance()->get_symbolic_link_path(subpath(0, i));
				//std::cout << "ntix::npath::normalise resolved: " << resolved.str() << std::endl;

				npath remainder = subpath(i);
				//for (size_t j=0; j < size(); j++)
				//	std::cout << "ntix::npath::normalise subpath(" << j << "): " << subpath(j) << std::endl;

				npath joined = remainder.length() == 0 ? resolved : resolved.append_path(remainder);
				return trailing() ? joined.as_container().normalise() : joined.normalise();
			} catch (const nexception& e) {
				//std::cout << "ntix::npath::normalise nexception: " << e << std::endl;
				if (e.status() != STATUS_OBJECT_TYPE_MISMATCH)
					throw e;
				continue;  // this prefix isn't a symlink -- try one element deeper
			}
		}

		return *this;  // nothing in the path resolved as a symlink -- already normalised
	}

	std::vector<npath> expand_glob() {
		// If pattern has no wildcards, return as-is
		if (!this->nstr().is_glob())
		{
			return { this };
		}

		// Resolve to absolute and split into directory + pattern
		npath abs_pattern = resolve();
		npath dir = abs_pattern.parent();
		nstring glob_part = abs_pattern.basename().nstr();

		// List and filter
		std::vector<nstring> names;

		if (dir.type() == "File")
		{
			std::vector<file_directory_info>  entries = NtixFileLib::get_instance()->read_directory(dir);
			for (const file_directory_info& e : entries) {
				names.push_back(e.name);
			}
		} else {
			std::vector<directory_info> entried = NtixObjectLib::get_instance()->read_directory(dir);
		}
		std::vector<nstring> matches = nstring::glob_filter(names, glob_part);

		if (matches.empty()) {
			return {};  // No matches
		}

		// Reconstruct as absolute paths
		std::vector<npath> results;
		for (const auto& m : matches) {
			results.push_back(dir + npath(m));
		}

		// Convert back to relative if input was relative
		if (!pattern.absolute()) {
			npath pattern_dir = pattern.parent();  // e.g., "." or "subdir"
			npath cwd = npath(NtixCoreLib::get_instance()->resolve_path(nstring(".")));

			for (auto& r : results) {
				r = r.unresolve(pattern_dir, cwd);
			}
		}

		return results;
	}

	const npath npath::resolve() const
	{
		if (path_.front() == '\\')
			return *this;  // already OM-absolute -- ntix_resolve_path never normalizes this branch either

		std::wstring full = NtixCoreLib::get_instance()->resolve_path(path_.wc_str());
		return npath(nstring("\\??\\" + nstring(full).str())).normalise();
}

	const npath npath::append_path(const npath& path) const
	{
		if (path_.back() == '\\' && path.nstr().front() == '\\')
			return npath(path_ + path.nstr().substr(1));
		else if ( path_.back() != '\\' && path.nstr().front() != '\\')
			return npath(path_ + '\\' + path.nstr());
		else
			return npath(path_ + path.nstr());
	}

	// user path:  .\Directory
	// converted path: \Device\HarddiskVolume3\Users\example\Directory
	// this path: \Device\HarddiskVolume3\Users\example\Directory\subdir\file.txt

	// -> .\Directory\subdir\file.txt (unless there is a bug)

	const npath npath::unresolve(const npath& user, const npath& converted) const
	{
		// we should possible do checking that the head of this path matches converted
		return user.append_path(npath(path_.substr(converted.nstr().size())));
	}

	const std::string& npath::str() const { return path_.str(); }

	const nstring& npath::nstr() const { return path_; }

	OBJECT_ATTRIBUTES& npath::oa(ULONG oa_flags) const
	{
		if (uc_.Buffer == nullptr)
		{
			uc_ = path_.unicode_str();
			oa_.Length = sizeof(OBJECT_ATTRIBUTES);
			oa_.ObjectName = &uc_;
			oa_.RootDirectory = NULL;
			oa_.Attributes = oa_flags;
			oa_.SecurityDescriptor = NULL;
			oa_.SecurityQualityOfService = NULL;
		}
		return oa_;
	}

	bool npath::glob_match(const npath& match) const { return path_.glob_match(match.nstr()); }

	const std::vector<nstring>& npath::elements() const
	{
		if (elements_.size() > 0)
			return elements_;

		std::stringstream pathstream(path_.str());
		std::string segment;
		while(std::getline(pathstream, segment, '\\'))
		{
			elements_.push_back(nstring(segment));
		}
		// getline drops the empty token after a trailing '\' -- see trailing()
		return elements_;
	}

	const nstring& npath::at(size_t index) const { return elements().at(index); }

	const nstring& npath::operator[] (size_t index) const { return elements()[index]; }

	size_t npath::length() const { return path_.size(); }

	size_t npath::size() const { return elements().size(); }

	const npath npath::basename() const { return npath(elements().back()); }

	// \  = 0
	// \Device = 1
	// \Device\HarddiskVolume1 = 2
	// \Device\HarddiskVolume1\ = 2
	// size counts \ number except trailing

	// oh but wait, file system parents should end in a \ character
	// that includes paths like \Device\HarddiskVolume1\path the parent must be \Device\HarddiskVolume1\ and not \Device\HarddiskVolume1
	// Seriously NT what is wrong with you?

	// \Device\HarddiskVolume1\directory\file -> \Device\HarddiskVolume1\directory\ returned
	// \Device\HarddiskVolume1\path -> \Device\HarddiskVolume1\ returned
	// \Device\HarddiskVolume1\ -> \Device returned
	// \Device\HarddiskVolume1 -> \Device returned

	const npath npath::parent() const {
		string t = type(strip_trailing());
		if (t == "Object")
			return subpath(0,elements().size()-1);
		else
			return subpath(0,elements().size()-1).as_container();
	}


	// \Device\HarddiskVolume3\Users
	// 0\1\2\3 size = 3
	// (0,0) ->
	// (0,1) -> \ path
	// (1,1) -> Device
	const npath npath::subpath(size_t begin, size_t len) const
	{
		if (len == 0)
			return npath("");

		if (begin==0 && absolute() && len == 1)
			return npath("\\");

		size_t sz = elements().size();
		if (len > sz)
			len = sz;
		size_t end = begin + len;
		if (end > sz)
			end = sz;

		std::string result;

		for (size_t i = begin; i < end; i++) {
			if (i != begin) result += '\\';
			result += elements_[i].str();
		}
		return npath(nstring(result));
	}

	//bool npath::absolute() const { return elements()[0].empty(); }
	bool npath::absolute() const { return path_.size() > 0 && path_.front() == '\\'; }  // saves initialising elements_

	// "\" is the OM root, not a trailing separator
	bool npath::trailing() const { return path_.size() > 1 && path_.back() == '\\'; }

	const npath npath::as_container() const
	{
		if (path_.size() == 0 || path_.back() == '\\')
			return *this;
		return npath(path_ + '\\');
	}

	const npath npath::strip_trailing() const
	{
		if (!trailing())
			return *this;
		return npath(path_.substr(0, path_.size() - 1));
	}

	const nstring npath::type() const
	{
		if (!absolute())
			return "File";
		NtixObjectLib* nol = NtixObjectLib::get_instance();
		try {
			nstring type = nol->get_type(strip_trailing());
			std::cerr << "DEBUG: npath::type: get_type succeeded: " << *this << std::endl;

			// \Device\ is a sloppy directory; \Device\HarddiskVolume3\ is the namespace under the device
			if (!trailing() || type == "Directory")
				return "Object";
		} catch (nexception& e) {
			// std::cerr << "DEBUG: npath::type: get_type failed " << *this << ": "  << e << std::endl;
		}
		NtixFileLib* nfl = NtixFileLib::get_instance();
		if (nfl->exists(*this))
			return "File";
		throw nexception("npath::type", STATUS_OBJECT_NAME_NOT_FOUND); // what NtCreateFile returns if not found
	}

	// TODO: CASE SENSITIVITY
	bool npath::operator==(const npath& n) const { return this->str() == n.str(); }
	bool npath::operator==(const nstring& n) const { return this->str() == n.str(); }
	bool npath::operator==(const char* n) const { return this->str() == n; }

}

ntix::npath operator+ (const ntix::npath& lhs, const ntix::npath& rhs) { return lhs.append_path(rhs); }

std::ostream& operator<<(std::ostream& os, const ntix::npath& p)
{
	os << p.str();
	return os;
}

