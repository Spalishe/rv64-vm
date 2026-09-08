

# TLB

```cpp
#include <tlb.hpp>

class TLB
```

Defined in include/tlb.hpp:30

RISC-V Translation Lookaside Buffer.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`TLB`](#tlb) `inline` | [TLB](#tlb) Constructor. |
|  | [`~TLB`](#~tlb) `inline` | [TLB](#tlb) Destructor. |
| `bool` | [`lookup`](#lookup)  | Looks up in cache for [TLB](#tlb) entry. |
| `void` | [`insert`](#insert)  | Inserts new [TLB](#tlb) entry in cache. |
| `void` | [`flush_all`](#flush_all) `inline` | Flushes all [TLB](#tlb) entries. |
| `void` | [`flush_addr`](#flush_addr) `inline` | Flushes all [TLB](#tlb) entries by address. |
| `void` | [`flush_asid`](#flush_asid) `inline` | Flushes all [TLB](#tlb) entries by ASID. |
| `void` | [`flush_addr_asid`](#flush_addr_asid) `inline` | Flushes all [TLB](#tlb) entries by address and ASID. |

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
| `va` | `uint64_t` | Virtual Address |
| `type` | [`AccessType`](#traps_8hpp_1a36b9a80a5a835ac5371a96e3eed57b9b) | Access type |
| `asid` | `uint16_t` | ASID |
| `mode` | `int` | Privilage Mode casted to integer |
| `mxr` | `bool` | [Hart](Hart.md#hart) Status MXR bit |
| `sum` | `bool` | [Hart](Hart.md#hart) Status SUM bit |
| `pa` | `uint64_t *` | Physical Address pointer |

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
| `va` | `uint64_t` | Virtual Address |
| `page_bits` | `uint8_t` | Size of PPN page bits |
| `perm` | `uint8_t` | Permissions bit set |
| `asid` | `uint16_t` | Entry ASID |
| `global` | `bool` | Entry G bit |

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

