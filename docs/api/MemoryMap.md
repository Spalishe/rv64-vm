

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

Defined in include/memory_map.hpp:107

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

Defined in include/memory_map.hpp:113

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

Defined in include/memory_map.hpp:119

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

Defined in include/memory_map.hpp:134

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

Defined in include/memory_map.hpp:172

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

Defined in include/memory_map.hpp:193

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

Defined in include/memory_map.hpp:223

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

Defined in include/memory_map.hpp:257

Finds region by specidied guest address.

#### Returns
Memory region 

**See also**: [MemoryRegion](rv64vm-runner-MemoryMap-MemoryRegion.md#memoryregion)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest addr |

