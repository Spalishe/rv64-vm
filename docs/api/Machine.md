

# Machine

```cpp
#include <machine.hpp>

class Machine
```

Defined in include/machine.hpp:59

RV64-VM Main machine class.

This class implements RISC-V emulator machine.

## Public Attributes

| Return | Name | Description |
|--------|------|-------------|
| [`int`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`virtio_count`](#virtio_count)  | VirtIO devices counter. |

---



### virtio_count

```cpp
int virtio_count = 0
```

Type: [`int`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df)

Defined in include/machine.hpp:256

VirtIO devices counter.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Machine`](#machine)  | [Machine](#machine) constructor. |
|  | [`~Machine`](#~machine)  | [Machine](#machine) destructor. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`start_init`](#start_init) `inline` | Device initialization start. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`end_init`](#end_init) `inline` | Device initialization end. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`run`](#run)  | Runs machine. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`stop`](#stop)  | Stops machine. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`reset`](#reset)  | Resets machines. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`wait`](#wait)  | Joins machine work thread. |
| [`MMIO`](MMIO.md#mmio) * | [`get_mmio`](#get_mmio) `inline` | Returns [MMIO](MMIO.md#mmio) pointer. |
| [`fdt_node`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | [`get_fdt`](#get_fdt) `inline` | Returns FDT pointer. |
| [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`get_timebase`](#get_timebase) `const` `inline` | Returns config specified timer timebase (Hz/S) |
| [`MemoryMap`](MemoryMap.md#memorymap) * | [`get_mmap`](#get_mmap) `inline` | Returns [MemoryMap](MemoryMap.md#memorymap) pointer. |
| [`MachineState`](#machinestate) | [`get_state`](#get_state) `const` `inline` | Returns [Machine](#machine) internal state. |
| [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`get_memory_size`](#get_memory_size) `const` `inline` | Returns config specified RAM size. |
| [`uint8_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`get_hart_count`](#get_hart_count) `const` `inline` | Returns config specified [Hart](Hart.md#hart) count. |
| [`Hart`](Hart.md#hart) & | [`get_hart`](#get_hart) `inline` | Returns specified [Hart](Hart.md#hart) by index. |
| [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`load_vd_image`](#load_vd_image)  | Loads Image file to VirtIO-Blk. |
| [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`load_bios`](#load_bios)  | Loads Firmware file. |
| [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`load_kernel`](#load_kernel)  | Loads Kernel file. |
| [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`load_dtb`](#load_dtb)  | Loads DTB file. |
| [`FILE`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | [`get_vd_image`](#get_vd_image)  | Returns FILE pointer to loaded VirtIO-Blk file. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`set_uart_output`](#set_uart_output)  | Sets UART output stream. |
| [`FILE`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | [`get_uart_output`](#get_uart_output)  | Returns UART output stream. |

---



### Machine

```cpp
Machine(constMachineConfig & cfg)
```

Defined in include/machine.hpp:78

[Machine](#machine) constructor.

Creates RISC-V machine

**See also**: [MachineConfig](MachineConfig.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cfg` | [`const`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df)[`MachineConfig`](MachineConfig.md#machineconfig) & | [Machine](#machine) configuration |

---



### ~Machine

```cpp
~Machine()
```

Defined in include/machine.hpp:84

[Machine](#machine) destructor.

Destroys RISC-V machine 
> [!NOTE]
> It is recommended to stop machine before destroy it.

---



### start_init

`inline`

```cpp
inline void start_init()
```

Defined in include/machine.hpp:95

Device initialization start.

Creates FDT Base for all devices In this block you supposed to create all devices you want, or load DTB from file.

---



### end_init

`inline`

```cpp
inline void end_init()
```

Defined in include/machine.hpp:105

Device initialization end.

Writes FDT to memory. This function must be called after you created all devices you wanted.

---



### run

```cpp
void run()
```

Defined in include/machine.hpp:114

Runs machine.

Starts all [Hart](Hart.md#hart)'s execution loop.

---



### stop

```cpp
void stop()
```

Defined in include/machine.hpp:120

Stops machine.

Sends a signal to machine so it could stop and destroy all harts safely. 
> [!NOTE]
> In that moment it joins work thread.

---



### reset

```cpp
void reset()
```

Defined in include/machine.hpp:126

Resets machines.

Sends a signal to machine so it could safely recreate all HART's. 
> [!WARNING]
> Untested, but it is not recommended to first-time start machine with this function.

---



### wait

```cpp
void wait()
```

Defined in include/machine.hpp:130

Joins machine work thread.

---



### get_mmio

`inline`

```cpp
inline MMIO * get_mmio()
```

Defined in include/machine.hpp:137

Returns [MMIO](MMIO.md#mmio) pointer.

**See also**: [MMIO](MMIO.md)

#### Returns
[MMIO](MMIO.md#mmio) Pointer

---



### get_fdt

`inline`

```cpp
inline fdt_node * get_fdt()
```

Defined in include/machine.hpp:143

Returns FDT pointer.

**See also**: libfdt.h

#### Returns
FDT pointer

---



### get_timebase

`const` `inline`

```cpp
inline uint64_t get_timebase() const
```

Defined in include/machine.hpp:148

Returns config specified timer timebase (Hz/S)

#### Returns
Timebase number

---



### get_mmap

`inline`

```cpp
inline MemoryMap * get_mmap()
```

Defined in include/machine.hpp:154

Returns [MemoryMap](MemoryMap.md#memorymap) pointer.

**See also**: [MemoryMap](MemoryMap.md)

#### Returns
[MemoryMap](MemoryMap.md#memorymap) pointer

---



### get_state

`const` `inline`

```cpp
inline MachineState get_state() const
```

Defined in include/machine.hpp:161

Returns [Machine](#machine) internal state.

**See also**: MachineState

#### Returns
[Machine](#machine) State enum

---



### get_memory_size

`const` `inline`

```cpp
inline uint64_t get_memory_size() const
```

Defined in include/machine.hpp:166

Returns config specified RAM size.

#### Returns
Memory Size (bytes)

---



### get_hart_count

`const` `inline`

```cpp
inline uint8_t get_hart_count() const
```

Defined in include/machine.hpp:172

Returns config specified [Hart](Hart.md#hart) count.

**See also**: HART

#### Returns
HART count

---



### get_hart

`inline`

```cpp
inline Hart & get_hart(size_t index)
```

Defined in include/machine.hpp:179

Returns specified [Hart](Hart.md#hart) by index.

**See also**: [Hart](Hart.md)

#### Returns
[Hart](Hart.md#hart) object

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | [`size_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | HART index (not ID!) |

---



### load_vd_image

```cpp
bool load_vd_image(const std::string & path)
```

Defined in include/machine.hpp:186

Loads Image file to VirtIO-Blk.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | [`const`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) std::string & | Image path |

---



### load_bios

```cpp
bool load_bios(const std::string & path)
```

Defined in include/machine.hpp:192

Loads Firmware file.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | [`const`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) std::string & | Firmware path |

---



### load_kernel

```cpp
bool load_kernel(const std::string & path)
```

Defined in include/machine.hpp:198

Loads Kernel file.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | [`const`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) std::string & | Kernel path |

---



### load_dtb

```cpp
bool load_dtb(const std::string & path)
```

Defined in include/machine.hpp:206

Loads DTB file.

Loads custom FDT from DTB to memory. 
> [!NOTE]
> Make sure to set init_fdt in config to false before init!

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | [`const`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) std::string & | DTB path |

---



### get_vd_image

```cpp
FILE * get_vd_image(int idx)
```

Defined in include/machine.hpp:212

Returns FILE pointer to loaded VirtIO-Blk file.

#### Returns
FILE pointer

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `idx` | [`int`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Image index |

---



### set_uart_output

```cpp
void set_uart_output(FILE * stream)
```

Defined in include/machine.hpp:217

Sets UART output stream.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `stream` | [`FILE`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | Output stream |

---



### get_uart_output

```cpp
FILE * get_uart_output()
```

Defined in include/machine.hpp:222

Returns UART output stream.

#### Returns
Output stream

## Public Types

| Name | Description |
|------|-------------|
| [`MachineState`](#machinestate)  | [Machine](#machine) State enum. |

---



### MachineState

```cpp
enum MachineState
```

Type: [`uint8_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df)

Defined in include/machine.hpp:65

[Machine](#machine) State enum.

| Value | Description |
|-------|-------------|
| `Off` |  |
| `Halted` | Powered off |
| `Running` | Awaiting any command |
| `Resetting` | Self-explanatory |
