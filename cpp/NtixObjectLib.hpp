#ifndef NTIX_OBJECT_HPP
#define NTIX_OBJECT_HPP

#include <string>

#include "NtixLoader.hpp"
#include "nstring.hpp"
#include "npath.hpp"

typedef struct _OBJECT_DIRECTORY_INFORMATION
{
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

#define NTDLL_OBJECT_EXPORTS \
	X(NtClose, NTSTATUS, (HANDLE)) \
	X(NtOpenSymbolicLinkObject, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtQuerySymbolicLinkObject, NTSTATUS, (HANDLE, PUNICODE_STRING, PULONG)) \
	X(NtOpenDirectoryObject, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtQueryDirectoryObject, NTSTATUS, (HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG))

#define X(name, ret, args) typedef ret (NTAPI *_##name##_t) args;
NTDLL_OBJECT_EXPORTS
#undef X

namespace ntix {

class npath;

typedef struct directory_info_t {
	nstring name;
	nstring type;
} directory_info;

	class NtixObjectLib {
	private:

#define X(name, ret, args) _##name##_t _##name = nullptr;
NTDLL_OBJECT_EXPORTS
#undef X

		NtixObjectLib();  // no one else can create one
		~NtixObjectLib(); // prevent accidental deletion

		static NtixObjectLib* ptr;

		void close(HANDLE h) const;
		HANDLE open_symbolic_link(npath p, ACCESS_MASK am) const;
		NTSTATUS query_symbolic_link(HANDLE hSym, PUNICODE_STRING usTarget, PULONG returnedLength) const;
		ULONG query_symbolic_link_size(HANDLE hSym) const;
		HANDLE open_directory(npath p, ACCESS_MASK am) const;
		NTSTATUS query_directory(HANDLE hDir, PVOID buffer, ULONG bufferLength, BOOLEAN ReturnSingleEntry,
			BOOLEAN restartScan, PULONG pContext, PULONG pReturnLength) const;


	public:
		static NtixObjectLib* get_instance();

		NtixObjectLib(const NtixObjectLib&) = delete;
		NtixObjectLib(NtixObjectLib&&) = delete;
		NtixObjectLib& operator=(const NtixObjectLib&) = delete;
		NtixObjectLib& operator=(NtixObjectLib&&) = delete;

		const npath get_symbolic_link_path(npath p) const;
		std::vector<directory_info> read_directory(npath p) const;
		const nstring get_type(npath p) const;
		bool is_directory(npath p) const;
		bool is_symlink(npath p) const;


	};
}
#endif
