# rv64-vm

> **⚠️ JIT is WORK IN PROGRESS.** The native x86-64 block JIT is experimental:
> only a subset of RV64I is translated, loads/stores and control flow fall back
> to the interpreter, and it is validated by differential tests against the
> interpreter (same guest binary on both paths, final state compared). Expect
> rough edges; the interpreter is the reference execution path.

A software emulator for the RISC-V instruction set architecture (ISA) written
in C++. It supports the RV64GC ISA, the privileged ISA (MmU, Sv39 virtual
memory, machine/supervisor/user privilege levels) and peripheral devices. See
the ["Features List"](#features-list) section for the details.

## Running
Available arguments are:
```
  --bios: File with Machine Level program (bootloader)
  --kernel: File with Supervisor Level program
  --virtioblk: File with Image file that will put on VirtIO-BLK
  --dtb: Use specified FDT instead of auto-generated
  --dumpdtb: Dumps auto-generated FDT to file
  --gdb: Starts GDB Stub on port 1512
  --append: Append command line arguments
  --harts (-S): Set custom harts count (Default is 1)
  --framebuffer (-fb): Enables framebuffer with defined size (F.e. 640x480)
  --memsize (-M): Set custom memory size (Default is 512 MB)
```

## Features List

The emulator supports the following features:
- [x] RV64G ISA
  - [x] RV64I
  - [x] RV64M
  - [x] RV64A (No atomicity for now)
  - [x] RV64F
  - [x] RV64D
  - [x] Zifencei
  - [x] Zicsr
- [x] RV64C
- [ ] Bit manipulations
  - [x] Zba
  - [x] Zbb
  - [x] Zbc
  - [x] Zbs
  - [ ] Zbkb
- [ ] Other instruction sets
  - [x] Zicboz (for faster memory zeroing)
- [x] Privileged ISA
- [x] Control and status registers (CSRs)
  - [x] Machine-level CSRs
  - [x] Supervisor-level CSRs
  - [x] User-level CSRs
- [x] Devices
  - [x] UART: universal asynchronous receiver-transmitter
  - [x] CLINT: core local interruptor
  - [x] PLIC: platform level interrupt controller
  - [x] Virtio-BLK: virtual I/O Block Device
  - [x] Framebuffer: virtual simple screen
  - [x] OpenCores I2C: Inter-Integrated Circuit
  - [x] HID-over-I2C: Human Interface Device over Inter-Integrated Circuit
    - [x] Boot keyboard: Basic usable keyboard(for now without Numpad support)
- [x] FDT

## Build
```bash
git clone <this repository>
cd rv64-vm
make
```
Output program will be located in corresponding target and architecture
folder(f.e. build.linux.x86_64/).

The JIT target is **x86-64 only**; on other architectures build the
interpreter-only configuration:
```bash
make USE_JIT=0
```

### Library
You can also compile emulator as lib:
```bash
cd rv64-vm
make lib
```
And static lib as well:
```bash
cd rv64-vm
make slib
```

See [Getting Started with libraries](docs/Getting-Started-Library.md) and
[API Reference](docs/API-Reference.md)

## Dependencies
You can install all required dependencies using:

Arch:
```bash
sudo pacman -S make gcc git
```

Ubuntu:
```bash
sudo apt install make gcc git
```

For the framebuffer device the Vulkan/X11/Wayland development packages are
also required (`libvulkan-dev`, `libx11-dev`, `libwayland-dev`).

## License
This project is licensed under the Apache 2.0 License – see the
[LICENSE](LICENSE)
