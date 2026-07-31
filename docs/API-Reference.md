# rv64-vm API Reference
This document contains all Exported API functions.

## Namespace `rv64vm::runner`

* **[Machine](api/Machine.md)**
  * [Machine](api/Machine.md#machine)
  * [~Machine](api/Machine.md#machine)
  * [start_init](api/Machine.md#start_init)
  * [end_init](api/Machine.md#end_init)
  * [run](api/Machine.md#run)
  * [stop](api/Machine.md#stop)
  * [reset](api/Machine.md#reset)
  * [wait](api/Machine.md#wait)
  * [get_mmio](api/Machine.md#get_mmio)
  * [get_fdt](api/Machine.md#get_fdt)
  * [get_timebase](api/Machine.md#get_timebase)
  * [get_mmap](api/Machine.md#get_mmap)
  * [get_state](api/Machine.md#get_state)
  * [get_memory_size](api/Machine.md#get_memory_size)
  * [get_hart_count](api/Machine.md#get_hart_count)
  * [get_hart](api/Machine.md#get_hart)
  * [load_image](api/Machine.md#load_image)
  * [load_bios](api/Machine.md#load_bios)
  * [load_kernel](api/Machine.md#load_kernel)
  * [load_dtb](api/Machine.md#load_dtb)
  * [get_image](api/Machine.md#get_image)
  * [set_uart_output](api/Machine.md#set_uart_output)
  * [get_uart_output](api/Machine.md#get_uart_output)
* **[MemoryMap](api/MemoryMap.md)**
  * [MemoryMap](api/MemoryMap.md#memorymap)
  * [~MemoryMap](api/MemoryMap.md#memorymap)
  * [get_regions](api/MemoryMap.md#get_regions)
  * [get_ram_direct](api/MemoryMap.md#get_ram_direct)
  * [add_region](api/MemoryMap.md#add_region)
  * [load_file](api/MemoryMap.md#load_file)
  * [load_buffer](api/MemoryMap.md#load_buffer)
  * [load](api/MemoryMap.md#load)
  * [store](api/MemoryMap.md#store)
  * [find_region](api/MemoryMap.md#find_region)

## Namespace `rv64vm::runner::MemoryMap`

* **[MemoryRegion](api/MemoryRegion.md)**
  * [get_base_addr](api/MemoryRegion.md#get_base_addr)
  * [get_size](api/MemoryRegion.md#get_size)
  * [ptr](api/MemoryRegion.md#ptr)

