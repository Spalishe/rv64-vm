

# runner

## Classes

| Name | Description |
|------|-------------|
| [`Machine`](rv64vm-runner-Machine.md#machine) | RV64-VM Main machine class. |
| [`MemoryMap`](rv64vm-runner-MemoryMap.md#memorymap) | RV64-VM Memory map class. |
| [`MachineConfig`](rv64vm-runner-MachineConfig.md#machineconfig) | [Machine](rv64vm-runner-Machine.md#machine) configuration. |

## Enumerations

| Name | Description |
|------|-------------|
| [`MachineState`](#machinestate)  | [Machine](rv64vm-runner-Machine.md#machine) State enum. |

---



### MachineState

```cpp
enum MachineState
```

[Machine](rv64vm-runner-Machine.md#machine) State enum.

| Value | Description |
|-------|-------------|
| `Off` |  |
| `Halted` |  |
| `Running` |  |
| `Resetting` |  |



## Substructure `runner::MemoryMap`

# MemoryMap

```cpp
#include <memory_map.hpp>

class MemoryMap
```

Defined in include/memory_map.hpp:36

RV64-VM Memory map class.

This class implements Main memory storage.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`MemoryMap`](#memorymap-1) | `function` | Declared here |
| [`~MemoryMap`](#memorymap-2) | `function` | Declared here |
| [`get_regions`](#get_regions) | `function` | Declared here |
| [`get_ram_direct`](#get_ram_direct) | `function` | Declared here |
| [`add_region`](#add_region) | `function` | Declared here |
| [`load_file`](#load_file) | `function` | Declared here |
| [`load_buffer`](#load_buffer) | `function` | Declared here |
| [`load`](#load) | `function` | Declared here |
| [`store`](#store) | `function` | Declared here |
| [`find_region`](#find_region) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MemoryMap`](#memorymap-1) `inline` | [MemoryMap](#memorymap) constructor. |
|  | [`~MemoryMap`](#memorymap-2) `inline` | [MemoryMap](#memorymap) destructor. |
| `std::vector< MemoryRegion * > &` | [`get_regions`](#get_regions) `inline` | Returns all created regions. |
| `MemoryRegion *` | [`get_ram_direct`](#get_ram_direct) `const` `inline` | Returns pointer to 0x80000000 guest ram region. |
| `void` | [`add_region`](#add_region) `inline` | Creates new guest region in memory. |
| `bool` | [`load_file`](#load_file) `inline` | Loads binary or ELF file to guest memory. |
| `bool` | [`load_buffer`](#load_buffer) `inline` | Loads buffer to guest memory. |
| `uint64_t` | [`load`](#load) `inline` | Loads value from guest memory. |
| `void` | [`store`](#store) `inline` | Stores value to guest memory. |
| `MemoryRegion *` | [`find_region`](#find_region) `inline` | Finds region by specidied guest address. |

---



### MemoryMap

`inline`

```cpp
inline MemoryMap()
```

Defined in include/memory_map.hpp:42

[MemoryMap](#memorymap) constructor.

---



### ~MemoryMap

`inline`

```cpp
inline ~MemoryMap()
```

Defined in include/memory_map.hpp:47

[MemoryMap](#memorymap) destructor.

Safely removes all regions

---



### get_regions

`inline`

```cpp
inline std::vector< MemoryRegion * > & get_regions()
```

Defined in include/memory_map.hpp:118

Returns all created regions.

**See also**: [MemoryRegion](rv64vm-runner-MemoryMap-MemoryRegion.md#memoryregion)

#### Returns
Memory regions

---



### get_ram_direct

`const` `inline`

```cpp
inline MemoryRegion * get_ram_direct() const
```

Defined in include/memory_map.hpp:124

Returns pointer to 0x80000000 guest ram region.

**See also**: [MemoryRegion](rv64vm-runner-MemoryMap-MemoryRegion.md#memoryregion)

#### Returns
Direct pointer to region

---



### add_region

`inline`

```cpp
inline void add_region(uint64_t base, size_t size)
```

Defined in include/memory_map.hpp:130

Creates new guest region in memory.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `base` | `uint64_t` | Guest base |
| `size` | `size_t` | Region size |

---



### load_file

`inline`

```cpp
inline bool load_file(uint64_t memory_path, std::string path = "", uint64_t * entry_pc = NULL)
```

Defined in include/memory_map.hpp:145

Loads binary or ELF file to guest memory.

#### Returns
Success boolean

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `memory_path` | `uint64_t` | Guest address to put data in |
| `path` | `std::string` | Path to file |
| `entry_pc` | `uint64_t *` | Output program counter readed from ELF file(if you use elf) |

---



### load_buffer

`inline`

```cpp
inline bool load_buffer(uint64_t memory_path, char * buffer, uint64_t size, uint64_t * entry_pc = NULL)
```

Defined in include/memory_map.hpp:183

Loads buffer to guest memory.

#### Returns
Success boolean

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `memory_path` | `uint64_t` | Guest address to put data in |
| `buffer` | `char *` | Host pointer to buffer |
| `size` | `uint64_t` | Buffer size |
| `entry_pc` | `uint64_t *` | Output program counter readed from ELF file(if you use elf) |

---



### load

`inline`

```cpp
inline uint64_t load(uint64_t addr, uint64_t size)
```

Defined in include/memory_map.hpp:204

Loads value from guest memory.

#### Returns
Value stored in memory

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest address |
| `size` | `uint64_t` | Data size (bits) |

---



### store

`inline`

```cpp
inline void store(uint64_t addr, uint64_t size, uint64_t value)
```

Defined in include/memory_map.hpp:234

Stores value to guest memory.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest address |
| `size` | `uint64_t` | Data size (bits) |
| `value` | `uint64_t` | Value to store |

---



### find_region

`inline`

```cpp
inline MemoryRegion * find_region(uint64_t addr)
```

Defined in include/memory_map.hpp:268

Finds region by specidied guest address.

#### Returns
Memory region 

**See also**: [MemoryRegion](rv64vm-runner-MemoryMap-MemoryRegion.md#memoryregion)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest addr |

## Substructure `runner::MachineConfig`

# MachineConfig

```cpp
#include <machine.hpp>

struct MachineConfig
```

Defined in include/machine.hpp:44

[Machine](rv64vm-runner-Machine.md#machine) configuration.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`memory_size`](#structrv64vm_1_1runner_1_1machineconfig_1ab71ae52ca3f902f2be5c8720733fa35b) | `variable` | Declared here |
| [`hart_count`](#structrv64vm_1_1runner_1_1machineconfig_1abf535165f80350ceed9e612b1f601742) | `variable` | Declared here |
| [`entry_pc`](#structrv64vm_1_1runner_1_1machineconfig_1a050dbc9f3fb27a75c2610a3abe5318fb) | `variable` | Declared here |
| [`timebase`](#structrv64vm_1_1runner_1_1machineconfig_1ae68608fe83b8c8e5629c9bbc08157b93) | `variable` | Declared here |
| [`append`](#structrv64vm_1_1runner_1_1machineconfig_1a0015526ea4b828c7941960e811ce06d2) | `variable` | Declared here |
| [`dtb_dump_path`](#structrv64vm_1_1runner_1_1machineconfig_1acbba15822e5d2f227efac59c84593776) | `variable` | Declared here |
| [`init_fdt`](#structrv64vm_1_1runner_1_1machineconfig_1ac56347caaef1a0add9c06a1ba7736dfa) | `variable` | Declared here |

## Substructure `runner::Machine`

# Machine

```cpp
#include <machine.hpp>

class Machine
```

Defined in include/machine.hpp:59

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

Defined in include/machine.hpp:67

[Machine](#machine) constructor.

Creates RISC-V machine

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cfg` | `const [MachineConfig](rv64vm-runner-MachineConfig.md#machineconfig) &` | [Machine](#machine) configuration |

---



### ~Machine

```cpp
~Machine()
```

Defined in include/machine.hpp:73

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

Defined in include/machine.hpp:84

Device initialization start.

Creates FDT Base for all devices In this block you supposed to create all devices you want, or load DTB from file.

---



### end_init

`inline`

```cpp
inline void end_init()
```

Defined in include/machine.hpp:94

Device initialization end.

Writes FDT to memory. This function must be called after you created all devices you wanted.

---



### run

```cpp
void run()
```

Defined in include/machine.hpp:103

Runs machine.

Starts all Hart's execution loop.

---



### stop

```cpp
void stop()
```

Defined in include/machine.hpp:109

Stops machine.

Sends a signal to machine so it could stop and destroy all harts safely. 
> [!NOTE]
> In that moment it joins work thread.

---



### reset

```cpp
void reset()
```

Defined in include/machine.hpp:115

Resets machines.

Sends a signal to machine so it could safely recreate all HART's. 
> [!WARNING]
> Untested, but it is not recommended to first-time start machine with this function.

---



### wait

```cpp
void wait()
```

Defined in include/machine.hpp:119

Joins machine work thread.

---



### get_mmio

`inline`

```cpp
inline MMIO * get_mmio()
```

Defined in include/machine.hpp:126

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

Defined in include/machine.hpp:132

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

Defined in include/machine.hpp:137

Returns config specified timer timebase (Hz/S)

#### Returns
Timebase number

---



### get_mmap

`inline`

```cpp
inline MemoryMap * get_mmap()
```

Defined in include/machine.hpp:143

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

Defined in include/machine.hpp:150

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

Defined in include/machine.hpp:155

Returns config specified RAM size.

#### Returns
Memory Size (bytes)

---



### get_hart_count

`const` `inline`

```cpp
inline uint8_t get_hart_count() const
```

Defined in include/machine.hpp:161

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

Defined in include/machine.hpp:168

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

Defined in include/machine.hpp:175

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

Defined in include/machine.hpp:181

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

Defined in include/machine.hpp:187

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

Defined in include/machine.hpp:195

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

Defined in include/machine.hpp:200

Returns FILE pointer to loaded Image file.

#### Returns
FILE pointer

---



### set_uart_output

```cpp
void set_uart_output(FILE * stream)
```

Defined in include/machine.hpp:205

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

Defined in include/machine.hpp:210

Returns UART output stream.

#### Returns
Output stream

## Substructure `runner::MemoryMap::MemoryRegion`

# MemoryRegion

```cpp
#include <memory_map.hpp>

class MemoryRegion
```

Defined in include/memory_map.hpp:57

Memory Region object.

A pie in a total cake. Contains raw data to memory.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`get_base_addr`](#get_base_addr) | `function` | Declared here |
| [`get_size`](#get_size) | `function` | Declared here |
| [`get_data`](#get_data) | `function` | Declared here |
| [`ptr`](#ptr) | `function` | Declared here |
| [`MemoryRegion`](#memoryregion-1) | `function` | Declared here |
| [`~MemoryRegion`](#memoryregion-2) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
| `uint64_t` | [`get_base_addr`](#get_base_addr) `const` `inline` | Returns Memory region base address in GUEST ram. |
| `size_t` | [`get_size`](#get_size) `const` `inline` | Returns Memory region size. |
| `uint8_t *` | [`get_data`](#get_data) `const` `inline` | Returns Memory region data host pointer. |
| `uint8_t *` | [`ptr`](#ptr) `inline` | Returns Host pointer to Guest address. |
|  | [`MemoryRegion`](#memoryregion-1) `inline` | Memory region constructor. |
|  | [`~MemoryRegion`](#memoryregion-2) `inline` | Memory region destructor. |

---



### get_base_addr

`const` `inline`

```cpp
inline uint64_t get_base_addr() const
```

Defined in include/memory_map.hpp:64

Returns Memory region base address in GUEST ram.

#### Returns
Guest base address

---



### get_size

`const` `inline`

```cpp
inline size_t get_size() const
```

Defined in include/memory_map.hpp:69

Returns Memory region size.

#### Returns
Size (bytes)

---



### get_data

`const` `inline`

```cpp
inline uint8_t * get_data() const
```

Defined in include/memory_map.hpp:74

Returns Memory region data host pointer.

#### Returns
Data host pointer

---



### ptr

`inline`

```cpp
inline uint8_t * ptr(uint64_t addr)
```

Defined in include/memory_map.hpp:82

Returns Host pointer to Guest address.

#### Returns
Host address 

> [!NOTE]
> Make sure you check if [MemoryRegion](#memoryregion) base address and size is in range

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest address |

---



### MemoryRegion

`inline`

```cpp
inline MemoryRegion(uint64_t base, size_t sz)
```

Defined in include/memory_map.hpp:94

Memory region constructor.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `base` | `uint64_t` | Guest address |
| `sz` | `size_t` | Memory size |

---



### ~MemoryRegion

`inline`

```cpp
inline ~MemoryRegion()
```

Defined in include/memory_map.hpp:102

Memory region destructor.