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

#include "argparser.cpp"
#include <atomic>
#include <cstdio>
#include <exception>
#include <iostream>
#include <string>
#include <unordered_map>

#include "../include/gui/wayland/wayland.hpp"
#include "../include/gui/x11.hpp"

#include "../include/devices/framebuffer.hpp"
#include "../include/devices/hid/hid_keyboard.hpp"
#include "../include/devices/i2c/i2c-core.hpp"
#include "../include/devices/uart.hpp"
#include "../include/gdbstub.hpp"
#include "../include/machine.hpp"
#include "fcntl.h"
#include "termios.h"
#include <thread>

/*
 *		   TODO:
 *			-MMU
 *			-PMP
 *			-SPMP
 *		    -JIT:
 *				- MMU support(Software TLB)
 *				- 2-Pass Branch tags
 *				- AUIPC
 *				- LUI
 *				- RVC
 *		    -Zawrs
 *		    -Zabha
 *		    -Zacas
 *		    -Zbkb
 *		    -Machine suspend
 *          -Device:
 *            1. RTC GoldFish
 *            2. VirtIO-GPU
 *
 *          -Possible, but stupid ideas:
 *			1. Smcntrpmf
 *			2. Scountovf
 *			3. Smstateen
 *			4. Ssstateen
 *			5. Mmaia
 *			6. Smaia
 *
 */

termios oldt;
std::atomic<bool> termios_running = false;
std::vector<char> combo_sequence  = { 3, 24 }; // Ctrl+C (0x03) -> Ctrl+X (0x18)
std::vector<char> buffer;
std::shared_ptr<UART> uart;

