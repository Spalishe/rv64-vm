/*
Copyright 2026 Spalishe

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#pragma once

#include <cstdint>

/**
 * @defgroup RV64VM-API Emulator API
 * @brief Entire documentation for RV64-VM
 */

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

		class XHCI;
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
#include "devices/usb/xhci.hpp"			// IWYU pragma: export
#include "devices/virtio_blk.hpp"		// IWYU pragma: export
#include "hart.hpp"						// IWYU pragma: export
#include "machine.hpp"					// IWYU pragma: export
#include "memory_map.hpp"				// IWYU pragma: export
#include "mmio.hpp"						// IWYU pragma: export
