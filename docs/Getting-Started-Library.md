## Using a library

`rv64-vm` can be compiled as a shared or static library to embed a 64-bit RISC-V virtual machine inside your own C++ applications.

### Quick Start Example

```cpp
// Include main RV64-VM Header
#include "rv64-vm.hpp"

int main(int argc, char* argv[]) {
	// Create Machine config, which will define all work parameters	
	rv64vm::runner::MachineConfig cfg = rv64vm::runner::MachineConfig();
	cfg.append						  = "root=/dev/vda"; // Keep empty string for no value
	cfg.dtb_dump_path				  = "/tmp/dtb_path"; // ^
	cfg.hart_count					  = 1;
	cfg.MMUMode						  = rv64vm::runner::MMU::SatpMode::Sv39;
	cfg.memory_size					  = 512*1024*1024; // 512 Megabytes. 

	// Create machine object
	rv64vm::runner::Machine machine = rv64vm::runner::Machine(cfg);

	const char* bios_path = "/tmp/fw_jump.elf"
	const char* kernel_path = "/tmp/Image"
	const char* image_path = "/tmp/rootfs.ext2"

	// Load Firmware into memory at 0x80000000
	machine.load_bios(bios_path);

	// Load Kernel into memory at 0x80200000
	machine.load_kernel(kernel_path);
	
	// Load Image into VirtIO-BLK
	machine.load_image(image_path);

	// FDT part
	// You can define your own compiled DTB.
	// machine.load_dtb("/tmp/test_dtb.dtb");

	// Starts Device initialization state
	machine.start_init();

	// After this function you want to create devices you want (such as custom ones)
	// Example:
	
	auto i2c = machine.get_mmio()->get<rv64vm::dev::I2C>();
	auto kb  = i2c->create_device<rv64vm::dev::HID_Keyboard>(machine, machine.get_fdt());
	auto fb  = machine.get_mmio()->create_device<rv64vm::dev::Framebuffer>(0x18000000, machine, machine.get_fdt(), fb_w, fb_h, window);
	
	// End device init state
	machine.end_init();
	// After call no new device will be added to FDT, because its already compiled

	// Run the machine!
	machine.run();

	// You can also join machine work thread to main one
	machine.wait();
}
```

### API

Consider looking to [API Reference](API-Reference.md)
