

# TLB

```cpp
#include <tlb.hpp>

class TLB
```

Defined in include/tlb.hpp:33

RISC-V Translation Lookaside Buffer.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`TLB`](#tlb) `inline` | [TLB](#tlb) Constructor. |
|  | [`~TLB`](#~tlb) `inline` | [TLB](#tlb) Destructor. |
| `bool` | [`lookup`](#lookup)  | Looks up in cache for [TLB](#tlb) entry. |
| `void` | [`insert`](#insert)  | Inserts new [TLB](#tlb) entry in cache. |
| `void` | [`note_exec`](#note_exec) `inline` | Strips the write/dirty capability from the entry for `va`. |
| `void` | [`flush_all`](#flush_all) `inline` | Flushes all [TLB](#tlb) entries. |
| `void` | [`flush_addr`](#flush_addr) `inline` | Flushes [TLB](#tlb) entries by address (SFENCE.VMA rs1) |
| `void` | [`flush_asid`](#flush_asid) `inline` | Flushes all [TLB](#tlb) entries of an ASID (SFENCE.VMA x0, rs2) |
| `void` | [`flush_addr_asid`](#flush_addr_asid) `inline` | Flushes a single address mapping of an ASID (SFENCE.VMA rs1, rs2) |

---



### TLB

`inline`

```cpp
inline TLB()
```

Defined in include/tlb.hpp:39

[TLB](#tlb) Constructor.

---



### ~TLB

`inline`

```cpp
inline ~TLB()
```

Defined in include/tlb.hpp:43

[TLB](#tlb) Destructor.

---



### lookup

```cpp
bool lookup(uint64_t va, AccessType type, uint16_t asid, int mode, bool mxr, bool sum, uint64_t * pa)
```

Defined in include/tlb.hpp:90

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
void insert(uint64_t va, uint64_t pa, uint8_t page_bits, uint8_t perm, uint16_t asid, bool global, const void * host_page = nullptr)
```

Defined in include/tlb.hpp:104

Inserts new [TLB](#tlb) entry in cache.

**See also**: [MMU](MMU.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `va` | `uint64_t` | Virtual Address |
| `pa` | `uint64_t` | Physical address (any byte inside the target page) |
| `page_bits` | `uint8_t` | Size of PPN page bits |
| `perm` | `uint8_t` | Permissions bit set |
| `asid` | `uint16_t` | Entry ASID |
| `global` | `bool` | Entry G bit |
| `host_page` | `const void *` | Host pointer to the start of the guest page, or nullptr when the page has no direct host mapping (MMIO/IO) |

---



### note_exec

`inline`

```cpp
inline void note_exec(uint64_t va)
```

Defined in include/tlb.hpp:117

Strips the write/dirty capability from the entry for `va`.

W^X: once a page is executed, JITed stores must miss the [TLB](#tlb) so stores fall back to the interpreter, which detects self-modifying writes and invalidates compiled code.

---



### flush_all

`inline`

```cpp
inline void flush_all()
```

Defined in include/tlb.hpp:129

Flushes all [TLB](#tlb) entries.

---



### flush_addr

`inline`

```cpp
inline void flush_addr(uint64_t va)
```

Defined in include/tlb.hpp:146

Flushes [TLB](#tlb) entries by address (SFENCE.VMA rs1)

The [TLB](#tlb) is direct-mapped on (va >> 12); any resident entry that could serve an access to `va` lives at index(va), regardless of page size. Invalidating in place (generation = 0) instead of bumping the global generation keeps every unrelated compiled JIT block and its baked [TLB](#tlb) checks alive; the affected slot alone fails its baked check, gets refilled by the C++ page walk, and the stale block then runs against the fresh entry.

---



### flush_asid

`inline`

```cpp
inline void flush_asid(uint16_t asid)
```

Defined in include/tlb.hpp:155

Flushes all [TLB](#tlb) entries of an ASID (SFENCE.VMA x0, rs2)

---



### flush_addr_asid

`inline`

```cpp
inline void flush_addr_asid(uint64_t va, uint16_t asid)
```

Defined in include/tlb.hpp:164

Flushes a single address mapping of an ASID (SFENCE.VMA rs1, rs2)

