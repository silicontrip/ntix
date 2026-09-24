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
	X(NtAdjustPrivilegesToken, NTSTATUS, (HANDLE, BOOLEAN, PTOKEN_PRIVILEGES, ULONG, PTOKEN_PRIVILEGES, PULONG)) \
	X(NtClose, NTSTATUS, (HANDLE)) \
	X(NtMakeTemporaryObject, NTSTATUS, (HANDLE)) \
	X(NtOpenDirectoryObject, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtOpenEvent, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtOpenMutant, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtOpenProcessToken, NTSTATUS, (HANDLE, ACCESS_MASK, PHANDLE)) \
	X(NtOpenSection, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtOpenSemaphore, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtOpenSymbolicLinkObject, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)) \
	X(NtPrivilegeCheck, NTSTATUS, (HANDLE, PPRIVILEGE_SET, PBOOLEAN)) \
	X(NtQueryDirectoryObject, NTSTATUS, (HANDLE, PVOID, ULONG, BOOLEAN, BOOLEAN, PULONG, PULONG)) \
	X(NtQueryInformationProcess, NTSTATUS, (HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG)) \
	X(NtQueryInformationToken, NTSTATUS, (HANDLE, TOKEN_INFORMATION_CLASS, PVOID, ULONG, PULONG)) \
	X(NtQueryObject, NTSTATUS, (HANDLE, ULONG, PVOID, ULONG, PULONG)) \
	X(NtQuerySecurityObject, NTSTATUS, (HANDLE, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR, ULONG, PULONG)) \
	X(NtQuerySymbolicLinkObject, NTSTATUS, (HANDLE, PUNICODE_STRING, PULONG)) \
	X(NtQuerySystemInformation, NTSTATUS, (SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG)) \
	X(NtReadVirtualMemory, NTSTATUS, (HANDLE, PVOID, PVOID, ULONG, PULONG)) \
	X(NtSetInformationObject, NTSTATUS, (HANDLE, OBJECT_INFORMATION_CLASS, PVOID, ULONG)) \
	X(NtSetInformationToken, NTSTATUS, (HANDLE, TOKEN_INFORMATION_CLASS, PVOID, ULONG)) \
	X(NtSetSecurityObject, NTSTATUS, (HANDLE, SECURITY_INFORMATION, PSECURITY_DESCRIPTOR))

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

		NTSTATUS query(HANDLE Handle, ULONG ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength) const;
		NTSTATUS query_directory(HANDLE hDir, PVOID buffer, ULONG bufferLength, BOOLEAN ReturnSingleEntry, BOOLEAN restartScan, PULONG pContext, PULONG pReturnLength) const;
		NTSTATUS query_information_process(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) const;
		NTSTATUS query_information_token(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength) const;
		NTSTATUS query_symbolic_link(HANDLE hSym, PUNICODE_STRING usTarget, PULONG returnedLength) const;
		NTSTATUS read_virtual_memory(HANDLE ProcessHandle, PVOID BaseAddress,PVOID Buffer, ULONG NumberOfBytesToRead, PULONG NumberOfBytesRead) const;

	public:
		static NtixObjectLib* get_instance();

		NtixObjectLib(const NtixObjectLib&) = delete;
		NtixObjectLib(NtixObjectLib&&) = delete;
		NtixObjectLib& operator=(const NtixObjectLib&) = delete;
		NtixObjectLib& operator=(NtixObjectLib&&) = delete;

		// if it throws and doesn't return NTSTATUS, it's public

		void adjust_privileges_token(HANDLE TokenHandle, PTOKEN_PRIVILEGES NewState, ULONG BufferLength) const;

		void close(HANDLE h) const;

		void disable_privileges_token(HANDLE TokenHandle) const;

		const npath get_symbolic_link_path(npath p) const;
		const nstring get_type(npath p) const;
		bool is_directory(npath p) const;
		bool is_symlink(npath p) const;

		void make_temporary(HANDLE h) const;

		HANDLE open_directory(npath p, ACCESS_MASK am) const;
		HANDLE open_event(npath p, ACCESS_MASK am) const;
		HANDLE open_mutant(npath p, ACCESS_MASK am) const;
		HANDLE open_process_token(HANDLE p, ACCESS_MASK am) const;
		HANDLE open_section(npath p, ACCESS_MASK am) const;
		HANDLE open_semaphore(npath p, ACCESS_MASK am) const;
		HANDLE open_symbolic_link(npath p, ACCESS_MASK am) const;

		BOOLEAN privilege_check(HANDLE p, PPRIVILEGE_SET ps) const;

		ULONG query_size(HANDLE oh, ULONG oic) const;
		ULONG query_information_process_size(HANDLE ph, PROCESSINFOCLASS pic) const;
		ULONG query_information_token_size(HANDLE ph, TOKEN_INFORMATION_CLASS tic) const;
		ULONG query_security_size(HANDLE Handle) const;
		ULONG query_symbolic_link_size(HANDLE hSym) const;
		ULONG query_system_information_size(SYSTEM_INFORMATION_CLASS SystemInformationClass) const;

		std::vector<directory_info> read_directory(npath p) const;

		void set_information(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength) const;
		void set_information_token(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength) const;
		void set_security(HANDLE Handle, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor) const;
	};
}
#endif
