#ifndef NTIX_FILE_HPP
#define NTIX_FILE_HPP

#include <string>

#include "NtixCoreLib.hpp"
#include "NtixLoader.hpp"
#include "nstring.hpp"
#include "npath.hpp"

#include <windows.h> // For HANDLE, NTSTATUS, etc.
#include <winternl.h>

#define IOCTL_MOUNTMGR_QUERY_POINTS  0x006D0008
#define IOCTL_NSI_PROXY_EXECUTE_OPERATION 0x0012001B

// The specific module GUID used by the TCP protocol provider driver inside the kernel
const GUID NSI_TCP_MODULE_GUID = { 0xEB047A03, 0xB1F9, 0x472F, { 0xA7, 0x2E, 0x93, 0x4D, 0xEC, 0xC5, 0x36, 0xC9 } };

// Structure definitions representing the internal tracking classes
enum NSI_STRUCT_TYPE {
	NSI_STRUCT_TCP_ALL = 3 // Tells the provider to return full tracking tables (including PIDs)
};

// \Device\Tcp
// \Device\Nsi

typedef struct _NSI_PROXY_QUERY_PARAMETERS {
	ULONG_PTR Unknown1;
	ULONG_PTR Unknown2;
	GUID ModuleId;              // Pass NSI_TCP_MODULE_GUID
	ULONG StructType;           // Pass NSI_STRUCT_TYPE entry (3)
	ULONG Unknown3;
	ULONG Unknown4;
	PVOID QueryOutputBuffer;    // Pointer to destination table array receiving socket data
	SIZE_T OutputBufferSize;    // Allocation size for table array response mapping
	PVOID QueryOutputBuffer2;
	SIZE_T OutputBufferSize2;
	PVOID QueryOutputBuffer3;   // Array containing connection tracking statistics
	SIZE_T OutputBufferSize3;
	PVOID PidTableBuffer;       // Pointer to destination array receiving target PIDs mapping
	SIZE_T PidTableBufferSize;  // Allocation size of the PID tracker destination
} NSI_PROXY_QUERY_PARAMETERS;

typedef struct _MOUNTMGR_MOUNT_POINT {
  ULONG  SymbolicLinkNameOffset;
  USHORT SymbolicLinkNameLength;
  USHORT Reserved1;
  ULONG  UniqueIdOffset;
  USHORT UniqueIdLength;
  USHORT Reserved2;
  ULONG  DeviceNameOffset;
  USHORT DeviceNameLength;
  USHORT Reserved3;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _REPARSE_DATA_BUFFER {
	ULONG ReparseTag;
	USHORT ReparseDataLength;
	USHORT Reserved;
	union {
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			ULONG Flags;
			WCHAR PathBuffer[1];
		} SymbolicLinkReparseBuffer;
		struct {
			USHORT SubstituteNameOffset;
			USHORT SubstituteNameLength;
			USHORT PrintNameOffset;
			USHORT PrintNameLength;
			WCHAR PathBuffer[1];
		} MountPointReparseBuffer;
		struct {
			UCHAR DataBuffer[1];
		} GenericReparseBuffer;
	};
} REPARSE_DATA_BUFFER;

typedef struct _MOUNTMGR_MOUNT_POINTS {
  ULONG                Size;
  ULONG                NumberOfMountPoints;
  MOUNTMGR_MOUNT_POINT MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

#define NTDLL_FILE_EXPORTS \
	X(NtClose, NTSTATUS, (HANDLE)) \
	X(NtCreateFile, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PIO_STATUS_BLOCK, PLARGE_INTEGER, ULONG, ULONG, ULONG, ULONG, PVOID, ULONG)) \
	X(NtQueryDirectoryFile, NTSTATUS, (HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG, BOOLEAN, PUNICODE_STRING, BOOLEAN)) \
	X(NtDeviceIoControlFile, NTSTATUS, (HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, ULONG, PVOID, ULONG, PVOID, ULONG)) \
	X(NtFsControlFile, NTSTATUS, (HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, ULONG, PVOID, ULONG, PVOID, ULONG)) \
	X(NtSetInformationFile, NTSTATUS, (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG)) \
	X(NtQueryInformationFile, NTSTATUS, (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG)) \
	X(NtQueryVolumeInformationFile, NTSTATUS, (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, ULONG)) \
	X(NtReadFile, NTSTATUS, (HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG)) \
	X(NtWriteFile, NTSTATUS, (HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK, PVOID, ULONG, PLARGE_INTEGER, PULONG)) \
	X(NtQueryEaFile, NTSTATUS, (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, BOOLEAN, PVOID, ULONG, PULONG, BOOLEAN)) \
	X(NtSetEaFile, NTSTATUS, (HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG))

#define X(name, ret, args) typedef ret (NTAPI *_##name##_t) args;
NTDLL_FILE_EXPORTS
#undef X

namespace ntix {

class npath;

typedef struct file_directory_info_t {
	nstring name;
	// not doing file index because it's different to fileid which we use in overloaded open
	LONGLONG ctime;
	LONGLONG atime;
	LONGLONG wtime;
	LONGLONG mtime;
	LONGLONG size;
	LONGLONG asize;
	ULONG attrib;
} file_directory_info;

typedef struct mount_point_t
{
	nstring symlink;
} mount_point;

