

# VirtIO_BLK

```cpp
#include <virtio_blk.hpp>

class VirtIO_BLK
```

Defined in include/devices/virtio_blk.hpp:142

> **Inherits:** [`Device`](Device.md#device)

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`read`](#read-7) | `function` | Declared here |
| [`write`](#write-7) | `function` | Declared here |
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

## Private Methods

| Return | Name | Description |
|--------|------|-------------|
| `uint64_t` | [`read`](#read-7) `virtual` | [Device](Device.md#device) read function. |
| `void` | [`write`](#write-7) `virtual` | [Device](Device.md#device) write function. |

---



### read

`virtual`

```cpp
virtual uint64_t read(uint64_t addr, MemorySize size)
```

Defined in include/devices/virtio_blk.hpp:150

[Device](Device.md#device) read function.

#### Reimplements

- [`read`](Device.md#read)

---



### write

`virtual`

```cpp
virtual void write(uint64_t addr, MemorySize size, uint64_t val)
```

Defined in include/devices/virtio_blk.hpp:151

[Device](Device.md#device) write function.

#### Reimplements

- [`write`](Device.md#write)

