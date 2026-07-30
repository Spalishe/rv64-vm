

# Machine

```cpp
#include <machine.hpp>

class Machine
```

Defined in include/machine.hpp:52

RV64-VM Main machine class.

This class implements RISC-V emulator machine.

## List of all members

| Name | Kind | Owner |
|------|------|-------|
| [`Machine`](#machine-1) | `function` | Declared here |
| [`~Machine`](#machine-2) | `function` | Declared here |

## Public Methods

| Return | Name | Description |
|--------|------|-------------|
|  | [`Machine`](#machine-1)  | [Machine](#machine) constructor. |
|  | [`~Machine`](#machine-2)  | [Machine](#machine) destructor. |

---



### Machine

```cpp
Machine(const MachineConfig & cfg)
```

Defined in include/machine.hpp:60

[Machine](#machine) constructor.

Creates RISC-V machine

#### Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `cfg` | `const MachineConfig &` | [Machine](#machine) configuration |

---



### ~Machine

```cpp
~Machine()
```

Defined in include/machine.hpp:66

[Machine](#machine) destructor.

Destroys RISC-V machine 
> [!NOTE]
> It is recommended to stop machine before destroy it.

