#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/ShellLib.h>
#include <Library/BaseLib.h>
#include <Protocol/SpiNorFlash.h>

#define APP_NAME L"spi"

typedef enum {
  OP_NONE,
  OP_READ,
  OP_WRITE,
  OP_ERASE
} SPI_OPERATION;

STATIC
VOID
PrintUsage(VOID)
{
  Print(L"Usage: %s [-d file|-w file|-e] [-a addr] [-l len]\n", APP_NAME);
  Print(L"  -d file  Dump flash to file\n");
  Print(L"  -w file  Write file to flash\n");
  Print(L"  -e       Erase flash region\n");
  Print(L"  -a addr  Flash address in hex (default 0)\n");
  Print(L"  -l len   Length in bytes (default remaining flash)\n");
}

STATIC
EFI_STATUS
DumpFlash(EFI_SPI_NOR_FLASH_PROTOCOL *Spi, UINT32 Address, UINT32 Length, CHAR16 *FileName)
{
  EFI_STATUS Status;
  UINT8 *Buffer;
  SHELL_FILE_HANDLE File;
  UINTN Size;

  Buffer = AllocateZeroPool(Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Spi->ReadData(Spi, Address, Length, Buffer);
  if (EFI_ERROR(Status)) {
    FreePool(Buffer);
    return Status;
  }

  Status = ShellOpenFileByName(FileName, &File,
                               EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                               0);
  if (EFI_ERROR(Status)) {
    FreePool(Buffer);
    return Status;
  }

  Size = Length;
  Status = ShellWriteFile(File, &Size, Buffer);
  ShellCloseFile(&File);
  FreePool(Buffer);
  return Status;
}

STATIC
EFI_STATUS
WriteFlash(EFI_SPI_NOR_FLASH_PROTOCOL *Spi, UINT32 Address, CHAR16 *FileName)
{
  EFI_STATUS Status;
  SHELL_FILE_HANDLE File;
  UINT64 FileSize;
  UINTN Size;
  UINT8 *Buffer;
  UINT32 BlockCount;

  Status = ShellOpenFileByName(FileName, &File, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  Status = ShellGetFileSize(File, &FileSize);
  if (EFI_ERROR(Status)) {
    ShellCloseFile(&File);
    return Status;
  }
  Buffer = AllocateZeroPool((UINTN)FileSize);
  if (Buffer == NULL) {
    ShellCloseFile(&File);
    return EFI_OUT_OF_RESOURCES;
  }

  Size = (UINTN)FileSize;
  Status = ShellReadFile(File, &Size, Buffer);
  ShellCloseFile(&File);
  if (EFI_ERROR(Status)) {
    FreePool(Buffer);
    return Status;
  }

  BlockCount = (UINT32)((FileSize + Spi->EraseBlockBytes - 1) / Spi->EraseBlockBytes);
  Status = Spi->Erase(Spi, Address, BlockCount);
  if (!EFI_ERROR(Status)) {
    Status = Spi->WriteData(Spi, Address, (UINT32)FileSize, Buffer);
  }
  FreePool(Buffer);
  return Status;
}

STATIC
EFI_STATUS
EraseFlash(EFI_SPI_NOR_FLASH_PROTOCOL *Spi, UINT32 Address, UINT32 Length)
{
  UINT32 BlockCount;
  BlockCount = (Length + Spi->EraseBlockBytes - 1) / Spi->EraseBlockBytes;
  return Spi->Erase(Spi, Address, BlockCount);
}

INTN
EFIAPI
ShellAppMain(IN UINTN Argc, IN CHAR16 **Argv)
{
  EFI_STATUS Status;
  EFI_SPI_NOR_FLASH_PROTOCOL *Spi;
  SPI_OPERATION Operation = OP_NONE;
  CHAR16 *FileName = NULL;
  UINT32 Address = 0;
  UINT32 Length = 0;
  UINTN Index;

  for (Index = 1; Index < Argc; Index++) {
    if ((StrCmp(Argv[Index], L"-w") == 0) && (Index + 1 < Argc)) {
      Operation = OP_WRITE;
      FileName = Argv[++Index];
    } else if ((StrCmp(Argv[Index], L"-d") == 0) && (Index + 1 < Argc)) {
      Operation = OP_READ;
      FileName = Argv[++Index];
    } else if (StrCmp(Argv[Index], L"-e") == 0) {
      Operation = OP_ERASE;
    } else if ((StrCmp(Argv[Index], L"-a") == 0) && (Index + 1 < Argc)) {
      Address = (UINT32)StrHexToUintn(Argv[++Index]);
    } else if ((StrCmp(Argv[Index], L"-l") == 0) && (Index + 1 < Argc)) {
      Length = (UINT32)StrHexToUintn(Argv[++Index]);
    } else {
      PrintUsage();
      return EFI_INVALID_PARAMETER;
    }
  }

  if (Operation == OP_NONE) {
    PrintUsage();
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol(&gEfiSpiNorFlashProtocolGuid, NULL, (VOID **)&Spi);
  if (EFI_ERROR(Status)) {
    Print(L"SPI NOR flash protocol not found\n");
    return Status;
  }

  if (Length == 0) {
    Length = Spi->FlashSize - Address;
  }
  if (Address + Length > Spi->FlashSize) {
    Print(L"Address range exceeds flash size\n");
    return EFI_INVALID_PARAMETER;
  }

  switch (Operation) {
    case OP_READ:
      Status = DumpFlash(Spi, Address, Length, FileName);
      break;
    case OP_WRITE:
      Status = WriteFlash(Spi, Address, FileName);
      break;
    case OP_ERASE:
      Status = EraseFlash(Spi, Address, Length);
      break;
    default:
      Status = EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR(Status)) {
    Print(L"Operation failed: %r\n", Status);
  } else {
    Print(L"Success\n");
  }

  return Status;
}

