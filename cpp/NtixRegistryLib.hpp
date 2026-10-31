#ifndef NTIX_REGISTRY_HPP
#define NTIX_REGISTRY_HPP

#include <string>
#include <vector>

#include "NtixLoader.hpp"
#include "nstring.hpp"
#include "npath.hpp"

// the information classes and structs are only in ddk/wdm.h

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyNameInformation,
	KeyCachedInformation,
	KeyFlagsInformation,
	KeyVirtualizationInformation,
	KeyHandleTagsInformation,
	KeyTrustInformation,
	KeyLayerInformation,
	MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64,
	KeyValueLayerInformation,
	MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_BASIC_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG TitleIndex;
	ULONG NameLength;
	WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG TitleIndex;
	ULONG ClassOffset;
	ULONG ClassLength;
	ULONG SubKeys;
	ULONG MaxNameLen;
	ULONG MaxClassLen;
	ULONG Values;
	ULONG MaxValueNameLen;
	ULONG MaxValueDataLen;
	WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
	ULONG NameLength;
	WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataOffset;  // from the start of this struct
	ULONG DataLength;
	ULONG NameLength;
	WCHAR Name[1];
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataLength;
	UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

#define NTDLL_REGISTRY_EXPORTS \
	X(NtClose, NTSTATUS, (HANDLE)) \
	X(NtOpenKeyEx, NTSTATUS, (PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, ULONG)) \
	X(NtQueryKey, NTSTATUS, (HANDLE, ULONG, PVOID, ULONG, PULONG)) \
	X(NtEnumerateKey, NTSTATUS, (HANDLE, ULONG, ULONG, PVOID, ULONG, PULONG)) \
	X(NtEnumerateValueKey, NTSTATUS, (HANDLE, ULONG, ULONG, PVOID, ULONG, PULONG)) \
	X(NtQueryValueKey, NTSTATUS, (HANDLE, PUNICODE_STRING, ULONG, PVOID, ULONG, PULONG))

#define X(name, ret, args) typedef ret (NTAPI *_##name##_t) args;
NTDLL_REGISTRY_EXPORTS
#undef X

namespace ntix {

class npath;

// a subkey, as returned by enumeration
typedef struct key_entry_t {
	nstring name;
	LONGLONG wtime;
} key_entry;

// a key, as returned by NtQueryKey on the key itself
typedef struct key_info_t {
	LONGLONG wtime;
	ULONG subkeys;
	ULONG values;
} key_info;

// values are not objects: they have no path, only a name relative to an open key.
// the name may be empty (the default value) and may contain '\'.
typedef struct value_info_t {
	nstring name;
	ULONG type;
	ULONG size;
	std::vector<BYTE> data; // empty unless data was requested
} value_info;

	class NtixRegistryLib {
	private:

#define X(name, ret, args) _##name##_t _##name = nullptr;
NTDLL_REGISTRY_EXPORTS
#undef X

		NtixRegistryLib();  // no one else can create one
		~NtixRegistryLib(); // prevent accidental deletion

		static NtixRegistryLib* ptr;

		NTSTATUS query_key(HANDLE KeyHandle, ULONG KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) const;
		NTSTATUS enumerate_key(HANDLE KeyHandle, ULONG Index, ULONG KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) const;
		NTSTATUS enumerate_value_key(HANDLE KeyHandle, ULONG Index, ULONG KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) const;
		NTSTATUS query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) const;

	public:
		static NtixRegistryLib* get_instance();

		NtixRegistryLib(const NtixRegistryLib&) = delete;
		NtixRegistryLib(NtixRegistryLib&&) = delete;
		NtixRegistryLib& operator=(const NtixRegistryLib&) = delete;
		NtixRegistryLib& operator=(NtixRegistryLib&&) = delete;

		// if it throws and doesn't return NTSTATUS, it's public

		void close(HANDLE h) const;

		// oa_flags OBJ_OPENLINK opens a link key itself rather than its target.
		// open_options REG_OPTION_BACKUP_RESTORE uses SeBackupPrivilege (must be enabled).
		HANDLE open_key(npath p, ACCESS_MASK am, ULONG oa_flags = 0, ULONG open_options = 0) const;

		// "Key" or "SymbolicLink"; throws if p is not a key
		const nstring get_type(npath p) const;
		bool is_symlink(npath p) const;
		const npath get_symbolic_link_path(npath p) const;
		// the key's own name for itself, after following links
		const npath get_canonical_path(npath p) const;
		key_info get_info(npath p) const;

		std::vector<key_entry> read_keys(npath p) const;
		std::vector<value_info> read_values(npath p, bool with_data = false) const;
		value_info read_value(npath p, const nstring& name) const;
		value_info read_value(HANDLE h, const nstring& name) const;

		static const nstring type_str(ULONG type);
	};
}
#endif
