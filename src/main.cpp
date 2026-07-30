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

#include "../include/rv64-vm.hpp"

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
std::shared_ptr<rv64vm::dev::UART> uart;

rv64vm::runner::Machine* mach;
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

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <random>

// Helper function to test if a specific port can be bound
bool try_bind_port(int sock, int port)
{
	sockaddr_in addr;
	addr.sin_family		 = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port		 = htons(port);

	// bind() returns 0 on success
	return (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0);
}

int get_random_port()
{
	// create the socket
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0) return -1;

	// try the primary choice (1512) first
	if(try_bind_port(sock, 1512))
	{
		close(sock); // Close here if you just wanted the number
		return 1512;
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(1513, 65535);

	// Limit attempts to avoid an infinite loop if the network stack is broken
	for(int attempts = 0; attempts < 100; ++attempts)
	{
		int random_port = distr(gen);

		if(try_bind_port(sock, random_port))
		{
			close(sock);
			return random_port;
		}
	}

	close(sock);
	return -1; // Failed to find any available port
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

	rv64vm::runner::MachineConfig cfg = rv64vm::runner::MachineConfig();
	cfg.append						  = append_var->val();
	cfg.dtb_dump_path				  = dumpdtb_var->val();
	cfg.hart_count					  = harts;
	cfg.memory_size					  = memsize;

	rv64vm::runner::Machine machine = rv64vm::runner::Machine(cfg);

	machine.load_bios(bios_var->val().c_str());

	if(kernel_var->defined())
		machine.load_kernel(kernel_var->val().c_str());
	if(image_var->defined())
		machine.load_image(image_var->val().c_str());

	if(dtb_var->defined())
		machine.load_dtb(dtb_var->val().c_str());

	else
	{
		if(dumpdtb_var->defined()) cfg.dtb_dump_path = dumpdtb_var->val();
		if(append_var->defined()) cfg.append = append_var->val();
	}
	machine.start_init();

	// Place for non SoC
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
		machine.enable_gdb(true, get_random_port());
	}
#endif

	uart = machine.get_mmio()->get<rv64vm::dev::UART>();

#ifdef USE_FRAMEBUFFER
	AppWindow window;
	VkInstance instance;
	VkSurfaceKHR surface;
	if(fb_w != 0 && fb_h != 0)
	{
		auto i2c	  = machine.get_mmio()->get<rv64vm::dev::I2C>();
		auto kb		  = i2c->create_device<rv64vm::dev::HID_Keyboard>(machine, machine.get_fdt());
		window.kb	  = std::dynamic_pointer_cast<rv64vm::dev::HID_Keyboard>(kb);
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

		machine.get_mmio()->create_device<rv64vm::dev::Framebuffer>(0x18000000, machine, machine.get_fdt(), fb_w, fb_h, window);
	}
#endif

	machine.end_init();

	machine.run();
	machine.wait();

	termios_running.store(false, std::memory_order_seq_cst);
	if(term.joinable())
		term.join();

	// gdb stub will deconstruct automatically

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
