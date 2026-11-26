

# MemoryMap

```cpp
#include <memory_map.hpp>

class MemoryMap
```

Defined in include/memory_map.hpp:38

RV64-VM Memory map class.

This class implements Main memory storage.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MemoryMap`](#memorymap) `inline` | [MemoryMap](#memorymap) constructor. |
|  | [`~MemoryMap`](#~memorymap) `inline` | [MemoryMap](#memorymap) destructor. |
| std::vector< [`MemoryRegion`](MemoryRegion.md#memoryregion) * > & | [`get_regions`](#get_regions) `inline` | Returns all created regions. |
| [`MemoryRegion`](MemoryRegion.md#memoryregion) * | [`get_ram_direct`](#get_ram_direct) `const` `inline` | Returns pointer to 0x80000000 guest ram region. |
| `void` | [`add_region`](#add_region) `inline` | Creates new guest region in memory. |
| `bool` | [`load_file`](#load_file) `inline` | Loads binary or ELF file to guest memory. |
| `bool` | [`load_buffer`](#load_buffer) `inline` | Loads buffer to guest memory. |
| `uint64_t` | [`load`](#load) `inline` | Loads value from guest memory. |
| `void` | [`store`](#store) `inline` | Stores value to guest memory. |
| [`MemoryRegion`](MemoryRegion.md#memoryregion) * | [`find_jit_direct`](#find_jit_direct) `inline` | Finds a whole range inside a JIT-direct region. |
| [`MemoryRegion`](MemoryRegion.md#memoryregion) * | [`find_region`](#find_region) `inline` | Finds region by specidied guest address. |

---



### MemoryMap

`inline`

```cpp
inline MemoryMap()
```

Defined in include/memory_map.hpp:44

[MemoryMap](#memorymap) constructor.

---



### ~MemoryMap

`inline`

```cpp
inline ~MemoryMap()
```

Defined in include/memory_map.hpp:49

[MemoryMap](#memorymap) destructor.

Safely removes all regions

---



### get_regions

`inline`

```cpp
inline std::vector< MemoryRegion * > & get_regions()
```

Defined in include/memory_map.hpp:130

Returns all created regions.

**See also**: [MemoryRegion](MemoryRegion.md)

#### Returns
Memory regions

---



### get_ram_direct

`const` `inline`

```cpp
inline MemoryRegion * get_ram_direct() const
```

Defined in include/memory_map.hpp:136

Returns pointer to 0x80000000 guest ram region.

**See also**: [MemoryRegion](MemoryRegion.md)

#### Returns
Direct pointer to region

---



### add_region

`inline`

```cpp
inline void add_region(uint64_t base, size_t size, bool jit_direct = false)
```

Defined in include/memory_map.hpp:142

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

Defined in include/memory_map.hpp:157

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

Defined in include/memory_map.hpp:195

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

Defined in include/memory_map.hpp:216

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

Defined in include/memory_map.hpp:258

Stores value to guest memory.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest address |
| `size` | `uint64_t` | Data size (bits) |
| `value` | `uint64_t` | Value to store |

---



### find_jit_direct

`inline`

```cpp
inline MemoryRegion * find_jit_direct(uint64_t addr, size_t len)
```

Defined in include/memory_map.hpp:301

Finds a whole range inside a JIT-direct region.

Only RAM and explicitly marked flat buffers (framebuffer) qualify. Side-effectful [MMIO](MMIO.md#mmio) devices are excluded so JIT stores cannot bypass their write handlers.

---



### find_region

`inline`

```cpp
inline MemoryRegion * find_region(uint64_t addr)
```

Defined in include/memory_map.hpp:327

Finds region by specidied guest address.

#### Returns
Memory region

**See also**: [MemoryRegion](MemoryRegion.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest addr |

