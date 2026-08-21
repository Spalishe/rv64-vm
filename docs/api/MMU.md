

# MMU

```cpp
#include <mmu.hpp>

class MMU
```

Defined in include/mmu.hpp:34

RISC-V Memory Management Unit.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`MMU`](#mmu) | `function` | Declared here |
| [`~MMU`](#~mmu) | `function` | Declared here |
| [`get_tlb`](#get_tlb) | `function` | Declared here |
| [`translate`](#translate) | `function` | Declared here |
| [`SatpMode`](#satpmode) | `enum` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MMU`](#mmu) `inline` | [MMU](#mmu) Constructor. |
|  | [`~MMU`](#~mmu) `inline` | [MMU](#mmu) Destructor. |
| `TLB &` | [`get_tlb`](#get_tlb) `inline` | Returns [TLB](TLB.md#tlb) reference. |
| `MemoryReturn` | [`translate`](#translate)  | Translates Virtual address to Physical address. |

---



### MMU

`inline`

```cpp
inline MMU()
```

Defined in include/mmu.hpp:40

[MMU](#mmu) Constructor.

---



### ~MMU

`inline`

```cpp
inline ~MMU()
```

Defined in include/mmu.hpp:46

[MMU](#mmu) Destructor.

---



### get_tlb

`inline`

```cpp
inline TLB & get_tlb()
```

Defined in include/mmu.hpp:66

Returns [TLB](TLB.md#tlb) reference.

**See also**: [TLB](TLB.md)

#### Returns
[TLB](TLB.md#tlb) reference

---



### translate

```cpp
MemoryReturn translate(Hart * hart, AccessType type, uint64_t va, uint64_t * pa)
```

Defined in include/mmu.hpp:179

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

Defined in include/mmu.hpp:53

SATP Mode.

Defines current SATP CSR register MODE bits

| Value | Description |
|-------|-------------|
| `Bare` |  |
| `Sv39` | Direct access |
| `Sv48` | [Sv39](Sv39.md#sv39) Protection mode |
| `Sv57` | Sv48 Protection mode |

