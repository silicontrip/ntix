#include "NtixRegistryLib.hpp"
#include "NtixCoreLib.hpp"
#include "nexception.hpp"

#include <sstream>

namespace ntix {
	NtixRegistryLib* NtixRegistryLib::ptr = 0;

	NtixRegistryLib::NtixRegistryLib()
	{
		// do the ntdll loader
		PVOID hNtDll = GetModuleBase(L"ntdll.dll");

#define X(name, ret, args) _##name = (_##name##_t)GetProcAddressNative(hNtDll, #name);
NTDLL_REGISTRY_EXPORTS
#undef X

	}

	NtixRegistryLib* NtixRegistryLib::get_instance()
	{
		if (!ptr)
			ptr = new NtixRegistryLib();
		return ptr;
	}

// Nt* wrappers, private, return NTSTATUS, do not throw.

	NTSTATUS NtixRegistryLib::query_key(HANDLE KeyHandle, ULONG KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) const
	{
		return _NtQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
	}

	NTSTATUS NtixRegistryLib::enumerate_key(HANDLE KeyHandle, ULONG Index, ULONG KeyInformationClass, PVOID KeyInformation, ULONG Length, PULONG ResultLength) const
	{
		return _NtEnumerateKey(KeyHandle, Index, KeyInformationClass, KeyInformation, Length, ResultLength);
	}

	NTSTATUS NtixRegistryLib::enumerate_value_key(HANDLE KeyHandle, ULONG Index, ULONG KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) const
	{
		return _NtEnumerateValueKey(KeyHandle, Index, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
	}

	NTSTATUS NtixRegistryLib::query_value_key(HANDLE KeyHandle, PUNICODE_STRING ValueName, ULONG KeyValueInformationClass, PVOID KeyValueInformation, ULONG Length, PULONG ResultLength) const
	{
		return _NtQueryValueKey(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
	}

// semantic calls, public, return a value, throw on NTSTATUS error

// The registry query calls all report a short buffer the same way: STATUS_BUFFER_OVERFLOW
// (fixed part filled, variable part truncated) or STATUS_BUFFER_TOO_SMALL (nothing filled),
// with ResultLength set to the size needed. Every caller below grows and retries on either.

	void NtixRegistryLib::close(HANDLE h) const
	{
		NTSTATUS s = _NtClose(h);
		if (!NT_SUCCESS(s))
			throw nexception("NtixRegistryLib::close",s);
	}

	HANDLE NtixRegistryLib::open_key(npath p, ACCESS_MASK am, ULONG oa_flags, ULONG open_options) const
	{
		// p is our own copy, so its oa() cache is fresh and oa_flags will be applied
		HANDLE hKey;
		NTSTATUS s = _NtOpenKeyEx(&hKey, am, &p.oa(oa_flags), open_options);
		if (!NT_SUCCESS(s))
			throw nexception("NtixRegistryLib::open_key",s);
		return hKey;
	}

	// a link key is an ordinary key with the link flag set at creation, and a REG_LINK
	// value named SymbolicLinkValue holding the target. Opening it without OBJ_OPENLINK
	// opens the target instead, so every link test has to open with OBJ_OPENLINK.
	const nstring NtixRegistryLib::get_type(npath p) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE, OBJ_OPENLINK);
		nstring type("Key");
		try {
			if (read_value(hKey, nstring("SymbolicLinkValue")).type == REG_LINK)
				type = "SymbolicLink";
		} catch (nexception& e) {
			if (e.status() != STATUS_OBJECT_NAME_NOT_FOUND)
			{
				close(hKey);
				throw e;
			}
		}
		close(hKey);
		return type;
	}

	bool NtixRegistryLib::is_symlink(npath p) const
	{
		return get_type(p) == "SymbolicLink";
	}

