

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
| `std::vector< std::shared_ptr<::rv64vm::dev::Device > >` | [`devs`](#devs)  | Device list. |

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
| `MemoryReturn` | [`write`](#write)  | Write operation. |
| `MemoryReturn` | [`read`](#read)  | Read operation. |
| `std::shared_ptr< T >` | [`create_device`](#create_device) `inline` | Creates new device. |
| `std::shared_ptr< T >` | [`create_device_auto`](#create_device_auto) `inline` | Creates new device automatically. |
| `void` | [`tick_all`](#tick_all) `inline` | Devices tick function. |
| `std::shared_ptr< T >` | [`get`](#get) `inline` | Device getter function. |

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
MemoryReturn write(Hart & h, uint64_t vaddr, MemorySize size, uint64_t val)
```

Defined in include/mmio.hpp:60

Write operation.

Writes data to DRAM. If defined address is beyond DRAM base address then it check for all devices and writes data to them.

---



### read

```cpp
MemoryReturn read(Hart & h, uint64_t vaddr, MemorySize size, void * val)
```

Defined in include/mmio.hpp:65

Read operation.

Reads data from DRAM. If defined address is beyond DRAM base address then it check for all devices and reads their memory.

---



### create_device

`inline`

```cpp
template<typenameT, typename... Args> inline std::shared_ptr< T > create_device(Args &&... args)
```

Defined in include/mmio.hpp:72

Creates new device.

Creates new T device and automatically adds it to device list.

---



### create_device_auto

`inline`

```cpp
template<typenameT> inline std::shared_ptr< T > create_device_auto(Machine & cpu)
```

Defined in include/mmio.hpp:84

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

Defined in include/mmio.hpp:96

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

Defined in include/mmio.hpp:112

Device getter function.

Returns first-found T from device list

