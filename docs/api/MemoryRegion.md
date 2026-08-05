

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
| [`MemoryRegion`](#memoryregion) | `function` | Declared here |
| [`~MemoryRegion`](#~memoryregion) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
| `uint64_t` | [`get_base_addr`](#get_base_addr) `const` `inline` | Returns Memory region base address in GUEST ram. |
| `size_t` | [`get_size`](#get_size) `const` `inline` | Returns Memory region size. |
| `uint8_t *` | [`get_data`](#get_data) `const` `inline` | Returns Memory region data host pointer. |
| `uint8_t *` | [`ptr`](#ptr) `inline` | Returns Host pointer to Guest address. |
|  | [`MemoryRegion`](#memoryregion) `inline` | Memory region constructor. |
|  | [`~MemoryRegion`](#~memoryregion) `inline` | Memory region destructor. |

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

