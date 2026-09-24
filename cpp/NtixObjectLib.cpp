#include "NtixObjectLib.hpp"
#include "NtixCoreLib.hpp"
#include "nexception.hpp"

namespace ntix {
	NtixObjectLib* NtixObjectLib::ptr = 0;

	NtixObjectLib::NtixObjectLib()
	{
		// do the ntdll loader
		PVOID hNtDll = GetModuleBase(L"ntdll.dll");

		// in the constructor:
#define X(name, ret, args) _##name = (_##name##_t)GetProcAddressNative(hNtDll, #name);
NTDLL_OBJECT_EXPORTS
#undef X

	}

	NtixObjectLib* NtixObjectLib::get_instance()
	{
		if (!ptr)
			ptr = new NtixObjectLib();
		return ptr;
	}

// Nt* wrappers, private, return NTSTATUS, do not throw.

	NTSTATUS NtixObjectLib::query(HANDLE Handle, ULONG ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength) const
	{
		return _NtQueryObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength, ReturnLength);
	}

	NTSTATUS NtixObjectLib::query_directory(HANDLE hDir, PVOID buffer, ULONG bufferLength, BOOLEAN ReturnSingleEntry, BOOLEAN restartScan, PULONG pContext, PULONG pReturnLength) const
	{
		return _NtQueryDirectoryObject(hDir, buffer, bufferLength, ReturnSingleEntry, restartScan, pContext, pReturnLength);
	}

	NTSTATUS NtixObjectLib::query_information_process(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) const
	{
		return _NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
	}

	NTSTATUS NtixObjectLib::query_information_token(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength, PULONG ReturnLength) const
	{
		return _NtQueryInformationToken(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength);
	}

	NTSTATUS NtixObjectLib::query_symbolic_link(HANDLE hSym, PUNICODE_STRING usTarget, PULONG returnedLength) const
	{
		return _NtQuerySymbolicLinkObject(hSym, usTarget, returnedLength);
	}

	NTSTATUS NtixObjectLib::read_virtual_memory(HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead,PULONG NumberOfBytesRead) const
	{
		return _NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
	}

