#include "NtixCoreLib.hpp"

namespace ntix {
	NtixCoreLib* NtixCoreLib::ptr = 0;

// https://joyasystems.com/list-of-ntstatus-codes
	const std::string NtixCoreLib::status_str (NTSTATUS status) {
		switch (status) {
			case 0:
				return "STATUS_SUCCESS";
			case (NTSTATUS)0x80000006:
				return "STATUS_NO_MORE_FILES";
			case (NTSTATUS)0x8000001A:
				return "STATUS_NO_MORE_ENTRIES";
			case (NTSTATUS)0xC0000001:
				return "STATUS_UNSUCCESSFUL";
			case (NTSTATUS)0xC0000002:
				return "STATUS_NOT_IMPLEMENTED";
			case (NTSTATUS)0xC0000003L:
				return "STATUS_INVALID_INFO_CLASS";
			case (NTSTATUS)0xC0000004L:
				return "STATUS_INFO_LENGTH_MISMATCH";
			case (NTSTATUS)0xC0000005L:
				return "STATUS_ACCESS_VIOLATION";
			case (NTSTATUS)0xC0000008L:
				return "STATUS_INVALID_HANDLE";
			case (NTSTATUS)0xC000000BL:
				return "STATUS_INVALID_CID";
			case (NTSTATUS)0xC000000DL:
				return "STATUS_INVALID_PARAMETER";
			case (NTSTATUS)0xC000000F:
				return "STATUS_NO_SUCH_FILE";
			case (NTSTATUS)0xC0000010L:
				return "STATUS_INVALID_DEVICE_REQUEST";
			case (NTSTATUS)0xC0000011L:
				return "STATUS_END_OF_FILE";
			case (NTSTATUS)0xC0000017L:
				return "STATUS_NO_MEMORY";
			case (NTSTATUS)0xC0000022L:
				return "STATUS_ACCESS_DENIED";
			case (NTSTATUS)0xC0000023L:
				return "STATUS_BUFFER_TOO_SMALL";
			case (NTSTATUS)0xC0000024L:
				return "STATUS_OBJECT_TYPE_MISMATCH";
			case (NTSTATUS)0xC0000033L:
				return "STATUS_OBJECT_NAME_INVALID";
			case (NTSTATUS)0xC0000034L:
				return "STATUS_OBJECT_NAME_NOT_FOUND";
			case (NTSTATUS)0xC0000035L:
				return "STATUS_OBJECT_NAME_COLLISION";
			case (NTSTATUS)0xC0000039L:
				return "STATUS_OBJECT_PATH_INVALID";
			case (NTSTATUS)0xC000003AL:
				return "STATUS_OBJECT_PATH_NOT_FOUND";
			case (NTSTATUS)0xC000003BL:
				return "STATUS_OBJECT_PATH_SYNTAX_BAD";
			case (NTSTATUS)0xC0000043L:
				return "STATUS_SHARING_VIOLATION";
			case (NTSTATUS)0xC0000056L:
				return "STATUS_DELETE_PENDING";
			case (NTSTATUS)0xC000005AL:
				return "STATUS_DLL_INIT_FAILED";
			case (NTSTATUS)0xC00000B0L:
				return "STATUS_PIPE_DISCONNECTED";
			case (NTSTATUS)0xC00000BAL:
				return "STATUS_FILE_IS_A_DIRECTORY";
			case (NTSTATUS)0xC00000BBL:
				return "STATUS_NOT_SUPPORTED";
			case (NTSTATUS)0xC00000E3L:
				return "STATUS_INVALID_OPLOCK_PROTOCOL";
			case (NTSTATUS)0xC0000101L:
				return "STATUS_DIRECTORY_NOT_EMPTY";
			case (NTSTATUS)0xC0000103L:
				return "STATUS_NOT_A_DIRECTORY";
			case (NTSTATUS)0xC0000121L:
				return "STATUS_CANNOT_DELETE";
			case (NTSTATUS)0xC0000225L:
				return "STATUS_NOT_FOUND";
			case (NTSTATUS)0xC0000275L:
				return "STATUS_NOT_A_REPARSE_POINT";
			case (NTSTATUS)0xC0000276L:
				return "STATUS_IO_REPARSE_TAG_INVALID";
			case (NTSTATUS)0xC0000277L:
				return "STATUS_IO_REPARSE_TAG_MISMATCH";
			case (NTSTATUS)0xC0000278L:
				return "STATUS_IO_REPARSE_DATA_INVALID";
			default:
				return "UNKNOWN_STATUS";
		}
	}

