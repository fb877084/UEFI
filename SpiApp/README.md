# SpiApp

A simple UEFI application demonstrating SPI NOR flash access using the
`EFI_SPI_NOR_FLASH_PROTOCOL`. The program supports basic read, write, and erase
operations on the flash device.

## Building

This application is intended to be built within the [EDK II](https://github.com/tianocore/edk2)
firmware development environment. Add `SpiApp.inf` to your platform DSC/FDF
and build for the desired ARM target.

```
build -a ARM -t GCC5 -p YourBoardPkg/YourBoard.dsc -m SpiApp/SpiApp.inf
```

## Usage

```
spi.efi [-d file|-w file|-e] [-a addr] [-l len]
```

Options:

- `-d file` Dump flash contents to `file`.
- `-w file` Write `file` to flash.
- `-e`      Erase the specified region.
- `-a addr` Flash address in hexadecimal (default `0`).
- `-l len`  Length in bytes (default remaining flash).

Example to program the SPI ROM:

```
spi.efi -w firmware.bin
```

Example to dump the SPI ROM to `dump.bin`:

```
spi.efi -d dump.bin
```

