#ifndef NTIX_NPATH_HPP
#define NTIX_NPATH_HPP

#include <vector>
#include <sstream>
#include "nstring.hpp"
#include "NtixObjectLib.hpp"
#include "NtixFileLib.hpp"
#include "NtixCoreLib.hpp"

namespace ntix {

	class npath {
		private:
			// bool casesensitive_; // TODO:
			nstring path_;
			mutable OBJECT_ATTRIBUTES oa_{};
			mutable UNICODE_STRING uc_{};
			mutable std::vector<nstring> elements_{};
		public:

			static const size_t npos = -1;
			explicit npath(const nstring& p) : path_(std::move(p)) {}
			//explicit npath(const std::string& p) : path_(nstring(p)) {}

			// oa_/uc_ are lazy caches that self-reference (oa_.ObjectName == &uc_) or point
			// into path_'s storage -- must never be copied/moved by value, only recomputed.
			npath(const npath& other);
			npath(npath&& other) noexcept;
			npath& operator=(const npath& other);
			npath& operator=(npath&& other) noexcept;

			const npath append_path(const npath& p) const;
			const npath normalise() const;
			const npath resolve() const;

			// user path:  .\Directory
			// converted path: \Device\HarddiskVolume3\Users\example\Directory
			// this path: \Device\HarddiskVolume3\Users\example\Directory\subdir\file.txt

			// -> .\Directory\subdir\file.txt (unless there is a bug, bugs will be fixed, not accepted as an accepted quirk)

			const npath unresolve(const npath& user, const npath& converted) const;
			bool glob_match(const npath& match) const;
			std::vector<npath> glob_expand() const;
			OBJECT_ATTRIBUTES& oa(ULONG oa_flags) const;
			const std::string& str() const;
			const nstring& nstr() const;
			const std::vector<nstring>& elements() const;
			const nstring& at(size_t index) const;
			const nstring& operator[] (size_t index) const;
			size_t length() const;
			size_t size() const;
			bool empty() const;
			const npath basename() const;
			const npath parent() const;
			const npath object_parent() const;
			const npath file_parent() const;

			// this returns a relative style path, never carries the trailing separator
			const npath subpath(size_t begin, size_t len = npos) const;
			bool absolute() const;
			// a trailing '\' is not an element; it addresses the namespace beneath the
			// final element (\Device\HarddiskVolume3\ is the volume root, not the device)
			bool trailing() const;
			const npath as_container() const;   // for file system directory opens
			const npath strip_trailing() const; // for object manager opens
			const nstring type() const;

			bool operator==(const npath& n) const;
			bool operator==(const nstring& n) const;
			bool operator==(const char* n) const;

	};
}

std::ostream& operator<<(std::ostream& os, const ntix::npath& l);

ntix::npath operator+ (const ntix::npath& lhs, const ntix::npath& rhs);

// this should take case sensitivity into account.
// and be smart about it...
template<> struct std::hash<ntix::npath> {
	std::size_t operator()(const ntix::npath& s) const noexcept {
		return std::hash<string>{}(s.str());
	}
};

#endif
