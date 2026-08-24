#ifndef NTIX_CORE_HPP
#define NTIX_CORE_HPP

#include <string>

#include "NtixLoader.hpp"
#include "nexception.hpp"
#include "nstring.hpp"

#define NTDLL_CORE_EXPORTS \
	X(RtlMultiByteToUnicodeN, NTSTATUS, (WCHAR*, ULONG, PULONG, const char*, ULONG)) \
	X(RtlUnicodeToMultiByteN, NTSTATUS, (char*, ULONG, PULONG, const WCHAR*, ULONG)) \
	X(RtlGetFullPathName_U, NTSTATUS, (PCWSTR, ULONG, PWSTR, PWSTR*)) \
	X(NtQuerySystemTime, NTSTATUS, (PLARGE_INTEGER))

#define X(name, ret, args) typedef ret (NTAPI *_##name##_t) args;
NTDLL_CORE_EXPORTS
#undef X


// typedef NTSTATUS (NTAPI *_RTLMULTIBYTETOUNICODEN)(
// 	PWCH UnicodeString, ULONG MaxBytesInUnicodeString,
// 	PULONG BytesInUnicodeString, const char* MultiByteString,
// 	ULONG BytesInMultibyteString
// );
//
// typedef NTSTATUS (NTAPI *_RTLUNICODETOMULTIBYTEN)(
// 	PCHAR MultiByteString, ULONG MaxBytesInMultiByteString,
// 	PULONG BytesInMultiByteString, PCWCH  UnicodeString,
// 	ULONG BytesInUnicodeString
// );

namespace ntix {

	class NtixCoreLib {
	private:

#define X(name, ret, args) _##name##_t _##name = nullptr;
NTDLL_CORE_EXPORTS
#undef X

		NtixCoreLib();  // no one else can create one
		~NtixCoreLib(); // prevent accidental deletion

		static NtixCoreLib* ptr;

		NTSTATUS query_system_time(PLARGE_INTEGER sysTime) const;
		NTSTATUS multibyte_to_unicode(WCHAR* ustr, ULONG max, PULONG bytes, const char* utf8, ULONG len) const;
		NTSTATUS unicode_to_multibyte(char* utf8, ULONG max, PULONG bytes, const WCHAR* ustr, ULONG len) const;
		NTSTATUS full_pathname(PCWSTR filename, ULONG len, PWSTR buffer, PWSTR* filepart) const;

	public:
		static const std::string status_str(NTSTATUS status);
		static NtixCoreLib* get_instance();

		NtixCoreLib(const NtixCoreLib&) = delete;
		NtixCoreLib(NtixCoreLib&&) = delete;
		NtixCoreLib& operator=(const NtixCoreLib&) = delete;
		NtixCoreLib& operator=(NtixCoreLib&&) = delete;

		LONGLONG system_time() const;
		std::wstring utf8_to_wide(const std::string& utf8) const;
		std::string wide_to_utf8(const std::wstring& wide) const;
		std::wstring resolve_path(const std::wstring& in) const;

	};
}
#endif
