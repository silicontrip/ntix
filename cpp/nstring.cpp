#include "nstring.hpp"

namespace ntix {

	std::regex nstring::glob_compile(const nstring& glob) {
		std::string regexStr = "^"; // Match from the start of the string
		regexStr.reserve(glob.size() * 2);

		size_t i = 0;
		size_t len = glob.size();

		while (i < len) {
			wchar_t c = glob[i];

			// 1. Handle double and single asterisks
			if (c == '*') {
				//
				// if (i + 1 < len && glob[i + 1] == '*') {
				//     regexStr += ".*"; // ** matches anything including slashes
				//     i += 2;
				// } else {
					regexStr += "[^\\]*"; // * matches anything except slashes
					i++;
				// }
            }
			// 2. Handle single character wildcard
			else if (c == '?') {
				regexStr += "[^\\]";
				i++;
			}
			// 3. Handle alternation braces {abc,123}
			// special case, escaped comma {abc,\\,,123}
			// special case, nested braces {abc,{123,789}}
			else if (c == '{') {
				regexStr += "(";
				i++;
				while (i < len && glob[i] != '}') {
					if (glob[i] == ',') {
						regexStr += "|";
					} else if (std::string(".+^$()|[]\\").find(glob[i]) != std::string::npos) {
						regexStr += "\\"; // Escape regex specials inside braces
						regexStr += glob[i];
					} else {
						regexStr += glob[i];
					}
					i++;
				}
				regexStr += ")";
				if (i < len) i++; // Skip closing '}'
			}
            // 4. Handle escape characters
            else if (c == '\\') {
				if (i + 1 < len && glob[i + 1] == '\\') {
					i++;
					if (i + 1 < len) {
						regexStr += "\\";
						regexStr += glob[i + 1];
						i += 2;
					} else {
						regexStr += "\\\\";
						i++;
					}
				} else {
					regexStr += "\\\\";
					i++;
				}
            }
            // 5. Escape native regex characters so they act as literals
            else if (std::string(".+^$()|{}[]").find(c) != std::string::npos) {
                regexStr += "\\";
                regexStr += c;
                i++;
            }
            // 6. Plain literal characters
            else {
                regexStr += c;
                i++;
            }
        }

        regexStr += "$"; // Match to the end of the string
        return std::regex(regexStr, std::regex::optimize);
    }

	std::vector<nstring> nstring::glob_filter(const std::vector<nstring>& items, const nstring& pattern) {
		std::regex re = glob_compile(pattern);
		std::vector<nstring> result;
		std::copy_if(items.begin(), items.end(), std::back_inserter(result),
			[&](const nstring& s) { return std::regex_match(s.str(), re); });
		return result;
	}

	nstring::nstring(UNICODE_STRING& uc)
	{
		std::wstring result(uc.Buffer, uc.Length/sizeof(WCHAR));
		wchar_ = result;
	}

	nstring::nstring(const WCHAR* buffer, ULONG lengthInBytes)
	{
		wchar_ = std::wstring(buffer, lengthInBytes / sizeof(WCHAR));
	}

	const std::wstring& nstring::wc_str() const {
		if (!wchar_)
			wchar_ = NtixCoreLib::get_instance()->utf8_to_wide(*utf8_);
		return *wchar_;
	}

	const UNICODE_STRING& nstring::unicode_str() const {
		if (us_.Buffer == nullptr)
			RtlInitUnicodeString(&us_, wc_str().c_str());  // wc_str() ensures the cache exists first
		return us_;
	}

	const std::string& nstring::str() const {
		if (!utf8_)
		{
		 // wc_str(), not the raw optional -- correct type, guarantees populated
		// which is probably not a goot idea, if both wchar_ and utf8_ are null
			if (wchar_)
				utf8_ = NtixCoreLib::get_instance()->wide_to_utf8(*wchar_);
			else
				utf8_ = "";
		}
		return *utf8_;
	}

	const bool nstring::match(const ntix::nstring& pattern) const
	{
		size_t o = 0;
		size_t p = 0;
		size_t tsize = this->size();
		size_t psize = pattern.size();
		while (p< pattern.size())
		{
			if (pattern[p] == '*')
			{
				while (p<psize)
				{
					if (pattern[p]!='*')
					{
						while(o<tsize)
						{
							if (this->substr(o).match(pattern.substr(p)))
								return true;
							o++;
						}
						return false;
					}
					p++;
				}
				return true;
			} else if (pattern[p] == '?') {
				if (o >= this->size())
					return false;
				o++;
				p++;
			}
		}
		return o == tsize;
	}

	size_t nstring::size() const { return this->str().size(); }
	bool nstring::empty() const { return this->str().empty(); }

	const char* nstring::c_str() const { return this->str().c_str(); }
	size_t nstring::find (const nstring& str, size_t pos) const { return this->str().find(str.str(),pos); }
	size_t nstring::find (char c, size_t pos) const { return this->str().find(c, pos); }
	size_t nstring::rfind (const nstring& str, size_t pos) const { return this->str().rfind(str.str(),pos); }

	const char& nstring::back() const { return this->str().back(); }
	const char& nstring::front() const { return this->str().front(); }
	const char& nstring::operator[] (size_t pos) const { return this->str()[pos]; }
	const char& nstring::at (size_t pos) const { return this->str().at(pos); }
	const nstring nstring::substr (size_t pos, size_t len) const { return nstring(this->str().substr(pos,len)); }

	int nstring::compare (const nstring& str) const { return this->str().compare(str.str()); }
	int nstring::compare (size_t pos, size_t len, const nstring& str) const { return this->str().compare(pos,len, str.str()); }
	int nstring::compare (size_t pos, size_t len, const nstring& str, size_t subpos, size_t sublen) const { return this->str().compare(pos,len, str.str(), subpos, sublen); }
	bool nstring::operator== (const nstring& o) const { return this->str() == o.str(); }
	bool nstring::operator== (const char* o) const { return this->str() == o; }
}

ntix::nstring operator+ (const ntix::nstring& lhs, const ntix::nstring& rhs) { return ntix::nstring(lhs.str() + rhs.str()); }
ntix::nstring operator+ (const ntix::nstring& lhs, char rhs) { return ntix::nstring(lhs.str() + rhs); }
ntix::nstring operator+ (char lhs, const ntix::nstring& rhs) { return ntix::nstring(lhs + rhs.str()); }
// bool operator== (const ntix::nstring& lhs, const ntix::nstring& rhs) { return lhs.str() == rhs.str(); }

std::ostream& operator<<(std::ostream& os, const ntix::nstring& p)
{
	os << p.str();
	return os;
}