Machine* mach;
// Thread function for overriding default stdin control keys
void termios_loop()
{
	int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	char ch;
	while(termios_running.load(std::memory_order_relaxed))
	{
		ssize_t r = read(STDIN_FILENO, &ch, 1);
		if(r > 0)
		{
			buffer.push_back(ch);

			if(buffer.size() > combo_sequence.size())
			{
				buffer.erase(buffer.begin());
			}

			if(buffer.size() == combo_sequence.size() && buffer == combo_sequence)
			{
				mach->stop();
				buffer.clear();
				break;
			}

			uart->receive_byte(ch);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // dont make our cpu cry
		}
	}
	printf("exit\n");
}
void cleanup_terminal()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int main(int argc, char* argv[])
{
	arp::Argparser parser(argc, argv);
	parser.setDescription("RISC-V EM");
	auto bios_var
		= parser.add<arp::str>("--bios", "File with Machine level program (bootloader)", arp::required, arp::nopos);
	auto kernel_var
		= parser.add<arp::str>("--kernel", "File with Supervisor Level program", arp::norequired, arp::nopos);
	auto image_var = parser.add<arp::str>("--image", "File with Image file that will put on VirtIO-BLK",
										  arp::norequired, arp::nopos);

	auto dtb_var
		= parser.add<arp::str>("--dtb", "Use specified FDT instead of auto-generated", arp::norequired, arp::nopos);
	auto dumpdtb_var
		= parser.add<arp::str>("--dumpdtb", "Dumps auto-generated FDT to file", arp::norequired, arp::nopos);
	auto append_var = parser.add<arp::str>("--append", "Append command line arguments", arp::norequired, arp::nopos);
#ifdef USE_GDBSTUB
	auto gdb_var = parser.add<arp::def>("--gdb", "Starts GDB Stub on port 1512", arp::norequired, arp::nopos);
#endif
	auto mem_var = parser.add<arp::str>("--memsize", "Set custom memory size (Default is 512 MB)", arp::norequired,
										arp::nopos, "-M");
	auto harts_var
		= parser.add<arp::uint>("--harts", "Set custom harts count (Default is 1)", arp::norequired, arp::nopos, "-S");
#ifdef USE_FRAMEBUFFER
	auto fb_var
		= parser.add<arp::str>("--framebuffer", "Enables framebuffer with defined size (F.e. 640x480)", arp::norequired, arp::nopos, "-fb");
#endif

	parser.parse();

	uint64_t memsize = 1024 * 1024 * 512; // 512 MB
	if(mem_var->defined())
	{
		std::string s = mem_var->val();
		char l		  = s.back();
		s.pop_back();

		std::unordered_map<char, int8_t> symbol_to_shift = {
			{ 'B', -10 },
			{ 'K', 1	 },
			{ 'M', 10  },
			{ 'G', 20  },
		};

		uint64_t mul = 0;
		try
		{
			mul = stoull(s);
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Size '" << s << "' is not a number." << std::endl;
			return -1;
		}
		uint64_t size = 1024;
		size		  = size << symbol_to_shift[l];
		size *= mul;
		memsize = size;
	}
	uint8_t harts = 1;
	if(harts_var->defined())
	{
		harts = harts_var->val();
		if(harts > 64)
		{
			std::cerr << "RISC-V CPU cannot have more than 64 cores at 1 time." << std::endl;

			return -1;
		}
	}

	atexit(cleanup_terminal);
	termios newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ISIG | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	Machine machine = Machine(memsize, harts);
	machine.init_mmap();

	// machine.mmap->load_file(0x80000000, bios_var->val());
	machine.bios_file = fopen(bios_var->val().c_str(), "rb");

	if(kernel_var->defined())
	{
		// machine.mmap->load_file(0x80200000, kernel_var->val());
		machine.kernel_file = fopen(kernel_var->val().c_str(), "rb");
	}
	if(image_var->defined())
	{
		machine.image_file = fopen(image_var->val().c_str(), "r+b");
	}

	if(dtb_var->defined())
	{
		machine.dtb_file = fopen(dtb_var->val().c_str(), "rb");
	}
	else
	{
		if(dumpdtb_var->defined()) machine.dtb_dump_path = dumpdtb_var->val();
		if(append_var->defined()) machine.append = append_var->val();
		machine.init_fdt();
	}
#ifdef USE_FRAMEBUFFER
	uint64_t fb_w = 0;
	uint64_t fb_h = 0;
	if(fb_var->defined())
	{
		std::string text = fb_var->val();
		if(text.empty())
		{
			fb_w = 640;
			fb_h = 480;
		}
		else
		{
			size_t pos = text.find('x');

			if(pos != std::string::npos)
			{
				fb_w = stoull(text.substr(0, pos));

				fb_h = stoull(text.substr(pos + 1));
			}
			else
			{
				std::cerr << "Wrong FB resolution! Example: 1280x720" << std::endl;
				return -1;
			}
		}
	}
#endif

	mach = &machine;

	termios_running = true;
	std::thread term(&termios_loop);

#ifdef USE_GDBSTUB
	std::thread gdbstub;
	if(gdb_var->defined())
	{
		machine.gdb = true;
		gdbstub		= std::thread(GDB_Create, &machine.harts[0], &machine);
	}
#endif

	machine.init_auto_devices();
	uart = machine.mmio->get<UART>();

	auto i2c = machine.mmio->get<I2C>();

#ifdef USE_FRAMEBUFFER
	AppWindow window;
	VkInstance instance;
	VkSurfaceKHR surface;
	if(fb_w != 0 && fb_h != 0)
	{
		auto kb		  = i2c->create_device<HID_Keyboard>(machine, machine.fdt);
		window.kb	  = std::dynamic_pointer_cast<HID_Keyboard>(kb);
		window.width  = fb_w;
		window.height = fb_h;
		if(!InitializeNativeWindow(window, "rv64-vm"))
		{
			return -1;
		}
		instance = CreateVulkanInstance();
		surface	 = VK_NULL_HANDLE;
		if(!CreateVulkanSurface(instance, window, surface))
		{
			return -1;
		}
		StartEventLoop(window);

		machine.mmio->create_device<Framebuffer>(0x18000000, machine, machine.fdt, fb_w, fb_h, window);
	}
#endif

	machine.write_fdt();
	machine.run();
	if(machine.work_thread_w)
	{
		machine.work_thread_joined = true;
		machine.work_thread.join(); // joining it so our program will not exit right after creating machine
	}

	termios_running.store(false, std::memory_order_seq_cst);
	term.join();
#ifdef USE_GDBSTUB
	if(gdb_var->defined())
	{
		GDB_Stop();
		gdbstub.join();
	}
#endif

#ifdef USE_FRAMEBUFFER
	if(fb_w != 0 && fb_h != 0)
	{
		if(surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(instance, surface, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
		TerminateNativeWindow(window);
	}
#endif

	return 0;
}
