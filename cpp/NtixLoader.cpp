#include "NtixLoader.hpp"

PVOID GetModuleBase(const WCHAR* name) {
	PEB_MINIMAL* peb;
#if defined(_M_X64) || defined(__x86_64__)
	peb = (PEB_MINIMAL*)__readgsqword(0x60);
#else
	peb = (PEB_MINIMAL*)__readfsdword(0x30);
#endif
	if (!peb || !peb->Ldr) return NULL;

	PLIST_ENTRY head = &peb->Ldr->InMemoryOrderModuleList;
	PLIST_ENTRY curr = head->Flink;

	while (curr != head) {
		LDR_DATA_TABLE_ENTRY_MINIMAL* entry = (LDR_DATA_TABLE_ENTRY_MINIMAL*)((char*)curr - 16);
		WCHAR* bname = entry->BaseDllName.Buffer;
		if (bname) {
			int match = 1;
			int i = 0;
			while (name[i] && bname[i]) {
				WCHAR c1 = name[i];
				WCHAR c2 = bname[i];
				if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
				if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
				if (c1 != c2) {
					match = 0;
					break;
				}
				i++;
			}
			if (match && !name[i] && !bname[i]) {
				return entry->DllBase;
			}
		}
		curr = curr->Flink;
	}
	return NULL;
}

PVOID GetProcAddressNative(PVOID moduleBase, const char* funcName) {
	if (!moduleBase) return NULL;

	unsigned char* base = (unsigned char*)moduleBase;
	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)base;
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return NULL;

	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)(base + dosHeader->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return NULL;

	IMAGE_DATA_DIRECTORY* exportDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (exportDir->VirtualAddress == 0 || exportDir->Size == 0) return NULL;

	IMAGE_EXPORT_DIRECTORY* exports = (IMAGE_EXPORT_DIRECTORY*)(base + exportDir->VirtualAddress);

	ULONG* names = (ULONG*)(base + exports->AddressOfNames);
	ULONG* functions = (ULONG*)(base + exports->AddressOfFunctions);
	USHORT* ordinals = (USHORT*)(base + exports->AddressOfNameOrdinals);

	for (ULONG i = 0; i < exports->NumberOfNames; i++) {
		const char* name = (const char*)(base + names[i]);
		int match = 1;
		int j = 0;
		while (funcName[j] || name[j]) {
			if (funcName[j] != name[j]) {
				match = 0;
				break;
			}
			j++;
		}
		if (match) {
			USHORT ord = ordinals[i];
			ULONG funcVa = functions[ord];
			return (PVOID)(base + funcVa);
		}
	}
	return NULL;
}