	NtixCoreLib::NtixCoreLib()
	{
		// do the ntdll loader
		PVOID hNtDll = GetModuleBase(L"ntdll.dll");

		// in the constructor:
#define X(name, ret, args) _##name = (_##name##_t)GetProcAddressNative(hNtDll, #name);
NTDLL_CORE_EXPORTS
#undef X

	}

	NtixCoreLib* NtixCoreLib::get_instance()
	{
		if (!ptr)
			ptr = new NtixCoreLib();
		return ptr;
	}

	NTSTATUS NtixCoreLib::multibyte_to_unicode(WCHAR* ustr, ULONG max, PULONG bytes, const char* utf8, ULONG len) const
	{
		return _RtlMultiByteToUnicodeN (ustr,max,bytes,utf8,len);
	}

	NTSTATUS NtixCoreLib::unicode_to_multibyte(char* utf8, ULONG max, PULONG bytes, const WCHAR* ustr, ULONG len) const
	{
		return _RtlUnicodeToMultiByteN (utf8,max,bytes,ustr,len);
	}

	NTSTATUS NtixCoreLib::full_pathname(PCWSTR filename, ULONG len, PWSTR buffer, PWSTR* filepart) const
	{
		return _RtlGetFullPathName_U(filename, len, buffer, filepart);
	}

	NTSTATUS NtixCoreLib::query_system_time(PLARGE_INTEGER sysTime) const
	{
		return _NtQuerySystemTime(sysTime);
	}

	LONGLONG NtixCoreLib::system_time() const
	{
		LARGE_INTEGER st;
		NTSTATUS s = query_system_time(&st);
		if (!NT_SUCCESS(s))
			throw nexception("NtixCoreLib::system_time",s);
		return st.QuadPart;
	}

  // UTF-8 never needs more UTF-16 code units than input bytes (worst case
  // is 1:1 -- see the comment at the call site in wc_str()), so no
  // NULL-probe sizing call is needed: allocate the safe upper bound, call
  // once, trim to what was actually written.
	std::wstring NtixCoreLib::utf8_to_wide(const std::string& utf8) const {
		std::wstring result(utf8.size(), L'\0');
		ULONG bytesWritten = 0;
		NTSTATUS status = multibyte_to_unicode(result.data(),
			(ULONG)(result.size() * sizeof(WCHAR)), &bytesWritten, utf8.c_str(),
			(ULONG)utf8.size());

		if (!NT_SUCCESS(status))
			throw nexception("nstring::ConvertUtf8ToWide failed",status);
		result.resize(bytesWritten / sizeof(WCHAR));
		return result;
	}

  // Worst case 3 UTF-8 bytes per UTF-16 code unit (BMP) -- see wc_str()'s
  // comment for the mirror-image reasoning. Safe upper bound, one call.
	 std::string NtixCoreLib::wide_to_utf8(const std::wstring& wide) const {
		std::string result(wide.size() * 3, '\0');
		ULONG bytesWritten = 0;
		NTSTATUS status = NtixCoreLib::get_instance()->unicode_to_multibyte(
			result.data(), (ULONG)result.size(),
			&bytesWritten, wide.c_str(), (ULONG)(wide.size() * sizeof(WCHAR)));
		if (!NT_SUCCESS(status))
			throw nexception("nstring::ConvertWideToUtf8 failed",status);
		result.resize(bytesWritten);
		return result;
	}

	std::wstring NtixCoreLib::resolve_path(const std::wstring& in) const {
		ULONG needed = full_pathname(in.c_str(), 0, nullptr, nullptr);
		if (needed == 0)
			throw nexception("npath::ResolveFullPath failed", STATUS_UNSUCCESSFUL);
		std::wstring result(needed / sizeof(WCHAR), L'\0');
		ULONG written = NtixCoreLib::get_instance()->full_pathname(in.c_str(), needed, result.data(), nullptr);
		if (written == 0)
			throw nexception("npath::ResolveFullPath failed", STATUS_UNSUCCESSFUL);
		result.resize(written / sizeof(WCHAR));
		return result;
	}

}