	class NtixFileLib {
	private:

#define X(name, ret, args) _##name##_t _##name = nullptr;
NTDLL_FILE_EXPORTS
#undef X

		NtixFileLib();  // no one else can create one
		~NtixFileLib(); // prevent accidental deletion

		static NtixFileLib* ptr;

		// NTSTATUS close_(HANDLE h) const;
		NTSTATUS create_file(PHANDLE FileHandle, ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
			PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess,
			ULONG CreateDisposition,ULONG CreateOptions, PVOID EaBuffer,
			ULONG EaLength) const;

		NTSTATUS query_directory_file(HANDLE hDir, PVOID buffer, ULONG bufferLength,
			ULONG infoClass, BOOLEAN restartScan, PULONG pReturnLength) const;

		//NTSTATUS device_ioctl_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
		//	ULONG IoControlCode, PVOID InputBuffer, ULONG InputBufferLength,
		//	PVOID OutputBuffer, ULONG OutputBufferLength) const;

		NTSTATUS fscontrol_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode,
			PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
			ULONG OutputBufferLength) const;

		NTSTATUS set_information_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
			PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const;

		NTSTATUS query_information_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
			PVOID FileInformation, ULONG Length, ULONG FileInformationClass) const;

		NTSTATUS query_volume_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
			PVOID FsInformation, ULONG Length, ULONG FsInformationClass) const;

		NTSTATUS device_ioctl_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock, ULONG IoControlCode,
			PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer,
			ULONG OutputBufferLength) const;

		NTSTATUS read_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
			PULONG Key) const;

		NTSTATUS write_file(HANDLE FileHandle, HANDLE Event, PVOID ApcRoutine, PVOID ApcContext,
			PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset,
			PULONG Key) const;

		NTSTATUS query_ea_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
			ULONG Length, BOOLEAN ReturnSingleEntry, PVOID EaList, ULONG EaListLength,
			PULONG EaIndex, BOOLEAN RestartScan) const;

		NTSTATUS set_ea_file(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer,
			ULONG Length) const;

	public:
		static NtixFileLib* get_instance();

		NtixFileLib(const NtixFileLib&) = delete;
		NtixFileLib(NtixFileLib&&) = delete;
		NtixFileLib& operator=(const NtixFileLib&) = delete;
		NtixFileLib& operator=(NtixFileLib&&) = delete;

// 1. What you want to do — DesiredAccess
// GENERIC_READ / GENERIC_WRITE
// FILE_READ_DATA, FILE_WRITE_DATA, FILE_APPEND_DATA
// DELETE                          ← needed for rename and delete-on-close
// FILE_READ_ATTRIBUTES            ← stat without read permission
// SYNCHRONIZE                     ← needed for synchronous I/O
// READ_CONTROL, WRITE_DAC         ← DACL operations
//
// 2. What others may do while you hold it — ShareAccess
// 0                    exclusive — no other opens
// FILE_SHARE_READ      others may read
// FILE_SHARE_WRITE     others may write
// FILE_SHARE_DELETE    others may rename or delete
// This is the inverse of what you'd expect to read: it's not your sharing mode, it's what you permit others.
//
// 3. Create or open? — CreateDisposition
// FILE_CREATE          fail if exists
// FILE_OPEN            fail if not exists
// FILE_OPEN_IF         create if not exists, open if exists
// FILE_OVERWRITE       fail if not exists, truncate if exists
// FILE_OVERWRITE_IF    create or truncate
// FILE_SUPERSEDE       replace entirely if exists
//
// 4. How to open — CreateOptions
// FILE_DIRECTORY_FILE         must be a directory
// FILE_NON_DIRECTORY_FILE     must not be a directory
// FILE_OPEN_REPARSE_POINT     stop at reparse point, do not follow
// FILE_OPEN_FOR_BACKUP_INTENT use SeBackupPrivilege bypass if available
// FILE_DELETE_ON_CLOSE        delete when last handle closes
// FILE_SYNCHRONOUS_IO_NONALERT synchronous I/O (needs SYNCHRONIZE in DesiredAccess)
// FILE_SEQUENTIAL_ONLY        hint: sequential access pattern

		const HANDLE open(npath p, ACCESS_MASK access, ULONG share, ULONG disposition, ULONG options, ULONG oa_flags = 0) const;
		const HANDLE open(ULONGLONG fileId, ACCESS_MASK access, ULONG share, ULONG disposition, ULONG options, ULONG oa_flags = 0) const;

		std::vector<file_directory_info> read_directory(npath p) const;

		void close(HANDLE h) const;
		bool exists(npath p) const;

		UINT64 disk_size(HANDLE hDevice) const;
		std::vector<nstring> mounts() const;
		void write_reparse(npath p, ULONG ReparseTag, USHORT ReparseDataLength, unsigned char* ReparseData) const;
		void rename(npath sp, npath dp, BOOLEAN replace);


	};
}
#endif
