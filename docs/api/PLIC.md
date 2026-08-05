

# PLIC

```cpp
#include <plic.hpp>

class PLIC
```

Defined in include/devices/plic.hpp:27

> **Inherits:** [`Device`](Device.md#device)

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`read`](#read-3) | `function` | Declared here |
| [`write`](#write-3) | `function` | Declared here |
| [`tick`](#~tick) | `function` | Declared here |
| [`mmap`](Device.md#mmap) | `variable` | Inherited from [`Device`](Device.md#device) |
| [`start`](Device.md#start) | `variable` | Inherited from [`Device`](Device.md#device) |
| [`size`](Device.md#size) | `variable` | Inherited from [`Device`](Device.md#device) |
| [`end`](Device.md#end) | `variable` | Inherited from [`Device`](Device.md#device) |
| [`Device`](Device.md#device-1) | `function` | Inherited from [`Device`](Device.md#device) |
| [`read`](Device.md#read) | `function` | Inherited from [`Device`](Device.md#device) |
| [`write`](Device.md#write) | `function` | Inherited from [`Device`](Device.md#device) |
| [`tick`](Device.md#tick) | `function` | Inherited from [`Device`](Device.md#device) |
| [`get`](Device.md#get) | `function` | Inherited from [`Device`](Device.md#device) |

## Inherited from [`Device`](Device.md#device)

| Kind | Name | Description |
|------|------|-------------|
| `variable` | [`mmap`](Device.md#mmap)  | MMAP pointer. |
| `variable` | [`start`](Device.md#start)  | [Device](Device.md#device) memory start address. |
| `variable` | [`size`](Device.md#size)  | [Device](Device.md#device) memory size. |
| `variable` | [`end`](Device.md#end)  | [Device](Device.md#device) memory end address. |
| `function` | [`Device`](Device.md#device-1) `inline` | [Device](Device.md#device) constructor. |
| `function` | [`read`](Device.md#read) `virtual` `inline` | [Device](Device.md#device) read function. |
| `function` | [`write`](Device.md#write) `virtual` `inline` | [Device](Device.md#device) write function. |
| `function` | [`tick`](Device.md#tick) `virtual` `inline` | [Device](Device.md#device) tick function. |
| `function` | [`get`](Device.md#get) `inline` | Returns device. |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
| `uint64_t` | [`read`](#read-3) `virtual` | [Device](Device.md#device) read function. |
| `void` | [`write`](#write-3) `virtual` | [Device](Device.md#device) write function. |
| `void` | [`tick`](#~tick) `virtual` | [Device](Device.md#device) tick function. |

---



### read

`virtual`

```cpp
virtual uint64_t read(uint64_t addr, MemorySize size)
```

Defined in include/devices/plic.hpp:34

[Device](Device.md#device) read function.

#### Reimplements

- [`read`](Device.md#read)

---



### write

`virtual`

```cpp
virtual void write(uint64_t addr, MemorySize size, uint64_t val)
```

Defined in include/devices/plic.hpp:35

[Device](Device.md#device) write function.

#### Reimplements

- [`write`](Device.md#write)

---



### tick

`virtual`

```cpp
virtual void tick()
```

Defined in include/devices/plic.hpp:36

[Device](Device.md#device) tick function.

This function would call [almost(optimization)](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) every tick

#### Reimplements

- [`tick`](Device.md#tick)