	const npath NtixRegistryLib::get_symbolic_link_path(npath p) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE, OBJ_OPENLINK);
		std::vector<BYTE> data;
		try {
			value_info v = read_value(hKey, nstring("SymbolicLinkValue"));
			if (v.type != REG_LINK)
				throw nexception("NtixRegistryLib::get_symbolic_link_path not REG_LINK", STATUS_OBJECT_TYPE_MISMATCH);
			data = v.data;
		} catch (nexception& e) {
			close(hKey);
			// match NtOpenSymbolicLinkObject on something that isn't a link
			if (e.status() == STATUS_OBJECT_NAME_NOT_FOUND)
				throw nexception("NtixRegistryLib::get_symbolic_link_path", STATUS_OBJECT_TYPE_MISMATCH);
			throw e;
		}
		close(hKey);

		// REG_LINK is an NT path, counted, not null terminated
		return npath(nstring((const WCHAR*)data.data(), (ULONG)data.size()));
	}

	const npath NtixRegistryLib::get_canonical_path(npath p) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE);

		std::vector<BYTE> buffer(sizeof(KEY_NAME_INFORMATION) + 512);
		ULONG retLen = 0;
		NTSTATUS status;
		for (;;)
		{
			status = query_key(hKey, KeyNameInformation, buffer.data(), (ULONG)buffer.size(), &retLen);
			if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
				break;
			buffer.resize(retLen > buffer.size() ? retLen : buffer.size() * 2);
		}
		close(hKey);
		if (!NT_SUCCESS(status))
			throw nexception("NtixRegistryLib::get_canonical_path query_key", status);

		KEY_NAME_INFORMATION* kni = (KEY_NAME_INFORMATION*)buffer.data();
		return npath(nstring(kni->Name, kni->NameLength));
	}

	key_info NtixRegistryLib::get_info(npath p) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE);

		// KeyFullInformation ends with the (rarely set) class string, so it can still overflow
		std::vector<BYTE> buffer(sizeof(KEY_FULL_INFORMATION) + 256);
		ULONG retLen = 0;
		NTSTATUS status;
		for (;;)
		{
			status = query_key(hKey, KeyFullInformation, buffer.data(), (ULONG)buffer.size(), &retLen);
			if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
				break;
			buffer.resize(retLen > buffer.size() ? retLen : buffer.size() * 2);
		}
		close(hKey);
		if (!NT_SUCCESS(status))
			throw nexception("NtixRegistryLib::get_info query_key", status);

		KEY_FULL_INFORMATION* kfi = (KEY_FULL_INFORMATION*)buffer.data();
		return { kfi->LastWriteTime.QuadPart, kfi->SubKeys, kfi->Values };
	}

	// Enumeration is by index, one entry per call. Nothing stops another process adding or
	// deleting a subkey mid-walk, which shifts the indexes: an entry may be skipped or repeated.
	std::vector<key_entry> NtixRegistryLib::read_keys(npath p) const
	{
		HANDLE hKey = open_key(p, KEY_ENUMERATE_SUB_KEYS);

		// key names are limited to 255 characters
		std::vector<BYTE> buffer(sizeof(KEY_BASIC_INFORMATION) + 512);
		std::vector<key_entry> keys;

		for (ULONG index = 0; ; index++)
		{
			ULONG retLen = 0;
			NTSTATUS status;
			for (;;)
			{
				status = enumerate_key(hKey, index, KeyBasicInformation, buffer.data(), (ULONG)buffer.size(), &retLen);
				if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					break;
				buffer.resize(retLen > buffer.size() ? retLen : buffer.size() * 2);
			}
			if (status == STATUS_NO_MORE_ENTRIES)
				break;
			if (!NT_SUCCESS(status))
			{
				close(hKey);
				throw nexception("NtixRegistryLib::read_keys enumerate_key", status);
			}

			KEY_BASIC_INFORMATION* kbi = (KEY_BASIC_INFORMATION*)buffer.data();
			keys.push_back({ nstring(kbi->Name, kbi->NameLength), kbi->LastWriteTime.QuadPart });
		}
		close(hKey);
		return keys;
	}

	// KeyValueBasicInformation has no size, so this always uses KeyValueFullInformation,
	// which carries the data whether we want it or not. with_data only decides if we keep it.
	std::vector<value_info> NtixRegistryLib::read_values(npath p, bool with_data) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE);

		std::vector<BYTE> buffer(4096);
		std::vector<value_info> values;

		for (ULONG index = 0; ; index++)
		{
			ULONG retLen = 0;
			NTSTATUS status;
			for (;;)
			{
				status = enumerate_value_key(hKey, index, KeyValueFullInformation, buffer.data(), (ULONG)buffer.size(), &retLen);
				if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
					break;
				buffer.resize(retLen > buffer.size() ? retLen : buffer.size() * 2);
			}
			if (status == STATUS_NO_MORE_ENTRIES)
				break;
			if (!NT_SUCCESS(status))
			{
				close(hKey);
				throw nexception("NtixRegistryLib::read_values enumerate_value_key", status);
			}

			KEY_VALUE_FULL_INFORMATION* kvfi = (KEY_VALUE_FULL_INFORMATION*)buffer.data();
			std::vector<BYTE> data;
			if (with_data)
				data.assign(buffer.data() + kvfi->DataOffset, buffer.data() + kvfi->DataOffset + kvfi->DataLength);
			values.push_back({ nstring(kvfi->Name, kvfi->NameLength), kvfi->Type, kvfi->DataLength, data });
		}
		close(hKey);
		return values;
	}

	value_info NtixRegistryLib::read_value(npath p, const nstring& name) const
	{
		HANDLE hKey = open_key(p, KEY_QUERY_VALUE);
		try {
			value_info v = read_value(hKey, name);
			close(hKey);
			return v;
		} catch (nexception& e) {
			close(hKey);
			throw e;
		}
	}

	// doesn't close h; the caller opened it and may have more to do with it
	value_info NtixRegistryLib::read_value(HANDLE h, const nstring& name) const
	{
		// an empty name is the key's default value
		UNICODE_STRING us = name.unicode_str();

		std::vector<BYTE> buffer(sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256);
		ULONG retLen = 0;
		NTSTATUS status;
		for (;;)
		{
			status = query_value_key(h, &us, KeyValuePartialInformation, buffer.data(), (ULONG)buffer.size(), &retLen);
			if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
				break;
			buffer.resize(retLen > buffer.size() ? retLen : buffer.size() * 2);
		}
		if (!NT_SUCCESS(status))
			throw nexception("NtixRegistryLib::read_value query_value_key", status);

		KEY_VALUE_PARTIAL_INFORMATION* kvpi = (KEY_VALUE_PARTIAL_INFORMATION*)buffer.data();
		return { name, kvpi->Type, kvpi->DataLength, std::vector<BYTE>(kvpi->Data, kvpi->Data + kvpi->DataLength) };
	}

	const nstring NtixRegistryLib::type_str(ULONG type)
	{
		switch (type) {
			case REG_NONE: return "REG_NONE";
			case REG_SZ: return "REG_SZ";
			case REG_EXPAND_SZ: return "REG_EXPAND_SZ";
			case REG_BINARY: return "REG_BINARY";
			case REG_DWORD: return "REG_DWORD";
			case REG_DWORD_BIG_ENDIAN: return "REG_DWORD_BIG_ENDIAN";
			case REG_LINK: return "REG_LINK";
			case REG_MULTI_SZ: return "REG_MULTI_SZ";
			case REG_RESOURCE_LIST: return "REG_RESOURCE_LIST";
			case REG_FULL_RESOURCE_DESCRIPTOR: return "REG_FULL_RESOURCE_DESCRIPTOR";
			case REG_RESOURCE_REQUIREMENTS_LIST: return "REG_RESOURCE_REQUIREMENTS_LIST";
			case REG_QWORD: return "REG_QWORD";
		}
		// the type is just a ULONG the writer chose; anything goes
		std::ostringstream ss;
		ss << "0x" << std::hex << type;
		return nstring(ss.str());
	}
}
