/*
 * UEFI Memory cleaner
 */

#include <efi.h>
#include <efilib.h>

#define PAGE_SIZE 4096

static const int verbose = 1;

static EFI_BOOT_SERVICES *gBS;
static SIMPLE_TEXT_OUTPUT_INTERFACE *gOut;
static SIMPLE_TEXT_OUTPUT_INTERFACE *gErr;

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
	EFI_STATUS r = EFI_SUCCESS;
	UINTN rsp;

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

		r = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, NULL, &DescriptorSize, NULL);
	} while (r == EFI_BUFFER_TOO_SMALL);

	asm volatile ("movq %%rsp, %0" : "=r"(rsp));

	/* Loop through every unoccupied memory range and reserve + clear it. */
	for (Map = MemoryMap;
	     Map != ((void*)MemoryMap + MemoryMapSize);
	     Map = NextMemoryDescriptor(Map, DescriptorSize)) {
		if (Map->Type == EfiConventionalMemory) {
			UINT64 i;
			void *addr = (void*)Map->PhysicalStart;
			UINTN start = Map->PhysicalStart;
			UINTN end = start + (PAGE_SIZE * Map->NumberOfPages);

			/* UEFI does not always protect its stack. Skip it */
			if (rsp >= start && rsp <= end) {
				debug_hex(L"Skipping (rsp): 0x", start);
				continue;
			}

			r = gBS->AllocatePages(AllocateAddress, EfiLoaderData, Map->NumberOfPages, (void*)&addr);
			if (r != EFI_SUCCESS) {
				/* Looks like the region is no longer available, skip it */
				debug_hex(L"Skipping (alloc): 0x", start);
				continue;
			}

			debug_hex(L"Start: 0x", start);
			debug_hex(L"End:   0x", end);

			/* Clear one page at a time */
			for (i = 0; i < Map->NumberOfPages; i++, addr += PAGE_SIZE) {
				gBS->SetMem(addr, PAGE_SIZE, 0);
			}

			gBS->FreePages(start, Map->NumberOfPages);
		}
	}

	/* Turn on interrupts again */
	asm volatile("sti");

	gErr->OutputString(gErr, L"Successfully cleared memory\n");

	/* Done */
	r = gBS->FreePool(MemoryMap);
	if (r != EFI_SUCCESS)
		goto err;

	/* Need to return an error so the next boot entry loads */
	return EFI_BUFFER_TOO_SMALL;

err:
	gErr->OutputString(gErr, L"Failed to clear memory\n");
	return r;
}
