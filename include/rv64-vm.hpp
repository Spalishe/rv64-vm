#pragma once

#include <cstdint>

// -- Main rv64-vm namespace --
namespace rv64vm
{
	// -- Core machine subspace --
	// Houses main CPU classes
	namespace runner
	{
		struct MachineConfig;
		enum class MachineState : uint8_t;

		class Machine;
		class Hart;
		class MMIO;
		class MemoryMap;
		class InstructionDecoder;
		class ELFParser;

		struct InstructionCache;
		struct InstructionData;
		struct Instruction;
	}
	namespace dev
	{
		struct Device;

		class UART;
		class PLIC;
		class CLINT;
		class SYSCON;
		class I2C;
		struct I2CSlave;
		class VirtIO_BLK;
#ifdef USE_FRAMEBUFFER
		class Framebuffer;
#endif
		struct HIDOverI2C;
		struct HID_Keyboard;
	}
}

#include "device.hpp"					// IWYU pragma: export
#include "devices/clint.hpp"			// IWYU pragma: export
#include "devices/framebuffer.hpp"		// IWYU pragma: export
#include "devices/hid/hid-over-i2c.hpp" // IWYU pragma: export
#include "devices/hid/hid_keyboard.hpp" // IWYU pragma: export
#include "devices/i2c/i2c-core.hpp"		// IWYU pragma: export
#include "devices/i2c/i2c-slave.hpp"	// IWYU pragma: export
#include "devices/plic.hpp"				// IWYU pragma: export
#include "devices/syscon.hpp"			// IWYU pragma: export
#include "devices/uart.hpp"				// IWYU pragma: export
#include "devices/virtio_blk.hpp"		// IWYU pragma: export
#include "hart.hpp"						// IWYU pragma: export
#include "machine.hpp"					// IWYU pragma: export
#include "memory_map.hpp"				// IWYU pragma: export
#include "mmio.hpp"						// IWYU pragma: export
