

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
| [`PrivilegeMode`](#privilegemode) | `enum` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Hart`](#hart)  | [Hart](#hart) constructor. |
|  | [`~Hart`](#~hart) `inline` | [Hart](#hart) destructor. |

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

