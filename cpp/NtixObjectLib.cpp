#include "NtixObjectLib.hpp"

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
	NTSTATUS NtixObjectLib::query_symbolic_link(HANDLE hSym, PUNICODE_STRING usTarget, PULONG returnedLength) const
	{
		return _NtQuerySymbolicLinkObject(hSym, usTarget, returnedLength);
	}

	NTSTATUS NtixObjectLib::query_directory(HANDLE hDir, PVOID buffer, ULONG bufferLength, BOOLEAN ReturnSingleEntry, BOOLEAN restartScan, PULONG pContext, PULONG pReturnLength) const
	{
		return _NtQueryDirectoryObject(hDir, buffer, bufferLength, ReturnSingleEntry, restartScan, pContext, pReturnLength);
	}

	NTSTATUS NtixObjectLib::query_information_process(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
		PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength) const
	{
		return _NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
	}

	NTSTATUS Ntix::read_virtual_memoryHANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer, ULONG NumberOfBytesToRead,PULONG NumberOfBytesRead) const
	{
		return _NtReadVirtualMemory(ProcessHandle, BaseAddress, Buffer, NumberOfBytesToRead, NumberOfBytesRead);
	}

// semantic calls, public, return a value, throw on NTSTATUS error
	void NtixObjectLib::close(HANDLE h) const
	{
		NTSTATUS s = _NtClose(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::close",s);
	}

	HANDLE NtixObjectLib::open_symbolic_link(npath p, ACCESS_MASK am) const
	{
		HANDLE hSym;
		NTSTATUS s = _NtOpenSymbolicLinkObject(&hSym,am,&p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_symbolic_link",s);
		return hSym;
	}

	ULONG NtixObjectLib::query_symbolic_link_size(HANDLE hSym) const
	{
		ULONG size;
		UNICODE_STRING usTarget;
		usTarget.Length = 0;
		usTarget.MaximumLength = 0;

		NTSTATUS status;
		status = query_symbolic_link(hSym, &usTarget, &size);
		if (status != STATUS_BUFFER_TOO_SMALL)
			throw nexception("NtixObjectLib::query_symbolic_link_size)",status);

		return size;
	}

	HANDLE NtixObjectLib::open_directory(npath p, ACCESS_MASK am) const
	{
		HANDLE hDir;
		NTSTATUS s = _NtOpenDirectoryObject(&hDir, am, &p.oa(0));
		if (!NT_SUCCESS(s))
			throw nexception("NtixObjectLib::open_directory",s);
		return hDir;
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
		status = query_symbolic_link(hSym, &usTarget, &size);

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

				//struct directory_info directory_entry = { name, type };

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
}
