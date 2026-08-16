

# MMU

```cpp
#include <mmu.hpp>

class MMU
```

Defined in include/mmu.hpp:31

RISC-V Memory Management Unit.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`MMU`](#mmu) | `function` | Declared here |
| [`~MMU`](#~mmu) | `function` | Declared here |
| [`translate`](#translate) | `function` | Declared here |
| [`SatpMode`](#satpmode) | `enum` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MMU`](#mmu) `inline` | [MMU](#mmu) Constructor. |
|  | [`~MMU`](#~mmu) `inline` | [MMU](#mmu) Destructor. |
| `MemoryReturn` | [`translate`](#translate)  | Translates Virtual address to Physical address. |

---



### MMU

`inline`

```cpp
inline MMU()
```

Defined in include/mmu.hpp:37

[MMU](#mmu) Constructor.

---



### ~MMU

`inline`

```cpp
inline ~MMU()
```

Defined in include/mmu.hpp:41

[MMU](#mmu) Destructor.

---



### translate

```cpp
MemoryReturn translate(Hart * hart, AccessType type, uint64_t va, uint64_t * pa)
```

Defined in include/mmu.hpp:167

Translates Virtual address to Physical address.

**See also**: [Hart](Hart.md)

#### Returns
Memory operation result

**See also**: MemoryReturn

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | `AccessType` | Access type |
| `va` | `uint64_t` | Virtual address |
| `pa` | `uint64_t *` | Pointer to Physical address to set |

## Public Types

| Name | Description |
|------|-------------|
| [`SatpMode`](#satpmode)  | SATP Mode. |

---



### SatpMode

```cpp
enum SatpMode
```

Defined in include/mmu.hpp:48

SATP Mode.

Defines current SATP CSR register MODE bits

| Value | Description |
|-------|-------------|
| `Bare` |  |
| `Sv39` | Direct access |
| `Sv48` | [Sv39](Sv39.md#sv39) Protection mode |
| `Sv57` | Sv48 Protection mode |

