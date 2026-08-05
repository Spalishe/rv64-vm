

# Device

```cpp
#include <device.hpp>

struct Device
```

Defined in include/device.hpp:31

> **Inherits:** `enable_shared_from_this< Device >`
> **Subclassed by:** [`CLINT`](CLINT.md#clint), [`I2C`](I2C.md#i2c), [`PLIC`](PLIC.md#plic), [`SYSCON`](SYSCON.md#syscon), [`UART`](UART.md#uart), [`VirtIO_BLK`](VirtIO_BLK.md#virtio_blk)

[Device](#device) base structure.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`mmap`](#mmap) | `variable` | Declared here |
| [`start`](#start) | `variable` | Declared here |
| [`size`](#size) | `variable` | Declared here |
| [`end`](#end) | `variable` | Declared here |
| [`Device`](#device) | `function` | Declared here |
| [`read`](#read) | `function` | Declared here |
| [`write`](#write) | `function` | Declared here |
| [`tick`](#tick) | `function` | Declared here |
| [`get`](#get) | `function` | Declared here |

## Public Attributes

| Return | Name | Description |
|--------|------|-------------|
| `rv64vm::runner::MemoryMap *` | [`mmap`](#mmap)  | MMAP pointer. |
| `uint64_t` | [`start`](#start)  | [Device](#device) memory start address. |
| `uint64_t` | [`size`](#size)  | [Device](#device) memory size. |
| `uint64_t` | [`end`](#end)  | [Device](#device) memory end address. |

---



### mmap

```cpp
rv64vm::runner::MemoryMap * mmap
```

Defined in include/device.hpp:45

MMAP pointer.

> [!NOTE]
> Defined by constructor

---



### start

```cpp
uint64_t start
```

Defined in include/device.hpp:50

[Device](#device) memory start address.

> [!NOTE]
> Defined by constructor

---



### size

```cpp
uint64_t size
```

Defined in include/device.hpp:55

[Device](#device) memory size.

> [!NOTE]
> Defined by constructor

---



### end

```cpp
uint64_t end
```

Defined in include/device.hpp:60

[Device](#device) memory end address.

> [!NOTE]
> Defined by constructor

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Device`](#device) `inline` | [Device](#device) constructor. |
| `uint64_t` | [`read`](#read) `virtual` `inline` | [Device](#device) read function. |
| `void` | [`write`](#write) `virtual` `inline` | [Device](#device) write function. |
| `void` | [`tick`](#tick) `virtual` `inline` | [Device](#device) tick function. |
| `std::shared_ptr< T >` | [`get`](#get) `inline` | Returns device. |

---



### Device

`inline`

```cpp
inline Device(uint64_t start, uint64_t size, fdt_node * fdt, rv64vm::runner::MemoryMap * mmap)
```

Defined in include/device.hpp:37

[Device](#device) constructor.

> [!NOTE]
> You must run all FDT functions here.

---



### read

`virtual` `inline`

```cpp
virtual inline uint64_t read(uint64_t addr, MemorySize size)
```

Defined in include/device.hpp:65

[Device](#device) read function.

#### Reimplemented by

- [`read`](CLINT.md#read-5)
- [`read`](I2C.md#read-2)
- [`read`](PLIC.md#read-3)
- [`read`](SYSCON.md#read-6)
- [`read`](UART.md#read-4)
- [`read`](VirtIO_BLK.md#read-7)

---



### write

`virtual` `inline`

```cpp
virtual inline void write(uint64_t addr, MemorySize size, uint64_t val)
```

Defined in include/device.hpp:69

[Device](#device) write function.

#### Reimplemented by

- [`write`](CLINT.md#write-5)
- [`write`](I2C.md#write-2)
- [`write`](PLIC.md#write-3)
- [`write`](SYSCON.md#write-6)
- [`write`](VirtIO_BLK.md#write-7)
- [`write`](UART.md#write-4)

---



### tick

`virtual` `inline`

```cpp
virtual inline void tick()
```

Defined in include/device.hpp:74

[Device](#device) tick function.

This function would call [almost(optimization)](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) every tick

#### Reimplemented by

- [`tick`](CLINT.md#tick-3)
- [`tick`](I2C.md#tick-1)
- [`tick`](PLIC.md#tick-2)

---



### get

`inline`

```cpp
template<typenameT> inline std::shared_ptr< T > get()
```

Defined in include/device.hpp:81

Returns device.

> [!WARNING]
> You would break something if you overwrite this.