// semantic calls, public, return a value, throw on NTSTATUS error

	void NtixObjectLib::adjust_privileges_token(HANDLE TokenHandle, PTOKEN_PRIVILEGES NewState, ULONG BufferLength) const
	{
		NTSTATUS s = _NtAdjustPrivilegesToken(TokenHandle, FALSE, NewState, BufferLength, NULL, NULL);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::adjust_privileges_token",s);
	}

	void NtixObjectLib::disable_privileges_token(HANDLE TokenHandle) const
	{
		NTSTATUS s = _NtAdjustPrivilegesToken(TokenHandle, TRUE, NULL, NULL, NULL, NULL);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::disable_privileges_token",s);
	}

	void NtixObjectLib::close(HANDLE h) const
	{
		NTSTATUS s = _NtClose(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::close",s);
	}

	const npath NtixObjectLib::get_symbolic_link_path(npath p) const
	{
		HANDLE hSym = open_symbolic_link(p,GENERIC_READ);
		ULONG size=0;
		try {
			size = query_symbolic_link_size(hSym);
		} catch (nexception& e) {
			close(hSym); // should we care about status as this point ?
			throw e;
		}

		UNICODE_STRING usTarget;

		WCHAR *targetWBuffer = (WCHAR*)malloc(size);

		usTarget.Buffer = targetWBuffer;
		usTarget.Length = 0;
		usTarget.MaximumLength = (USHORT)size;

		NTSTATUS status;
		status = _NtQuerySymbolicLinkObject(hSym, &usTarget, &size);

		// std::cout << "NtixObjectLib::get_symbolic_link_path::query_symbolic_link(usTarget) size: " << size << std::endl;
		// std::cout << "NtixObjectLib::get_symbolic_link_path::query_symbolic_link usTarget.Length: " << usTarget.Length << std::endl;

		if (!NT_SUCCESS(status))
		{
			free(targetWBuffer);
			close(hSym);
			throw nexception("NtixObjectLib::get_symbolic_link_path::query_symbolic_link(usTarget)",status);
		}

		nstring symPath(usTarget);

		// std::cout << "NtixObjectLib::get_symbolic_link_path query_symbolic_link: " << symPath << std::endl;

		free(targetWBuffer);
		close(hSym); // should we care about status as this point ?
		return npath(symPath);

	}

	// NT you truly suck at this, no mechanism to open an arbitrary path with minimum access QUERY_TYPE, to determine its type.
	// Then duplicate this handle with the access you desire.
	// A potential race condition exists if the symbolic link in the path happens to change between this call and the open call.
	//
	const nstring NtixObjectLib::get_type(npath p) const
	{
		if (p == "\\")
			return "Directory";

		// these two always seem to be needed when things go wrong
		//std::cerr << "NtixObjectLib::get_type path: "  << p << std::endl;
		//std::cerr << "NtixObjectLib::get_type parent path: "  << p.parent() << std::endl;

		try {
			std::vector<directory_info> dlist = read_directory(p.parent());
			for (directory_info ent: dlist)
			{
				if (p.basename() == ent.name)
					return ent.type;
			}
		} catch (nexception& e) {
			if (e.status() == STATUS_OBJECT_TYPE_MISMATCH)
				throw(nexception("NtixObjectLib::get_type read_directory", STATUS_OBJECT_NAME_NOT_FOUND));
			throw e;
		}

		// but wait this path might not appear in the parent directory.
		try {
			HANDLE h = open_directory(p,GENERIC_READ);
			close(h);
			return "Directory";
		} catch (nexception& e) {
			if (e.status() != STATUS_OBJECT_TYPE_MISMATCH )
				throw e;
		}
		try {
			HANDLE h = open_symbolic_link(p,GENERIC_READ);
			close(h);
			return "SymbolicLink";
		} catch (nexception& e) {
			if (e.status() != STATUS_OBJECT_TYPE_MISMATCH )
				throw e;
		}
		std::cerr << "NtixObjectLib::get_type UNHANDLED TYPE" << std::endl;  // I'm working on it
		throw(nexception("NtixObjectLib::get_type", STATUS_OBJECT_NAME_NOT_FOUND));
	}

	bool NtixObjectLib::is_directory(npath p) const
	{
		return get_type(p) == "Directory";
	}
	bool NtixObjectLib::is_symlink(npath p) const
	{
		return get_type(p) == "SymbolicLink";
	}

	void NtixObjectLib::make_temporary(HANDLE h) const
	{
		NTSTATUS s = _NtMakeTemporaryObject(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::make_temporary",s);
	}

	HANDLE NtixObjectLib::open_directory(npath p, ACCESS_MASK am) const
	{
		HANDLE hDir;
<<<<<<< HEAD
		NTSTATUS s = _NtOpenDirectoryObject(&hDir, am, &p.oa(0));
=======
		// the object manager rejects \Device\ with STATUS_OBJECT_NAME_INVALID
		NTSTATUS s = _NtOpenDirectoryObject(&hDir, am, &p.strip_trailing().oa(0));
>>>>>>> 8008defb673f4b2c7507bde812e2d1a529d82d08
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_directory",s);
		return hDir;
	}

	HANDLE NtixObjectLib::open_event(npath p, ACCESS_MASK am) const
	{
		HANDLE h;
		NTSTATUS s = _NtOpenEvent(&h, am, &p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_event",s);
		return h;
	}

	HANDLE NtixObjectLib::open_mutant(npath p, ACCESS_MASK am) const
	{
		HANDLE h;
		NTSTATUS s = _NtOpenMutant(&h, am, &p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_mutant",s);
		return h;
	}

	HANDLE NtixObjectLib::open_process_token(HANDLE p, ACCESS_MASK am) const
	{
		HANDLE h;
		NTSTATUS s = _NtOpenProcessToken(p, am, &h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_process_token",s);
		return h;
	}

	HANDLE NtixObjectLib::open_section(npath p, ACCESS_MASK am) const
	{
		HANDLE h;
		NTSTATUS s = _NtOpenSection(&h, am, &p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_section",s);
		return h;
	}

	HANDLE NtixObjectLib::open_semaphore(npath p, ACCESS_MASK am) const
	{
		HANDLE h;
		NTSTATUS s = _NtOpenSemaphore(&h, am, &p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_semaphore",s);
		return h;
	}

	HANDLE NtixObjectLib::open_symbolic_link(npath p, ACCESS_MASK am) const
	{
		HANDLE hSym;
		NTSTATUS s = _NtOpenSymbolicLinkObject(&hSym,am,&p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_symbolic_link",s);
		return hSym;
	}

	BOOLEAN NtixObjectLib::privilege_check(HANDLE p, PPRIVILEGE_SET ps) const
	{
		BOOLEAN Result;
		NTSTATUS s = _NtPrivilegeCheck(p, ps, &Result);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::privilege_check",s);

		return Result;
	}

	ULONG NtixObjectLib::query_information_token_size(HANDLE h, TOKEN_INFORMATION_CLASS tic) const
	{
		ULONG ReturnLength;

		NTSTATUS s = _NtQueryInformationToken(h, tic, NULL, 0, &ReturnLength);
		if (s != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_information_token_size)",s);

		return ReturnLength;
	}

	ULONG NtixObjectLib::query_security_size(HANDLE Handle) const
	{
		ULONG LengthNeeded;
		NTSTATUS s = _NtQuerySecurityObject(Handle, NULL, NULL, 0, &LengthNeeded);
		if (s != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_security_size",s);

		return LengthNeeded;
	}

	ULONG NtixObjectLib::query_size(HANDLE h, ULONG oic) const
	{
		ULONG ReturnLength;

		NTSTATUS s = _NtQueryObject(h, oic, NULL, 0, &ReturnLength);
		if (s != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_size)",s);

		return ReturnLength;
	}

	ULONG NtixObjectLib::query_symbolic_link_size(HANDLE hSym) const
	{
		ULONG size;
		UNICODE_STRING usTarget;
		usTarget.Length = 0;
		usTarget.MaximumLength = 0;

		NTSTATUS s = _NtQuerySymbolicLinkObject(hSym, &usTarget, &size);
		if (s != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_symbolic_link_size)",s);

		return size;
	}

	ULONG NtixObjectLib::query_system_information_size(SYSTEM_INFORMATION_CLASS SystemInformationClass) const
	{
		ULONG size;
		NTSTATUS s = _NtQuerySystemInformation (SystemInformationClass, NULL, 0, &size);
		if (s != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_system_information_size)",s);
		return size;
	}


	std::vector<directory_info> NtixObjectLib::read_directory(npath p) const
	{

		//std::cout << "NtixObjectLib::read_directory path: " << p << std::endl;

		NTSTATUS status;
		HANDLE hDir = open_directory(p,GENERIC_READ);

		PBYTE query_buf = (PBYTE)malloc(32768);
		if(!query_buf)
		{
			close(hDir);
			throw nexception("NtixObjectLib::read_directory::malloc(32768)",STATUS_NO_MEMORY);
		}

		ULONG query_context = 0;
		BOOLEAN restart = TRUE;

		std::vector<directory_info> directory_list;

		for (;;) {
			ULONG retLen = 0;
			status = query_directory(hDir, query_buf, 32768, false, restart, &query_context, &retLen);

			//std::cout << "NtixObjectLib::read_directory retLen: " << retLen << std::endl;

			restart = FALSE;
			if (status == STATUS_NO_MORE_ENTRIES) break;

			if (!NT_SUCCESS(status))
			{
				close(hDir);
				free(query_buf);
				throw nexception("NtixObjectLib::read_directory::query_directory",status);
			}

			OBJECT_DIRECTORY_INFORMATION *pObjInfo = (OBJECT_DIRECTORY_INFORMATION *)query_buf;
			ULONG offset = 0;
			while (pObjInfo->Name.Length > 0 && offset + sizeof(OBJECT_DIRECTORY_INFORMATION) <= retLen) {

				// std::cout << "NtixObjectLib::read_directory offset: " << offset << std::endl;

				nstring name(pObjInfo->Name);
				nstring type(pObjInfo->TypeName);

				// std::cout << "NtixObjectLib::read_directory name: " << name << " type: " << type <<  std::endl;

				directory_list.push_back( { name, type } );

				// std::cout << "NtixObjectLib::read_directory directory_list size: " << directory_list.size() << std::endl;

				pObjInfo++;
				offset += sizeof(OBJECT_DIRECTORY_INFORMATION);
			}
		}
		free(query_buf);
		close(hDir);
		return directory_list;
	}

	void NtixObjectLib::set_information(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength) const
	{
		NTSTATUS s = _NtSetInformationObject(Handle, ObjectInformationClass, ObjectInformation, ObjectInformationLength);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::set_information",s);
	}

	void NtixObjectLib::set_information_token(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, PVOID TokenInformation, ULONG TokenInformationLength) const
	{
		NTSTATUS s = _NtSetInformationToken(TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::set_information_token",s);
	}

	void NtixObjectLib::set_security(HANDLE Handle, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor) const
	{
		NTSTATUS s = _NtSetSecurityObject(Handle,SecurityInformation,SecurityDescriptor);
		if(!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::set_security",s);
	}


}
