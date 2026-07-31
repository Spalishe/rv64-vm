

# Machine

```cpp
#include <machine.hpp>

class Machine
```

Defined in include/machine.hpp:52

RV64-VM Main machine class.

This class implements RISC-V emulator machine.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`Machine`](#machine-1) | `function` | Declared here |
| [`~Machine`](#machine-2) | `function` | Declared here |
| [`start_init`](#start_init) | `function` | Declared here |
| [`end_init`](#end_init) | `function` | Declared here |
| [`run`](#run) | `function` | Declared here |
| [`stop`](#stop) | `function` | Declared here |
| [`reset`](#reset) | `function` | Declared here |
| [`wait`](#wait) | `function` | Declared here |
| [`get_mmio`](#get_mmio) | `function` | Declared here |
| [`get_fdt`](#get_fdt) | `function` | Declared here |
| [`get_timebase`](#get_timebase) | `function` | Declared here |
| [`get_mmap`](#get_mmap) | `function` | Declared here |
| [`get_state`](#get_state) | `function` | Declared here |
| [`get_memory_size`](#get_memory_size) | `function` | Declared here |
| [`get_hart_count`](#get_hart_count) | `function` | Declared here |
| [`get_hart`](#get_hart) | `function` | Declared here |
| [`load_image`](#load_image) | `function` | Declared here |
| [`load_bios`](#load_bios) | `function` | Declared here |
| [`load_kernel`](#load_kernel) | `function` | Declared here |
| [`load_dtb`](#load_dtb) | `function` | Declared here |
| [`get_image`](#get_image) | `function` | Declared here |
| [`set_uart_output`](#set_uart_output) | `function` | Declared here |
| [`get_uart_output`](#get_uart_output) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Machine`](#machine-1)  | [Machine](#machine) constructor. |
|  | [`~Machine`](#machine-2)  | [Machine](#machine) destructor. |
| `void` | [`start_init`](#start_init) `inline` | Device initialization start. |
| `void` | [`end_init`](#end_init) `inline` | Device initialization end. |
| `void` | [`run`](#run)  | Runs machine. |
| `void` | [`stop`](#stop)  | Stops machine. |
| `void` | [`reset`](#reset)  | Resets machines. |
| `void` | [`wait`](#wait)  | Joins machine work thread. |
| `MMIO *` | [`get_mmio`](#get_mmio) `inline` | Returns MMIO pointer. |
| `fdt_node *` | [`get_fdt`](#get_fdt) `inline` | Returns FDT pointer. |
| `uint64_t` | [`get_timebase`](#get_timebase) `const` `inline` | Returns config specified timer timebase (Hz/S) |
| `MemoryMap *` | [`get_mmap`](#get_mmap) `inline` | Returns [MemoryMap](rv64vm-runner-MemoryMap.md#memorymap) pointer. |
| `MachineState` | [`get_state`](#get_state) `const` `inline` | Returns [Machine](#machine) internal state. |
| `uint64_t` | [`get_memory_size`](#get_memory_size) `const` `inline` | Returns config specified RAM size. |
| `uint8_t` | [`get_hart_count`](#get_hart_count) `const` `inline` | Returns config specified Hart count. |
| `Hart &` | [`get_hart`](#get_hart) `inline` | Returns specified Hart by index. |
| `bool` | [`load_image`](#load_image)  | Loads Image file. |
| `bool` | [`load_bios`](#load_bios)  | Loads Firmware file. |
| `bool` | [`load_kernel`](#load_kernel)  | Loads Kernel file. |
| `bool` | [`load_dtb`](#load_dtb)  | Loads DTB file. |
| `FILE *` | [`get_image`](#get_image)  | Returns FILE pointer to loaded Image file. |
| `void` | [`set_uart_output`](#set_uart_output)  | Sets UART output stream. |
| `FILE *` | [`get_uart_output`](#get_uart_output)  | Returns UART output stream. |

---



### Machine

```cpp
Machine(const MachineConfig & cfg)
```

Defined in include/machine.hpp:60

[Machine](#machine) constructor.

Creates RISC-V machine

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cfg` | `const MachineConfig &` | [Machine](#machine) configuration |

---



### ~Machine

```cpp
~Machine()
```

Defined in include/machine.hpp:66

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

Defined in include/machine.hpp:77

Device initialization start.

Creates FDT Base for all devices In this block you supposed to create all devices you want, or load DTB from file.

---



### end_init

`inline`

```cpp
inline void end_init()
```

Defined in include/machine.hpp:87

Device initialization end.

Writes FDT to memory. This function must be called after you created all devices you wanted.

---



### run

```cpp
void run()
```

Defined in include/machine.hpp:96

Runs machine.

Starts all Hart's execution loop.

---



### stop

```cpp
void stop()
```

Defined in include/machine.hpp:102

Stops machine.

Sends a signal to machine so it could stop and destroy all harts safely. 
> [!NOTE]
> In that moment it joins work thread.

---



### reset

```cpp
void reset()
```

Defined in include/machine.hpp:108

Resets machines.

Sends a signal to machine so it could safely recreate all HART's. 
> [!WARNING]
> Untested, but it is not recommended to first-time start machine with this function.

---



### wait

```cpp
void wait()
```

Defined in include/machine.hpp:112

Joins machine work thread.

---



### get_mmio

`inline`

```cpp
inline MMIO * get_mmio()
```

Defined in include/machine.hpp:119

Returns MMIO pointer.

**See also**: MMIO 

#### Returns
MMIO Pointer

---



### get_fdt

`inline`

```cpp
inline fdt_node * get_fdt()
```

Defined in include/machine.hpp:125

Returns FDT pointer.

**See also**: [libfdt.h](#libfdt_8h_source)

#### Returns
FDT pointer

---



### get_timebase

`const` `inline`

```cpp
inline uint64_t get_timebase() const
```

Defined in include/machine.hpp:130

Returns config specified timer timebase (Hz/S)

#### Returns
Timebase number

---



### get_mmap

`inline`

```cpp
inline MemoryMap * get_mmap()
```

Defined in include/machine.hpp:136

Returns [MemoryMap](rv64vm-runner-MemoryMap.md#memorymap) pointer.

**See also**: [MemoryMap](rv64vm-runner-MemoryMap.md#memorymap)

#### Returns
[MemoryMap](rv64vm-runner-MemoryMap.md#memorymap) pointer

---



### get_state

`const` `inline`

```cpp
inline MachineState get_state() const
```

Defined in include/machine.hpp:143

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

Defined in include/machine.hpp:148

Returns config specified RAM size.

#### Returns
Memory Size (bytes)

---



### get_hart_count

`const` `inline`

```cpp
inline uint8_t get_hart_count() const
```

Defined in include/machine.hpp:154

Returns config specified Hart count.

**See also**: HART 

#### Returns
HART count

---



### get_hart

`inline`

```cpp
inline Hart & get_hart(size_t index)
```

Defined in include/machine.hpp:161

Returns specified Hart by index.

**See also**: Hart 

#### Returns
Hart object

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `index` | `size_t` | HART index (not ID!) |

---



### load_image

```cpp
bool load_image(const std::string & path)
```

Defined in include/machine.hpp:168

Loads Image file.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `const std::string &` | Image path |

---



### load_bios

```cpp
bool load_bios(const std::string & path)
```

Defined in include/machine.hpp:174

Loads Firmware file.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `const std::string &` | Firmware path |

---



### load_kernel

```cpp
bool load_kernel(const std::string & path)
```

Defined in include/machine.hpp:180

Loads Kernel file.

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `const std::string &` | Kernel path |

---



### load_dtb

```cpp
bool load_dtb(const std::string & path)
```

Defined in include/machine.hpp:188

Loads DTB file.

Loads custom FDT from DTB to memory. 
> [!NOTE]
> Make sure to set init_fdt in config to false before init!

#### Returns
Success bool

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `path` | `const std::string &` | DTB path |

---



### get_image

```cpp
FILE * get_image()
```

Defined in include/machine.hpp:193

Returns FILE pointer to loaded Image file.

#### Returns
FILE pointer

---



### set_uart_output

```cpp
void set_uart_output(FILE * stream)
```

Defined in include/machine.hpp:198

Sets UART output stream.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `stream` | `FILE *` | Output stream |

---



### get_uart_output

```cpp
FILE * get_uart_output()
```

Defined in include/machine.hpp:203

Returns UART output stream.

#### Returns
Output stream

