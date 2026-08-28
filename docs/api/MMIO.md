

# MMIO

```cpp
#include <mmio.hpp>

class MMIO
```

Defined in include/mmio.hpp:36

RV64-VM Memory-Mapped Input/Output controller.

This class implements RISC-V basic [MMIO](#mmio) structure which holds all devices

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`devs`](#devs) | `variable` | Declared here |
| [`MMIO`](#mmio) | `function` | Declared here |
| [`~MMIO`](#~mmio) | `function` | Declared here |
| [`write`](#write) | `function` | Declared here |
| [`read`](#read) | `function` | Declared here |
| [`create_device`](#create_device) | `function` | Declared here |
| [`create_device_auto`](#create_device_auto) | `function` | Declared here |
| [`tick_all`](#tick_all) | `function` | Declared here |
| [`get`](#get) | `function` | Declared here |

## Public Attributes

| Return | Name | Description |
|--------|------|-------------|
| std::vector< std::shared_ptr<[`::rv64vm::dev::Device`](Device.md#device) > > | [`devs`](#devs)  | Device list. |

---



### devs

```cpp
std::vector< std::shared_ptr<::rv64vm::dev::Device > > devs
```

Defined in include/mmio.hpp:54

Device list.

Contains list of all created and using devices in system.

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`MMIO`](#mmio)  | [MMIO](#mmio) constructor. |
|  | [`~MMIO`](#~mmio) `inline` | [MMIO](#mmio) destructor. |
| [`MemoryReturn`](#structmemoryreturn) | [`write`](#write)  | Write operation. |
| [`MemoryReturn`](#structmemoryreturn) | [`read`](#read)  | Read operation. |
| std::shared_ptr< [`T`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) > | [`create_device`](#create_device) `inline` | Creates new device. |
| std::shared_ptr< [`T`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) > | [`create_device_auto`](#create_device_auto) `inline` | Creates new device automatically. |
| [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | [`tick_all`](#tick_all) `inline` | Devices tick function. |
| std::shared_ptr< [`T`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) > | [`get`](#get) `inline` | Device getter function. |

---



### MMIO

```cpp
MMIO(MemoryMap * mmap, uint64_t mem_size)
```

Defined in include/mmio.hpp:43

[MMIO](#mmio) constructor.

Creates [MMIO](#mmio) object

---



### ~MMIO

`inline`

```cpp
inline ~MMIO()
```

Defined in include/mmio.hpp:48

[MMIO](#mmio) destructor.

Removes [MMIO](#mmio) object

---



### write

```cpp
MemoryReturn write(Hart & h, uint64_t addr, MemorySize size, uint64_t val, bool isphys = false)
```

Defined in include/mmio.hpp:67

Write operation.

Writes data to DRAM. If defined address is beyond DRAM base address then it check for all devices and writes data to them. 
#### Returns
Memory operation data

**See also**: MemoryReturn

**See also**: [Hart](Hart.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `h` | [`Hart`](Hart.md#hart) & | [Hart](Hart.md#hart) reference |
| `size` | [`MemorySize`](#traps_8hpp_1abae976fafb49b2f9df8f4b6468015481) | Operation data size |
| `val` | [`uint64_t`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) | Value @data isphys Is address physical or virtual |

---



### read

```cpp
MemoryReturn read(Hart & h, uint64_t addr, MemorySize size, void * val, bool isphys = false)
```

Defined in include/mmio.hpp:79

Read operation.

Reads data from DRAM. If defined address is beyond DRAM base address then it check for all devices and reads their memory. 
#### Returns
Memory operation data

**See also**: MemoryReturn

**See also**: [Hart](Hart.md)

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `h` | [`Hart`](Hart.md#hart) & | [Hart](Hart.md#hart) reference |
| `size` | [`MemorySize`](#traps_8hpp_1abae976fafb49b2f9df8f4b6468015481) | Operation data size |
| `val` | [`void`](#virtio__blk_8hpp_1a0e89cf6b9f6cd3125470b1bed2b823df) * | Pointer to new value @data isphys Is address physical or virtual |

---



### create_device

`inline`

```cpp
template<typenameT, typename... Args> inline std::shared_ptr< T > create_device(Args &&... args)
```

Defined in include/mmio.hpp:86

Creates new device.

Creates new T device and automatically adds it to device list.

---



### create_device_auto

`inline`

```cpp
template<typenameT> inline std::shared_ptr< T > create_device_auto(Machine & cpu)
```

Defined in include/mmio.hpp:98

Creates new device automatically.

Creates new T device by calling it auto create function. 
> [!NOTE]
> It is recommended to use this function to create devices.

---



### tick_all

`inline`

```cpp
inline void tick_all()
```

Defined in include/mmio.hpp:110

Devices tick function.

Wrapper that automatically will call every registered device tick function 
> [!NOTE]
> This function automatically calls in [Machine](Machine.md#machine), no need to call it manually **unless you have a reason**.

---



### get

`inline`

```cpp
template<typenameT> inline std::shared_ptr< T > get()
```

Defined in include/mmio.hpp:126

Device getter function.

Returns first-found T from device list

