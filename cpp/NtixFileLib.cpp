#include "NtixFileLib.hpp"
#include "nexception.hpp"
#include <cstring>

namespace ntix {
	NtixFileLib* NtixFileLib::ptr = 0;

	NtixFileLib::NtixFileLib()
	{
		// do the ntdll loader
		PVOID hNtDll = GetModuleBase(L"ntdll.dll");

		// in the constructor:
#define X(name, ret, args) _##name = (_##name##_t)GetProcAddressNative(hNtDll, #name);
NTDLL_FILE_EXPORTS
#undef X

	}

	NtixFileLib* NtixFileLib::get_instance()
	{
		if (!ptr)
			ptr = new NtixFileLib();
		return ptr;
	}

	NTSTATUS NtixFileLib::create_file(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
		ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
		ULONG EaLength) const
	{

		return _NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
			AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions,
			EaBuffer, EaLength);
	}

	NTSTATUS NtixFileLib::query_directory_file(HANDLE hDir, PVOID buffer, ULONG bufferLength, ULONG infoClass, BOOLEAN restartScan, PULONG pReturnLength) const
	{

		IO_STATUS_BLOCK isb;

		NTSTATUS status = _NtQueryDirectoryFile(hDir, NULL, NULL, NULL, &isb, buffer, bufferLength, infoClass,
			FALSE, // Return multiple entries
			NULL, restartScan);

		if (NT_SUCCESS(status)) {
			*pReturnLength = (ULONG)isb.Information;
		} else {
			*pReturnLength = 0;
		}
		return status;

	}

	NTSTATUS NtixFileLib::fscontrol_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode,
		PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
		ULONG OutputBufferLength) const
	{
		return _NtFsControlFile (FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

	NTSTATUS NtixFileLib::set_information_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const
	{
		return _NtSetInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
	}

	NTSTATUS NtixFileLib::query_information_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const
	{
		return _NtQueryInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
	}

	NTSTATUS NtixFileLib::query_volume_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FsInformation, ULONG Length, ULONG FsInformationClass) const
	{
		return _NtQueryVolumeInformationFile(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
	}

	NTSTATUS NtixFileLib::device_ioctl_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode,
		PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
		ULONG OutputBufferLength) const
	{
		return _NtDeviceIoControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

	NTSTATUS NtixFileLib::read_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
		PULONG Key) const
	{
		return _NtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
	}

	NTSTATUS NtixFileLib::write_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
		PULONG Key) const
	{
		return _NtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
	}

	NTSTATUS NtixFileLib::query_ea_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
		ULONG Length, BOOLEAN ReturnSingleEntry, PVOID EaList, ULONG EaListLength,
		PULONG EaIndex, BOOLEAN RestartScan) const
	{
		return _NtQueryEaFile(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
	}

	NTSTATUS NtixFileLib::set_ea_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length) const
	{
		return _NtSetEaFile(FileHandle, IoStatusBlock, Buffer, Length);
	}

	void NtixFileLib::close(HANDLE h) const
	{
		NTSTATUS status = _NtClose(h);
		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::close",status); // has this call ever failed?
	}

	const HANDLE NtixFileLib::open(npath p, ACCESS_MASK access, ULONG share, ULONG disposition, ULONG options, ULONG oa_flags) const
	{
		HANDLE hFile;
		IO_STATUS_BLOCK isb;
		NTSTATUS status = _NtCreateFile(&hFile, access, &p.oa(oa_flags), &isb, NULL, FILE_ATTRIBUTE_NORMAL,
			share, disposition, options, NULL, 0);
		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::open",status);
		return hFile;
	}

	const HANDLE NtixFileLib::open(ULONGLONG fileId, ACCESS_MASK access, ULONG share, ULONG disposition, ULONG options, ULONG oa_flags) const
	{
		HANDLE hFile;

		UNICODE_STRING usId;
		usId.Buffer        = (PWSTR)&fileId;
		usId.Length        = sizeof(ULONGLONG);
		usId.MaximumLength = sizeof(ULONGLONG);

		OBJECT_ATTRIBUTES oa;

		oa.Length = sizeof(OBJECT_ATTRIBUTES);
		oa.ObjectName = &usId;
		oa.RootDirectory = NULL;
		oa.Attributes = oa_flags;
		oa.SecurityDescriptor = NULL;
		oa.SecurityQualityOfService = NULL;

		IO_STATUS_BLOCK isb;
		NTSTATUS status = _NtCreateFile(&hFile, access, &oa, &isb, NULL, 0, share, FILE_OPEN, options | 0x00002000L,  NULL, 0);

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::open",status);
		return hFile;
	}

	bool NtixFileLib::exists(npath p) const
	{
		try {
			HANDLE hFile = open(p, SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, 0);
			close(hFile);
		} catch (nexception& e) {
			// we may want to throw on some exceptions
			std::cerr << "DEBUG: NtixFileLib::exists path: " << p << " exception: " << e << std::endl;
			return false;
		}
		return true;
	}

	std::vector<file_directory_info> NtixFileLib::read_directory(npath p) const
	{
		// the trailing '\' is harmless on a directory and required at a device root
		// I think we want to treat the user with respect and not correct their errors
		HANDLE hDir = open(p, FILE_LIST_DIRECTORY | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
			FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

		PBYTE buffer = (PBYTE)malloc(65536);
		if (!buffer) {
			close(hDir);
			throw nexception("NtixFileLib::read_directory malloc",STATUS_NO_MEMORY);
		}

		BOOLEAN restart = TRUE;
		ULONG retLen = 0;

		std::vector<file_directory_info> list;
		for (;;)
		{

			NTSTATUS status = query_directory_file(hDir, buffer, 65536, FileDirectoryInformation, restart, &retLen);
			restart = FALSE;

			if (status == STATUS_NO_MORE_FILES) break;

			if (!NT_SUCCESS(status))
			{
				free(buffer);
				close(hDir);
				throw nexception("NtixFileLib::read_directory::query_directory",status);
			}

			FILE_DIRECTORY_INFORMATION *pInfo = (FILE_DIRECTORY_INFORMATION *)buffer;
			while (pInfo) {
				nstring name = nstring(pInfo->FileName, pInfo->FileNameLength);
				file_directory_info entry = {
					name,
					pInfo->CreationTime.QuadPart,
					pInfo->LastAccessTime.QuadPart,
					pInfo->LastWriteTime.QuadPart,
					pInfo->ChangeTime.QuadPart,
					pInfo->EndOfFile.QuadPart,
					pInfo->AllocationSize.QuadPart,
					pInfo->FileAttributes
				};
				list.push_back(entry);
				if (pInfo->NextEntryOffset == 0) break;
                pInfo = (FILE_DIRECTORY_INFORMATION *)((PBYTE)pInfo + pInfo->NextEntryOffset);
			}
		}
		free(buffer);
		close(hDir);
		return list;
	}

	UINT64 NtixFileLib::disk_size(HANDLE hDevice) const {

		GET_LENGTH_INFORMATION lengthInfo;
		IO_STATUS_BLOCK isb;

		NTSTATUS status = device_ioctl_file(hDevice, NULL, NULL, NULL, &isb, IOCTL_DISK_GET_LENGTH_INFO,
			NULL, 0, &lengthInfo, sizeof(lengthInfo));

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::disk_size", status);

		return (UINT64)lengthInfo.Length.QuadPart;
	}

	std::vector<nstring> NtixFileLib::mounts() const {

		npath mpm("\\Device\\MountPointManager");

		HANDLE hMpm = open(mpm, SYNCHRONIZE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT);

		MOUNTMGR_MOUNT_POINT inputParam = { 0 };

		MOUNTMGR_MOUNT_POINTS*  mp = (MOUNTMGR_MOUNT_POINTS*) malloc(4096);

		IO_STATUS_BLOCK isb;

		NTSTATUS status = device_ioctl_file(hMpm, NULL, NULL, NULL, &isb, IOCTL_MOUNTMGR_QUERY_POINTS,
			&inputParam, sizeof(inputParam), mp, 4096);

		if (!NT_SUCCESS(status))
		{
			free(mp);
			close(hMpm);
			throw nexception("NtixFileLib::mounts device_ioctl_file", status);
		}

		std::vector<nstring> mlist;
		for (ULONG i=0; i<mp->NumberOfMountPoints; i++)
		{
			MOUNTMGR_MOUNT_POINT mmp = mp->MountPoints[i];

			nstring symbolicLink((WCHAR*)((char*)mp + mmp.SymbolicLinkNameOffset), mmp.SymbolicLinkNameLength);
			mlist.push_back(symbolicLink);
		}

		free(mp);
		return mlist;
	}

	void NtixFileLib::write_reparse(npath p, ULONG ReparseTag, USHORT ReparseDataLength, unsigned char* ReparseData) const
	{
		REPARSE_DATA_BUFFER* rdb = (REPARSE_DATA_BUFFER*)malloc(sizeof(ULONG) + sizeof(USHORT) + sizeof(USHORT) + ReparseDataLength);
		if (!rdb)
			throw nexception("NtixFileLib::write_reparse",STATUS_NO_MEMORY);

		rdb->ReparseTag = ReparseTag;
		rdb->ReparseDataLength = ReparseDataLength;
		rdb->Reserved = 0;

		memcpy(rdb->GenericReparseBuffer.DataBuffer,ReparseData,ReparseDataLength);

		NTSTATUS status;
		try {
			HANDLE hDir = open(p, FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
									FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT,0);


			IO_STATUS_BLOCK isb;

			status = fscontrol_file(hDir, NULL, NULL, NULL, &isb, FSCTL_SET_REPARSE_POINT, rdb, rdb->ReparseDataLength + 8, NULL, 0);
			close(hDir);

		} catch (nexception& e) {
			free(rdb);
			throw e;
		}

		free(rdb);

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::write_reparse fscontrol_file",status);

	}

	void NtixFileLib::rename(npath sp, npath dp, BOOLEAN replace)
	{

		// Open the source file/directory.
		// We need DELETE access to rename it, and SYNCHRONIZE for synchronous IO.
		// We share EVERYTHING (Read, Write, Delete) so we don't conflict with other handles.
		HANDLE hFile = open(sp, DELETE | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		FILE_RENAME_INFORMATION* fri;

		size_t wSize = sp.length()*sizeof(WCHAR);
		ULONG friSize = sizeof(FILE_RENAME_INFORMATION) + wSize;
		fri = (FILE_RENAME_INFORMATION *)malloc(friSize);

		fri->ReplaceIfExists = replace;
		fri->RootDirectory = NULL;
		fri->FileNameLength = sp.length()*sizeof(WCHAR);
		//fri.FileName = &sp.nstr().wc_str();

		memcpy(fri->FileName, &sp.nstr().wc_str() , wSize );

		IO_STATUS_BLOCK isb;
		NTSTATUS status = set_information_file(hFile, &isb, &fri, friSize, FileRenameInformation);

		free(fri);
		close(hFile);
		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::rename set_information_file",status);
	}

}
