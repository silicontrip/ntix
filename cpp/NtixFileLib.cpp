#include "NtixFileLib.hpp"
#include "NtixCoreLib.hpp"
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

	NTSTATUS NtixFileLib::create(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
		PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
		ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer,
		ULONG EaLength) const
	{

		return _NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
			AllocationSize, FileAttributes, ShareAccess, CreateDisposition, CreateOptions,
			EaBuffer, EaLength);
	}

	NTSTATUS NtixFileLib::query_directory(HANDLE hDir, PVOID buffer, ULONG bufferLength, ULONG infoClass, BOOLEAN restartScan, PULONG pReturnLength) const
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

	NTSTATUS NtixFileLib::fscontrol(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode,
		PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
		ULONG OutputBufferLength) const
	{
		return _NtFsControlFile (FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, FsControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

	NTSTATUS NtixFileLib::set_information(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const
	{
		return _NtSetInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
	}

	NTSTATUS NtixFileLib::query_information(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const
	{
		return _NtQueryInformationFile(FileHandle, IoStatusBlock, FileInformation, Length, FileInformationClass);
	}

	NTSTATUS NtixFileLib::query_volume(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		PVOID FsInformation, ULONG Length, ULONG FsInformationClass) const
	{
		return _NtQueryVolumeInformationFile(FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass);
	}

	NTSTATUS NtixFileLib::device_ioctl(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode,
		PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
		ULONG OutputBufferLength) const
	{
		return _NtDeviceIoControlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode,
			InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

	NTSTATUS NtixFileLib::read(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
		PULONG Key) const
	{
		return _NtReadFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
	}

	NTSTATUS NtixFileLib::write(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
		PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
		PULONG Key) const
	{
		return _NtWriteFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, Buffer, Length, ByteOffset, Key);
	}

	NTSTATUS NtixFileLib::query_ea(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
		ULONG Length, BOOLEAN ReturnSingleEntry, PVOID EaList, ULONG EaListLength,
		PULONG EaIndex, BOOLEAN RestartScan) const
	{
		return _NtQueryEaFile(FileHandle, IoStatusBlock, Buffer, Length, ReturnSingleEntry, EaList, EaListLength, EaIndex, RestartScan);
	}

	NTSTATUS NtixFileLib::set_ea(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length) const
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
			// std::cerr << "DEBUG: NtixFileLib::exists path: " << p << " exception: " << e << std::endl;
			return false;
		}
		return true;
	}

	bool NtixFileLib::dir_exists(npath p) const
	{
		try {
			HANDLE hFile = open(p, SYNCHRONIZE | FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 0);
			close(hFile);
		} catch (nexception& e) {
			// we may want to throw on some exceptions
			// std::cerr << "DEBUG: NtixFileLib::dir_exists path: " << p << " exception: " << e << std::endl;
			return false;
		}
		return true;
	}

	bool NtixFileLib::is_fs(npath p) const
	{
		try {
			HANDLE hFs = open(p, SYNCHRONIZE | FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 0);
			IO_STATUS_BLOCK isb;
			FILE_FS_ATTRIBUTE_INFORMATION ffai;
			NTSTATUS s = query_volume(hFs, &isb, &ffai, sizeof(FILE_FS_ATTRIBUTE_INFORMATION), FileFsAttributeInformation);
			close (hFs);
			if (s!=STATUS_BUFFER_OVERFLOW && !NT_SUCCESS(s))
			{
				//nexception e("NtixFileLib::is_fs",s);
				//std::cerr << "DEBUG: NtixFileLib::is_fs query_volume: " << p << " exception: " << e << std::endl;
				return false;
			}
		} catch (nexception& e) {
			// we may want to throw on some exceptions
			//std::cerr << "DEBUG: NtixFileLib::is_fs open: " << p << " exception: " << e << std::endl;
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

		std::vector<BYTE> buffer(65536);

		BOOLEAN restart = TRUE;
		ULONG retLen = 0;

		std::vector<file_directory_info> list;
		for (;;)
		{

			// possible look into;
			// FileIdBothDirectoryInformation / FileIdExtdDirectoryInformation

			NTSTATUS status = query_directory(hDir, buffer.data(), (ULONG)buffer.size(), FileDirectoryInformation, restart, &retLen);
			restart = FALSE;

			if (status == STATUS_NO_MORE_FILES) break;

			if (!NT_SUCCESS(status))
			{
				close(hDir);
				throw nexception("NtixFileLib::read_directory::query_directory",status);
			}

			FILE_DIRECTORY_INFORMATION *pInfo = (FILE_DIRECTORY_INFORMATION *)buffer.data();
			while (pInfo) {
				nstring name = nstring(pInfo->FileName, pInfo->FileNameLength);
				//std::cerr << "NtixFileLib::read_directory DEBUG: adding: " << name << std::endl;
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
		close(hDir);
		return list;
	}

	UINT64 NtixFileLib::disk_size(HANDLE hDevice) const {

		GET_LENGTH_INFORMATION lengthInfo;
		IO_STATUS_BLOCK isb;

		NTSTATUS status = device_ioctl(hDevice, NULL, NULL, NULL, &isb, IOCTL_DISK_GET_LENGTH_INFO,
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

		std::vector<BYTE> buffer(4096);
		MOUNTMGR_MOUNT_POINTS* mp = (MOUNTMGR_MOUNT_POINTS*)buffer.data();

		IO_STATUS_BLOCK isb;

		NTSTATUS status = device_ioctl(hMpm, NULL, NULL, NULL, &isb, IOCTL_MOUNTMGR_QUERY_POINTS,
			&inputParam, sizeof(inputParam), mp, (ULONG)buffer.size());

		close(hMpm);

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::mounts device_ioctl", status);


		std::vector<nstring> mlist;
		for (ULONG i=0; i<mp->NumberOfMountPoints; i++)
		{
			MOUNTMGR_MOUNT_POINT mmp = mp->MountPoints[i];

			nstring symbolicLink((WCHAR*)((char*)mp + mmp.SymbolicLinkNameOffset), mmp.SymbolicLinkNameLength);
			mlist.push_back(symbolicLink);
		}

		return mlist;
	}

	void NtixFileLib::dismount(npath dev) const
	{

	    HANDLE hDevice = open(dev, FILE_READ_DATA | SYNCHRONIZE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT);

		IO_STATUS_BLOCK isb;

		NTSTATUS status = device_ioctl(hDevice, NULL, NULL, NULL, &isb, FSCTL_DISMOUNT_VOLUME,
            NULL, 0, NULL, 0);

		close(hDevice);

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::dismount device_ioctl", status);

	}

	void NtixFileLib::rename(npath sp, npath dp, BOOLEAN replace)
	{

		// Open the source file/directory.
		// We need DELETE access to rename it, and SYNCHRONIZE for synchronous IO.
		// We share EVERYTHING (Read, Write, Delete) so we don't conflict with other handles.
		HANDLE hFile = open(sp, DELETE | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		// dp.length() is UTF-8 bytes; the size must come from the wide string
		// const std::wstring& wname = dp.nstr().wc_str();
		// ULONG wSize = (ULONG)dp.wsize();
		ULONG friSize = sizeof(FILE_RENAME_INFORMATION) + dp.wsize();
		std::vector<BYTE> buffer(friSize);
		FILE_RENAME_INFORMATION* fri = (FILE_RENAME_INFORMATION *)buffer.data();

		fri->ReplaceIfExists = replace;
		fri->RootDirectory = NULL;
		fri->FileNameLength = dp.wsize();

		memcpy(fri->FileName, dp.wdata(), dp.wsize());

		IO_STATUS_BLOCK isb;
		NTSTATUS status = set_information(hFile, &isb, fri, friSize, FileRenameInformation);

		close(hFile);
		if (!NT_SUCCESS(status))
		{
			std::cerr << "s: " << sp << " -> " << dp << std::endl;
			throw nexception("NtixFileLib::rename set_information ",status);
		}
	}

	// NT has no per-file handle count: ObjectBasicInformation.HandleCount is per FILE_OBJECT,
	// and every NtCreateFile makes a new one, so it's always 1 on our own handle.
	// FileProcessIdsUsingFileInformation asks the file system which processes have it open.
	std::vector<ULONG_PTR> NtixFileLib::processes_using(npath p) const
	{
		HANDLE hFile = open(p, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		std::vector<BYTE> buffer(sizeof(FILE_PROCESS_IDS_USING_FILE_INFORMATION) + 63 * sizeof(ULONG_PTR));
		IO_STATUS_BLOCK isb;
		NTSTATUS status;
		for (;;)
		{
			status = query_information(hFile, &isb, buffer.data(), (ULONG)buffer.size(), FileProcessIdsUsingFileInformation);
			if (status != STATUS_INFO_LENGTH_MISMATCH && status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
				break;
			buffer.resize(buffer.size() * 2);
		}
		close(hFile);
		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::processes_using query_information", status);

		// our own query handle may be in the list
		ULONG_PTR self = NtixCoreLib::get_instance()->process_id();

		FILE_PROCESS_IDS_USING_FILE_INFORMATION* info = (FILE_PROCESS_IDS_USING_FILE_INFORMATION*)buffer.data();
		std::vector<ULONG_PTR> pids;
		for (ULONG i = 0; i < info->NumberOfProcessIdsInList; i++)
			if (info->ProcessIdList[i] != self)
				pids.push_back(info->ProcessIdList[i]);
		return pids;
	}

	const nstring NtixFileLib::get_type(npath p) const
	{

		file_directory_info f = get_info(p);

		ULONG type = f.attrib;

		if ((type & 0x410) == 0x410)
			return "Reparse-Directory";

		if (type & 0x400)
			return "Reparse";

		if (type & 0x10)
			return "Directory";

		return "File";

	}

	file_directory_info NtixFileLib::get_info(npath p) const
	{
		HANDLE hFile = open(p, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN, FILE_OPEN_REPARSE_POINT |  FILE_OPEN_FOR_BACKUP_INTENT, 0);

		FILE_NETWORK_OPEN_INFORMATION fbi;
		IO_STATUS_BLOCK isb;

		NTSTATUS s = query_information(hFile, &isb, &fbi, sizeof(FILE_NETWORK_OPEN_INFORMATION), FileNetworkOpenInformation);
		close(hFile);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::get_info query_information",s);

		return {
			p.nstr(),
			fbi.CreationTime.QuadPart,
			fbi.LastAccessTime.QuadPart,
			fbi.LastWriteTime.QuadPart,
			fbi.ChangeTime.QuadPart,
			fbi.EndOfFile.QuadPart,
			fbi.AllocationSize.QuadPart,
			fbi.FileAttributes
		};
	}

	void NtixFileLib::set_info(npath p, file_directory_info f) const
	{
		HANDLE hFile = open(p, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN, FILE_OPEN_REPARSE_POINT |  FILE_OPEN_FOR_BACKUP_INTENT, 0);

		// FileNetworkOpenInformation is query-only; FileBasicInformation is the settable
		// subset (times + attributes). size/asize are ignored -- those are
		// FileEndOfFileInformation / FileAllocationInformation and need FILE_WRITE_DATA.
		// A time of 0 means "leave unchanged", so get_info -> tweak -> set_info round-trips.
		FILE_BASIC_INFORMATION fbi;
		IO_STATUS_BLOCK isb;

		fbi.CreationTime.QuadPart = f.ctime;
		fbi.LastAccessTime.QuadPart = f.atime;
		fbi.LastWriteTime.QuadPart = f.wtime;   // get_info stores LastWriteTime in wtime
		fbi.ChangeTime.QuadPart = f.mtime;      // and ChangeTime in mtime

		// only these bits are settable (FILE_ATTRIBUTE_VALID_SET_FLAGS); passing the
		// directory/reparse/compressed bits back can fail with STATUS_INVALID_PARAMETER
		ULONG attrib = f.attrib & 0x31a7;
		// 0 means "leave unchanged", so clearing the last attribute (e.g. read-only)
		// has to be sent as FILE_ATTRIBUTE_NORMAL
		fbi.FileAttributes = attrib ? attrib : FILE_ATTRIBUTE_NORMAL;

		NTSTATUS s = set_information(hFile, &isb, &fbi, sizeof(FILE_BASIC_INFORMATION), FileBasicInformation);
		close(hFile);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::set_info set_information",s);
	}

	// POSIX semantics unlink the name immediately, even while other processes hold it
	// open (with FILE_SHARE_DELETE); the data goes when the last handle closes.
	// Falls back to classic delete-on-last-close where the Ex class isn't supported
	// (pre-1709 Windows, FAT, many network redirectors).
	void NtixFileLib::delete_file(npath p, bool ignore_readonly) const
	{
		// FILE_OPEN_REPARSE_POINT: delete the link itself, never its target
		HANDLE hFile = open(p, DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		IO_STATUS_BLOCK isb;
		FILE_DISPOSITION_INFO_EX fdie;
		fdie.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS;
		if (ignore_readonly)
			fdie.Flags |= FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;

		NTSTATUS s = set_information(hFile, &isb, &fdie, sizeof(fdie), FileDispositionInformationEx);

		if (s == STATUS_INVALID_INFO_CLASS || s == STATUS_INVALID_PARAMETER || s == STATUS_NOT_SUPPORTED)
		{
			// no ignore-readonly here: a read-only file fails with STATUS_CANNOT_DELETE,
			// the caller clears the attribute with set_info and retries
			FILE_DISPOSITION_INFORMATION fdi;
			fdi.DoDeleteFile = TRUE;
			s = set_information(hFile, &isb, &fdi, sizeof(fdi), FileDispositionInformation);
		}
		close(hFile);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::delete_file set_information", s);
	}

	ULONG NtixFileLib::read_data(HANDLE h, void* buf, ULONG len) const
	{
		IO_STATUS_BLOCK isb = {};
		NTSTATUS s = read(h, NULL, NULL, NULL, &isb, buf, len, NULL, NULL);
		if (s == STATUS_END_OF_FILE)
			return 0;
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::read_data", s);
		return (ULONG)isb.Information;
	}

	void NtixFileLib::write_data(HANDLE h, const void* buf, ULONG len) const
	{
		IO_STATUS_BLOCK isb = {};
		NTSTATUS s = write(h, NULL, NULL, NULL, &isb, const_cast<void*>(buf), len, NULL, NULL);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::write_data", s);
	}

	std::vector<BYTE> NtixFileLib::read_ea(npath p) const
	{
		HANDLE h = open(p, FILE_READ_EA | SYNCHRONIZE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);

		std::vector<BYTE> buf(65536);
		IO_STATUS_BLOCK isb = {};
		NTSTATUS s;

		for (;;) {
			s = query_ea(h, &isb, buf.data(), (ULONG)buf.size(), FALSE, NULL, 0, NULL, TRUE);
			// No EAs on this file
			if (s == 0x80000012L || s == 0xC0000225L) { // STATUS_NO_EAS_ON_FILE / STATUS_NOT_FOUND
				close(h);
				return {};
			}
			if (s != STATUS_BUFFER_OVERFLOW && s != 0xC0000023L) // STATUS_BUFFER_TOO_SMALL
				break;
			buf.resize(buf.size() * 2);
		}

		close(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::read_ea", s);

		buf.resize(isb.Information);
		return buf;
	}

	void NtixFileLib::write_ea(npath p, const std::vector<BYTE>& ea) const
	{
		if (ea.empty()) return;
		HANDLE h = open(p, FILE_WRITE_EA | SYNCHRONIZE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT);
		IO_STATUS_BLOCK isb = {};
		NTSTATUS s = set_ea(h, &isb, const_cast<BYTE*>(ea.data()), (ULONG)ea.size());
		close(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::write_ea", s);
	}

	reparse_info NtixFileLib::read_reparse(npath p) const
	{
		HANDLE h = open(p, FILE_READ_DATA | SYNCHRONIZE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		IO_STATUS_BLOCK isb;
		std::vector<BYTE> buffer(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
		REPARSE_DATA_BUFFER* rdb = (REPARSE_DATA_BUFFER*)buffer.data();

		NTSTATUS s = fscontrol(h, NULL, NULL, NULL, &isb, FSCTL_GET_REPARSE_POINT,
			NULL, 0, rdb, (ULONG)buffer.size());
		close(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixFileLib::read_reparse", s);

		reparse_info r;
		r.raw.assign(buffer.begin(), buffer.begin() + 8 + rdb->ReparseDataLength);
		return r;
	}

	void NtixFileLib::write_reparse(npath p, reparse_info rp) const
	{
		HANDLE hDir = open(p, FILE_WRITE_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							FILE_OPEN, FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT, 0);

		IO_STATUS_BLOCK isb;
		NTSTATUS status = fscontrol(hDir, NULL, NULL, NULL, &isb, FSCTL_SET_REPARSE_POINT,
			rp.raw.data(), (ULONG)rp.raw.size(), NULL, 0);
		close(hDir);

		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::write_reparse fscontrol", status);
	}

	void NtixFileLib::create_file(npath p) const
	{
		HANDLE h = open(p, FILE_WRITE_DATA | SYNCHRONIZE, 0, FILE_CREATE,
			FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT);
		close(h);
	}

	void NtixFileLib::create_directory(npath p) const
	{
		HANDLE h = open(p, FILE_LIST_DIRECTORY | SYNCHRONIZE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_CREATE,
			FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT);
		close(h);
	}

	void NtixFileLib::create_hardlink(npath src, npath dest)
	{
		// FILE_LINK_INFORMATION has identical layout to FILE_RENAME_INFORMATION
		HANDLE hFile = open(src, FILE_READ_ATTRIBUTES | SYNCHRONIZE,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			FILE_OPEN, FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT, 0);

		ULONG fliSize = sizeof(FILE_RENAME_INFORMATION) + dest.wsize();
		std::vector<BYTE> buffer(fliSize, 0);
		FILE_RENAME_INFORMATION* fli = (FILE_RENAME_INFORMATION*)buffer.data();
		fli->ReplaceIfExists = FALSE;
		fli->RootDirectory = NULL;
		fli->FileNameLength = dest.wsize();
		memcpy(fli->FileName, dest.wdata(), dest.wsize());

		IO_STATUS_BLOCK isb;
		NTSTATUS status = set_information(hFile, &isb, fli, fliSize, FileLinkInformation);
		close(hFile);
		if (!NT_SUCCESS(status))
			throw nexception("NtixFileLib::create_hardlink", status);
	}

	reparse_info NtixFileLib::reparse_symlink(nstring target, nstring display, bool relative) const
	{
		USHORT subLen = (USHORT)target.wsize();
		USHORT printLen = (USHORT)display.wsize();
		// SymbolicLinkReparseBuffer fixed fields: 4×USHORT + ULONG = 12 bytes
		USHORT dataLen = 4 * sizeof(USHORT) + sizeof(ULONG) + subLen + printLen;

		reparse_info r;
		r.raw.resize(8 + dataLen, 0);

		REPARSE_DATA_BUFFER* rdb = r.rdb();
		rdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;
		rdb->ReparseDataLength = dataLen;
		rdb->Reserved = 0;

		auto& slrb = rdb->SymbolicLinkReparseBuffer;
		slrb.SubstituteNameOffset = 0;
		slrb.SubstituteNameLength = subLen;
		slrb.PrintNameOffset = subLen;
		slrb.PrintNameLength = printLen;
		slrb.Flags = relative ? 1 : 0;
		memcpy(slrb.PathBuffer, target.wdata(), subLen);
		memcpy((BYTE*)slrb.PathBuffer + subLen, display.wdata(), printLen);

		return r;
	}

	reparse_info NtixFileLib::reparse_junction(npath dest, npath target) const
	{
		create_directory(dest);

		USHORT subLen = (USHORT)target.wsize();
		// MountPointReparseBuffer fixed fields: 4×USHORT = 8 bytes; print name empty for junctions
		USHORT dataLen = 4 * sizeof(USHORT) + subLen;

		reparse_info r;
		r.raw.resize(8 + dataLen, 0);

		REPARSE_DATA_BUFFER* rdb = r.rdb();
		rdb->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
		rdb->ReparseDataLength = dataLen;
		rdb->Reserved = 0;

		auto& mprb = rdb->MountPointReparseBuffer;
		mprb.SubstituteNameOffset = 0;
		mprb.SubstituteNameLength = subLen;
		mprb.PrintNameOffset = subLen;
		mprb.PrintNameLength = 0;
		memcpy(mprb.PathBuffer, target.wdata(), subLen);

		return r;
	}

	nstring reparse_info::sub_name() const
	{
		if (rdb()->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
			const auto& buf = rdb()->MountPointReparseBuffer;
			return nstring(buf.PathBuffer + buf.SubstituteNameOffset / sizeof(WCHAR), buf.SubstituteNameLength);
		}
		if (rdb()->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
			const auto& buf = rdb()->SymbolicLinkReparseBuffer;
			return nstring(buf.PathBuffer + buf.SubstituteNameOffset / sizeof(WCHAR), buf.SubstituteNameLength);
		}
		throw nexception("reparse_info::sub_name", 0xC0000278L); // STATUS_IO_REPARSE_TAG_MISMATCH
	}

	nstring reparse_info::print_name() const
	{
		if (rdb()->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
			const auto& buf = rdb()->MountPointReparseBuffer;
			return nstring(buf.PathBuffer + buf.PrintNameOffset / sizeof(WCHAR), buf.PrintNameLength);
		}
		if (rdb()->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
			const auto& buf = rdb()->SymbolicLinkReparseBuffer;
			return nstring(buf.PathBuffer + buf.PrintNameOffset / sizeof(WCHAR), buf.PrintNameLength);
		}
		throw nexception("reparse_info::print_name", 0xC0000278L); // STATUS_IO_REPARSE_TAG_MISMATCH
	}


}
