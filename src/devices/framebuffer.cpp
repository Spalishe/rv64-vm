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
#include <stdexcept>
#ifdef USE_FRAMEBUFFER
#include "../../include/devices/framebuffer.hpp"
#include "../../include/libfdt.h"
#include "../../include/machine.hpp"
namespace rv64vm::dev
{
	Framebuffer::Framebuffer(uint64_t start, runner::Machine& cpu, fdt_node* fdt, size_t width, size_t height, AppWindow& window)
		: Device(start, width * height * 4 + 4, fdt, cpu.get_mmap()),
		  window(window),
		  cpu(cpu)
	{
		last_frame_time = std::chrono::steady_clock::now();
		// Use the registered memory region as the backing store. JIT stores to
		// the framebuffer can then use its host pointer directly instead of
		// exiting to MMIO::write() on every pixel.
		const size_t arr_size = size;
		cpu.get_mmap()->add_region(start, arr_size, true);
		buffer = cpu.get_mmap()->get_regions().back()->get_data();
		memset(buffer, 0, arr_size);
		if(fdt != NULL)
		{
			struct fdt_node* fb_fdt = fdt_node_create_reg("framebuffer", start);
			fdt_node_add_prop_reg(fb_fdt, "reg", start, size);
			fdt_node_add_prop(fb_fdt, "compatible", "simple-framebuffer", 18);
			fdt_node_add_prop_str(fb_fdt, "format", "x8r8g8b8");
			fdt_node_add_prop_u32(fb_fdt, "width", width);
			fdt_node_add_prop_u32(fb_fdt, "height", height);
			fdt_node_add_prop_u32(fb_fdt, "stride", width * 4);
			fdt_node* soc = fdt_node_find(fdt, "soc");
			fdt_node_add_child(soc, fb_fdt);
			fdt_node_free(soc);
		}
	}

	std::shared_ptr<Framebuffer> Framebuffer::init_auto(runner::Machine& cpu)
	{
		throw std::runtime_error("You can't create Framebuffer automatically!");
		// return std::make_shared<Framebuffer>(0x18000000, cpu, cpu.fdt, 640, 480);
	}

	uint64_t Framebuffer::read(uint64_t addr, MemorySize sz)
	{
		uint64_t offs = addr - start;
		if(offs >= size) return 0;
		switch(sz)
		{
			case MemorySize::Byte:
				return *(reinterpret_cast<uint8_t*>(buffer + offs));
			case MemorySize::Short:
				return *(reinterpret_cast<uint16_t*>(buffer + offs));
			case MemorySize::Int:
				return *(reinterpret_cast<uint32_t*>(buffer + offs));
			case MemorySize::Long:
				return *(reinterpret_cast<uint64_t*>(buffer + offs));
			default:
				return 0;
		}
	}

	void Framebuffer::write(uint64_t addr, MemorySize sz, uint64_t val)
	{
		uint64_t offs = addr - start;
		if(offs >= size) return;
		switch(sz)
		{
			case MemorySize::Byte:
				*(reinterpret_cast<uint8_t*>(buffer + offs)) = static_cast<uint8_t>(val);
				break;
			case MemorySize::Short:
				*(reinterpret_cast<uint16_t*>(buffer + offs)) = static_cast<uint16_t>(val);
				break;
			case MemorySize::Int:
				*(reinterpret_cast<uint32_t*>(buffer + offs)) = static_cast<uint32_t>(val);
				break;
			case MemorySize::Long:
				*(reinterpret_cast<uint64_t*>(buffer + offs)) = val;
				break;
		}
	}

	void Framebuffer::tick()
	{
		static constexpr auto FRAME_INTERVAL = std::chrono::microseconds(16667); // ~60 fps
		const auto now						 = std::chrono::steady_clock::now();
		if(now - last_frame_time >= FRAME_INTERVAL)
		{
			UpdateFrame(window, buffer);
			// Advance the deadline instead of resetting it to `now`, so a slow
			// tick does not accumulate jitter. Skip missed frames after a long
			// stall rather than firing a burst on the next tick.
			last_frame_time += FRAME_INTERVAL;
			if(now - last_frame_time >= FRAME_INTERVAL)
				last_frame_time = now;
		}
	}
}
#endif
