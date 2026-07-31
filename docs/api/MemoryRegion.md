

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
| [`ptr`](#ptr) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
| `uint64_t` | [`get_base_addr`](#get_base_addr) `const` `inline` | Returns Memory region base address in GUEST ram. |
| `size_t` | [`get_size`](#get_size) `const` `inline` | Returns Memory region size. |
| `uint8_t *` | [`ptr`](#ptr) `inline` | Returns Host pointer to Guest address. |

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



### ptr

`inline`

```cpp
inline uint8_t * ptr(uint64_t addr)
```

Defined in include/memory_map.hpp:77

Returns Host pointer to Guest address.

#### Returns
Host address 

> [!NOTE]
> Make sure you check if [MemoryRegion](#memoryregion) base address and size is in range

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `addr` | `uint64_t` | Guest address |

