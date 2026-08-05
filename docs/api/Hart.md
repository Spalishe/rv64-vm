

# Hart

```cpp
#include <hart.hpp>

class Hart
```

Defined in include/hart.hpp:37

RISC-V CPU Core.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`Hart`](#hart) | `function` | Declared here |
| [`~Hart`](#~hart) | `function` | Declared here |
| [`get_mmio`](#get_mmio) | `function` | Declared here |
| [`get_mmap`](#get_mmap) | `function` | Declared here |
| [`get_reservation`](#get_reservation) | `function` | Declared here |
| [`clear_decode_cache`](#clear_decode_cache) | `function` | Declared here |
| [`amo_check_reservation`](#amo_check_reservation) | `function` | Declared here |
| [`csr_read`](#csr_read) | `function` | Declared here |
| [`csr_write`](#csr_write) | `function` | Declared here |
| [`trap`](#trap) | `function` | Declared here |
| [`PrivilegeMode`](#privilegemode) | `enum` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Hart`](#hart)  | [Hart](#hart) constructor. |
|  | [`~Hart`](#~hart) `inline` | [Hart](#hart) destructor. |
| `MMIO *` | [`get_mmio`](#get_mmio) `inline` | Returns [MMIO](MMIO.md#mmio) pointer. |
| `MemoryMap *` | [`get_mmap`](#get_mmap) `inline` | Returns [MemoryMap](MemoryMap.md#memorymap) pointer. |
| `Reservation &` | [`get_reservation`](#get_reservation) `inline` | Returns CPU Atomic [Reservation](Reservation.md#reservation). |
| `void` | [`clear_decode_cache`](#clear_decode_cache) `inline` | Clears [Instruction](#structrv64vm_1_1runner_1_1instruction) Decoder Cache. |
| `void` | [`amo_check_reservation`](#amo_check_reservation) `inline` | Clears reservation if defined address is within CPU reservation address. |
| `uint64_t` | [`csr_read`](#csr_read)  | Returns value stored in CSR. |
| `void` | [`csr_write`](#csr_write)  | Stores value to CSR. |
| `void` | [`trap`](#trap)  | CPU Trap function. |

---



### Hart

```cpp
Hart(uint8_t id, uint64_t memsize)
```

Defined in include/hart.hpp:69

[Hart](#hart) constructor.

Creates RISC-V core 
> [!NOTE]
> It must be equal with memory size defined in machine config.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `id` | `uint8_t` | Internal [Hart](#hart) ID (starts from 0) |
| `memsize` | `uint64_t` | Memory size |

---



### ~Hart

`inline`

```cpp
inline ~Hart()
```

Defined in include/hart.hpp:76

[Hart](#hart) destructor.

Destroys RISC-V core

---



### get_mmio

`inline`

```cpp
inline MMIO * get_mmio()
```

Defined in include/hart.hpp:121

Returns [MMIO](MMIO.md#mmio) pointer.

**See also**: [MMIO](MMIO.md)

#### Returns
[MMIO](MMIO.md#mmio) Pointer

---



### get_mmap

`inline`

```cpp
inline MemoryMap * get_mmap()
```

Defined in include/hart.hpp:127

Returns [MemoryMap](MemoryMap.md#memorymap) pointer.

**See also**: [MemoryMap](MemoryMap.md)

#### Returns
[MemoryMap](MemoryMap.md#memorymap) pointer

---



### get_reservation

`inline`

```cpp
inline Reservation & get_reservation()
```

Defined in include/hart.hpp:133

Returns CPU Atomic [Reservation](Reservation.md#reservation).

**See also**: [Reservation](Reservation.md)

#### Returns
[Reservation](Reservation.md#reservation) reference object

---



### clear_decode_cache

`inline`

```cpp
inline void clear_decode_cache()
```

Defined in include/hart.hpp:137

Clears [Instruction](#structrv64vm_1_1runner_1_1instruction) Decoder Cache.

---



### amo_check_reservation

`inline`

```cpp
inline void amo_check_reservation(uint64_t va)
```

Defined in include/hart.hpp:150

Clears reservation if defined address is within CPU reservation address.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `va` | `uint64_t` | Virtual Address |

---



### csr_read

```cpp
uint64_t csr_read(uint16_t csr)
```

Defined in include/hart.hpp:169

Returns value stored in CSR.

#### Returns
CSR value

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `csr` | `uint16_t` | CSR address |

---



### csr_write

```cpp
void csr_write(uint16_t csr, uint64_t val)
```

Defined in include/hart.hpp:175

Stores value to CSR.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `csr` | `uint16_t` | CSR address |
| `val` | `uint64_t` | Value |

---



### trap

```cpp
void trap(uint64_t cause, uint64_t tval, bool interrupt)
```

Defined in include/hart.hpp:184

CPU Trap function.

Raises trap in CPU core.

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cause` | `uint64_t` | Trap cause |
| `tval` | `uint64_t` | Trap value (can be zero) |
| `interrupt` | `bool` | Is trap will be [interrupt(true)](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) of [exception(false)](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df)? |

## Public Types

| Name | Description |
|------|-------------|
| [`PrivilegeMode`](#privilegemode)  | CPU PrivilegeMode. |

---



### PrivilegeMode

```cpp
enum PrivilegeMode
```

Defined in include/hart.hpp:44

CPU PrivilegeMode.

Current CPU privilege mode.

| Value | Description |
|-------|-------------|
| `User` |  |
| `Supervisor` |  |
| `Hypervisor` |  |
| `Machine` |  |

