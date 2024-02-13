/*
 * UEFI Memory cleaner
 */

#include <stdint.h>
#include <efi.h>

#define PAGE_SIZE 4096
#define MAX_MAPS 100

static const int verbose = 1;

static EFI_BOOT_SERVICES *gBS;
static SIMPLE_TEXT_OUTPUT_INTERFACE *gOut;
static SIMPLE_TEXT_OUTPUT_INTERFACE *gErr;

struct MapEntry  {
	UINTN Start;
	UINTN End;
	int Type;
};

struct MapEntry CleanedMaps[MAX_MAPS];
UINTN NumCleanedMap = 0;

/* Debug function to print a hex value */
static void debug_hex(UINT16 *str, uint64_t val)
{
	int i;
	UINT16 s[] = L"0";

	if (!verbose)
		return;

	gOut->OutputString(gOut, str);
	for (i = 60; i >= 0; i -= 4) {
		int nibble = (val >> i) & 0xf;
		UINT16 c = nibble > 9 ? L'a' + nibble - 0xa : L'0' + nibble;
		s[0] = c;
		gOut->OutputString(gOut, s);
	}
	gOut->OutputString(gOut, L"\n");
}


EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* system_table)
{
	UINTN MemoryMapSize = 0;
	UINTN DescriptorSize = 0;
	EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
	EFI_MEMORY_DESCRIPTOR *Map = NULL;
	UINTN MapKey;
	EFI_STATUS r = EFI_SUCCESS;
	UINTN rsp;
	UINTN rip;

	//EFI_GUID MyVariableGuid = {0x12345678, 0x1234, 0x1234, {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0}};
	//CHAR16 *VariableName = L"MyVariableName";

	/* Remember important global variables */
	gBS = system_table->BootServices;
	gOut = system_table->ConOut;
	gErr = system_table->StdErr;


	gErr->OutputString(gErr, L"Clearing memory...\n");

	/* Disable IRQs to ensure nothing allocates memory in between */
	asm volatile("cli");

	/* Fetch memory map of the system */
	do {
		r = gBS->GetMemoryMap(&MemoryMapSize, NULL, NULL, NULL, NULL);
		if (r != EFI_BUFFER_TOO_SMALL)
			continue;

		r = gBS->AllocatePool(EfiBootServicesData, MemoryMapSize + 4096, (void**)&MemoryMap);
		if (r != EFI_SUCCESS)
			goto err;

		r = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, NULL);
	} while (r == EFI_BUFFER_TOO_SMALL);

	asm volatile ("movq %%rsp, %0" : "=r"(rsp));
	asm volatile ("lea (%%rip), %0" : "=r"(rip));
	debug_hex(L" rsp: 0x", rsp);
	debug_hex(L" rip: 0x", rip);

	UINTN ProtectedMemSize = 0;
	/* Loop through every unoccupied memory range and reserve + clear it. */
	for (Map = MemoryMap;
			Map != ((void*)MemoryMap + MemoryMapSize);
			Map = NextMemoryDescriptor(Map, DescriptorSize)) {
		UINT64 i;
		void *addr = (void*)Map->PhysicalStart;
		UINTN start = Map->PhysicalStart;
		UINTN end = start + (PAGE_SIZE * Map->NumberOfPages);

		debug_hex(L"\nStart: 0x", start);
		debug_hex(L"End:   0x", end);
		debug_hex(L"Size:   0x", end - start);
		debug_hex(L"Memory Type: ", Map->Type);

		if (Map->Type == EfiConventionalMemory) {

			r = gBS->AllocatePages(AllocateAddress, EfiLoaderData, Map->NumberOfPages, (void*)&addr);
			if (r != EFI_SUCCESS) {
				/* Looks like the region is no longer available, skip it */
				debug_hex(L"Skipping (alloc): 0x", start);
				continue;
			}

			//debug_hex(L"Cleaning the conventional memory region: 0x", start);
			/* Clear one page at a time */
			for (i = 0; i < Map->NumberOfPages; i++, addr += PAGE_SIZE) {
				gBS->SetMem(addr, PAGE_SIZE, 0);
			}

			gBS->FreePages(start, Map->NumberOfPages);
			//debug_hex(L"Cleaned the conventional memory region: 0x", start);
		}
		else {

			int Clearable = 0;
			switch (Map->Type) {
				// Becareful: The following mapped regions will be cleared
				case EfiACPIReclaimMemory:
				case EfiACPIMemoryNVS:
				case EfiMemoryMappedIO:
				case EfiMemoryMappedIOPortSpace:
					Clearable = 1;
					break;
				case EfiBootServicesData: 
					if (rsp < start || rsp > end) {
						Clearable = 1;
						break;
					}
				case EfiBootServicesCode:
					if (rip < start || rip > end) {
						Clearable = 1;
						break;
					}

					// The following regions will not be cleared but locked out
					// through grub
				case EfiPalCode:
				case EfiLoaderCode:
				case EfiUnusableMemory:
				case EfiReservedMemoryType:
				case EfiRuntimeServicesCode:
				case EfiRuntimeServicesData:
					Clearable = 0;
					break;
			}


			if (Clearable) {
				CleanedMaps[NumCleanedMap].Start = start;
				CleanedMaps[NumCleanedMap].End = end;
				++NumCleanedMap;
				debug_hex(L"Clearable ", start);
			}
			else {
				debug_hex(L"Uncleaned ", start);
				ProtectedMemSize += (end - start);
			}
		}
	}
	debug_hex(L"The total size of protectred memory: ", ProtectedMemSize);

	/* Turn on interrupts again */
	asm volatile("sti");


	/* Done */
	r = gBS->FreePool(MemoryMap);
	if (r != EFI_SUCCESS)
		goto err;

	// 
	// Record which regions will not be cleared here and expect
	// "memmap" to do so through the OS command line
	//

#if 0
	// MyData is the data you want to store
	UINT8 MyData[] = {1,2,3,4,5,6};

	UINTN DataSize = sizeof(MyData);
	r = system_table->RuntimeServices->SetVariable(
			VariableName,
			&MyVariableGuid,
			EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
			DataSize,
			MyData
			);
	if (EFI_ERROR(r)) {
		debug_hex(L"Failed to set variable ", r);
		goto err;
	}
	else {
		debug_hex(L"Set the variable ", r);
	}
#endif

#if 0
	//
	// Assuming no changes to the memory map, call ExitBootServices
	//
	r = gBS->ExitBootServices(image_handle, MapKey);
	if (EFI_ERROR(r)) {
		debug_hex(L"Failed to exit services ", r);
		goto err;
	}
#endif

	//
	// IMPORTANT: Do not use any other vairable besides the ones on
	// the stack because all other data will be cleared
	// 
	// The following would clear all reclaimable maps except for the
	// stack and the code of the current EFI application
	for (int i = 0; i < NumCleanedMap; ++i) {
		gBS->SetMem((void *)CleanedMaps[NumCleanedMap].Start,
				CleanedMaps[NumCleanedMap].End - CleanedMaps[NumCleanedMap].Start, 0);
		debug_hex(L"Cleaned ", CleanedMaps[NumCleanedMap].Start);
	}
	gErr->OutputString(gErr, L"Successfully cleared memory\n");


	return EFI_SUCCESS;

err:
	gErr->OutputString(gErr, L"Failed to clear memory\n");
	return r;
}
