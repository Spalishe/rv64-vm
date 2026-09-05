

# MMU

```cpp
#include <mmu.hpp>

class MMU
```

Defined in include/mmu.hpp:34

RISC-V Memory Management Unit.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MMU`](#mmu) `inline` | [MMU](#mmu) Constructor. |
|  | [`~MMU`](#~mmu) `inline` | [MMU](#mmu) Destructor. |
| [`TLB`](TLB.md#tlb) & | [`get_tlb`](#get_tlb) `inline` | Returns [TLB](TLB.md#tlb) reference. |
| [`MemoryReturn`](#structmemoryreturn) | [`translate`](#translate)  | Translates Virtual address to Physical address. |

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

Defined in include/mmu.hpp:171

Translates Virtual address to Physical address.

**See also**: [Hart](Hart.md)

#### Returns
Memory operation result

**See also**: MemoryReturn

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `type` | [`AccessType`](#traps_8hpp_1a36b9a80a5a835ac5371a96e3eed57b9b) | Access type |
| `va` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Virtual address |
| `pa` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | Pointer to Physical address to set |

## Public Types

| Name | Description |
|------|-------------|
| [`SatpMode`](#satpmode)  | SATP Mode. |

---



### SatpMode

```cpp
enum SatpMode
```

Type: [`int`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df)

Defined in include/mmu.hpp:53

SATP Mode.

Defines current SATP CSR register MODE bits

| Value | Description |
|-------|-------------|
| `Bare` |  |
| `Sv39` | Direct access |
| `Sv48` | [Sv39](Sv39.md#sv39) Protection mode |
| `Sv57` | Sv48 Protection mode |
