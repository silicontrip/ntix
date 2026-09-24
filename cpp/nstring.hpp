#ifndef NTIX_NSTRING_HPP
#define NTIX_NSTRING_HPP

#include <string>
#include <iostream>
#include <regex>
#include <algorithm>
//#include <windows.h>
//#include "NtixCoreLib.hpp"
#include "nexception.hpp"

namespace ntix {

	class NtixCoreLib;

	class nstring {
		private:
			mutable std::optional<std::string> utf8_;
			mutable std::optional<std::wstring> wchar_;
			mutable UNICODE_STRING us_{};  // zero-init: Buffer==nullptr is "not yet computed"

			static std::regex glob_compile(const nstring& glob);


		public:
			static const size_t npos = -1;

			explicit nstring(std::string utf8) : utf8_(std::move(utf8)) {}
			explicit nstring(std::wstring wchar) : wchar_(std::move(wchar)) {}
			nstring(const char* utf8) : utf8_(utf8) {}

			nstring(UNICODE_STRING& us);
			nstring(const WCHAR* buffer, ULONG lengthInBytes);

			// us_ is a lazy cache that points into wchar_'s own storage --
			// must never be copied/moved by value, only ever recomputed for *this* instance.
			nstring(const nstring& other);
			nstring(nstring&& other) noexcept;
			nstring& operator=(const nstring& other);
			nstring& operator=(nstring&& other) noexcept;

			std::vector<nstring> glob_filter(const std::vector<nstring>& items, const nstring& pattern);
			bool is_glob() const;
			bool glob_match(const nstring& pattern) const;
			std::regex nstring::glob_compile(const nstring& glob);


			const std::wstring& wc_str() const;
			const UNICODE_STRING& unicode_str() const;
			const std::string& str() const;
			const char* c_str() const;

			size_t size() const;
			bool empty() const;

			size_t find (const nstring& str, size_t pos = 0) const;
			size_t find (char c, size_t pos = 0) const;
			size_t rfind (const nstring& str, size_t pos = npos) const;

			const char& back() const;
			const char& front() const;
			const char& operator[] (size_t pos) const;
			const char& at (size_t pos) const;

			const nstring substr (size_t pos = 0, size_t len = npos) const;

			int compare (const nstring& str) const;
			int compare (size_t pos, size_t len, const nstring& str) const;
			int compare (size_t pos, size_t len, const nstring& str, size_t subpos, size_t sublen) const;
			bool operator==(const nstring& o) const;
			bool operator==(const char* o) const;

			const nstring& lower() const;
	};
}

std::ostream& operator<<(std::ostream& os, const ntix::nstring& l);

ntix::nstring operator+ (const ntix::nstring& lhs, const ntix::nstring& rhs);
ntix::nstring operator+ (const ntix::nstring& lhs, char rhs);
ntix::nstring operator+ (char lhs, const ntix::nstring& rhs);

//bool operator== (const ntix::nstring& lhs, const ntix::nstring& rhs);

template<> struct std::hash<ntix::nstring> {
	std::size_t operator()(const ntix::nstring& s) const noexcept {
		return std::hash<string>{}(s.str());
	}
};

#endif
