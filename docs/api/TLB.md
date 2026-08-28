

# TLB

```cpp
#include <tlb.hpp>

class TLB
```

Defined in include/tlb.hpp:30

RISC-V Translation Lookaside Buffer.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`TLB`](#tlb) | `function` | Declared here |
| [`~TLB`](#~tlb) | `function` | Declared here |
| [`lookup`](#lookup) | `function` | Declared here |
| [`insert`](#insert) | `function` | Declared here |
| [`flush_all`](#flush_all) | `function` | Declared here |
| [`flush_addr`](#flush_addr) | `function` | Declared here |
| [`flush_asid`](#flush_asid) | `function` | Declared here |
| [`flush_addr_asid`](#flush_addr_asid) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`TLB`](#tlb) `inline` | [TLB](#tlb) Constructor. |
|  | [`~TLB`](#~tlb) `inline` | [TLB](#tlb) Destructor. |
| [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`lookup`](#lookup)  | Looks up in cache for [TLB](#tlb) entry. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`insert`](#insert)  | Inserts new [TLB](#tlb) entry in cache. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`flush_all`](#flush_all) `inline` | Flushes all [TLB](#tlb) entries. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`flush_addr`](#flush_addr) `inline` | Flushes all [TLB](#tlb) entries by address. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`flush_asid`](#flush_asid) `inline` | Flushes all [TLB](#tlb) entries by ASID. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`flush_addr_asid`](#flush_addr_asid) `inline` | Flushes all [TLB](#tlb) entries by address and ASID. |

---



### TLB

`inline`

```cpp
inline TLB()
```

Defined in include/tlb.hpp:36

[TLB](#tlb) Constructor.

---



### ~TLB

`inline`

```cpp
inline ~TLB()
```

Defined in include/tlb.hpp:40

[TLB](#tlb) Destructor.

---



### lookup

```cpp
bool lookup(uint64_t va, AccessType type, uint16_t asid, int mode, bool mxr, bool sum, uint64_t * pa)
```

Defined in include/tlb.hpp:76

Looks up in cache for [TLB](#tlb) entry.

**See also**: [MMU](MMU.md)

**See also**: [Hart](Hart.md)

#### Returns
Is success?

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `va` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Virtual Address |
| `type` | [`AccessType`](#traps_8hpp_1a36b9a80a5a835ac5371a96e3eed57b9b) | Access type |
| `asid` | [`uint16_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | ASID |
| `mode` | [`int`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Privilage Mode casted to integer |
| `mxr` | [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [Hart](Hart.md#hart) Status MXR bit |
| `sum` | [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [Hart](Hart.md#hart) Status SUM bit |
| `pa` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | Physical Address pointer |

---



### insert

```cpp
void insert(uint64_t va, uint64_t pa, uint8_t page_bits, uint8_t perm, uint16_t asid, bool global)
```

Defined in include/tlb.hpp:87

Inserts new [TLB](#tlb) entry in cache.

**See also**: [MMU](MMU.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `va` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Virtual Address |
| `page_bits` | [`uint8_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Size of PPN page bits |
| `perm` | [`uint8_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Permissions bit set |
| `asid` | [`uint16_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Entry ASID |
| `global` | [`bool`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Entry G bit |

---



### flush_all

`inline`

```cpp
inline void flush_all()
```

Defined in include/tlb.hpp:92

Flushes all [TLB](#tlb) entries.

---



### flush_addr

`inline`

```cpp
inline void flush_addr(uint64_t)
```

Defined in include/tlb.hpp:98

Flushes all [TLB](#tlb) entries by address.

> [!NOTE]
> Currently does nothing; calls flush_all

---



### flush_asid

`inline`

```cpp
inline void flush_asid(uint16_t)
```

Defined in include/tlb.hpp:103

Flushes all [TLB](#tlb) entries by ASID.

> [!NOTE]
> Currently does nothing; calls flush_all

---



### flush_addr_asid

`inline`

```cpp
inline void flush_addr_asid(uint64_t, uint16_t)
```

Defined in include/tlb.hpp:108

Flushes all [TLB](#tlb) entries by address and ASID.

> [!NOTE]
> Currently does nothing; calls flush_all

