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

#include <unistd.h>
#ifdef USE_GDBSTUB

#include "../include/defines/csr.hpp"
#include "../include/machine.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <fcntl.h>
#include <format>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <vector>

static std::string to_little_endian_hex(uint64_t value)
{
	std::ostringstream oss;
	for(int i = 0; i < 8; i++)
	{
		uint8_t byte = (value >> (i * 8)) & 0xFF;
		oss << std::format("{:02x}", byte);
	}
	return oss.str();
}
static std::string swapHexEndian(std::string value, int size_bytes)
{
	std::ostringstream oss;
	for(int i = size_bytes; i >= 0; i--)
	{
		oss << value.substr(i * 2, 2);
	}
	return oss.str();
}

// The giant lookup table is localized to the cpp implementation space
static const std::vector<std::tuple<std::string, uint32_t, char, std::optional<std::vector<std::tuple<std::string, uint8_t, uint8_t>>>>>
	xml_data{
		{ "zero", 0, 'g', std::nullopt },
		{ "ra", 1, 'g', std::nullopt },
		{ "sp", 2, 'g', std::nullopt },
		{ "gp", 3, 'g', std::nullopt },
		{ "tp", 4, 'g', std::nullopt },
		{ "t0", 5, 'g', std::nullopt },
		{ "t1", 6, 'g', std::nullopt },
		{ "t2", 7, 'g', std::nullopt },
		{ "fp", 8, 'g', std::nullopt },
		{ "s1", 9, 'g', std::nullopt },
		{ "a0", 10, 'g', std::nullopt },
		{ "a1", 11, 'g', std::nullopt },
		{ "a2", 12, 'g', std::nullopt },
		{ "a3", 13, 'g', std::nullopt },
		{ "a4", 14, 'g', std::nullopt },
		{ "a5", 15, 'g', std::nullopt },
		{ "a6", 16, 'g', std::nullopt },
		{ "a7", 17, 'g', std::nullopt },
		{ "s2", 18, 'g', std::nullopt },
		{ "s3", 19, 'g', std::nullopt },
		{ "s4", 20, 'g', std::nullopt },
		{ "s5", 21, 'g', std::nullopt },
		{ "s6", 22, 'g', std::nullopt },
		{ "s7", 23, 'g', std::nullopt },
		{ "s8", 24, 'g', std::nullopt },
		{ "s9", 25, 'g', std::nullopt },
		{ "s10", 26, 'g', std::nullopt },
		{ "s11", 27, 'g', std::nullopt },
		{ "t3", 28, 'g', std::nullopt },
		{ "t4", 29, 'g', std::nullopt },
		{ "t5", 30, 'g', std::nullopt },
		{ "t6", 31, 'g', std::nullopt },
		{ "pc", 32, 'g', std::nullopt },

#ifdef USE_FPU
		{ "f0", 33, 'f', std::nullopt },
		{ "f1", 34, 'f', std::nullopt },
		{ "f2", 35, 'f', std::nullopt },
		{ "f3", 36, 'f', std::nullopt },
		{ "f4", 37, 'f', std::nullopt },
		{ "f5", 38, 'f', std::nullopt },
		{ "f6", 39, 'f', std::nullopt },
		{ "f7", 40, 'f', std::nullopt },
		{ "f8", 41, 'f', std::nullopt },
		{ "f9", 42, 'f', std::nullopt },
		{ "f10", 43, 'f', std::nullopt },
		{ "f11", 44, 'f', std::nullopt },
		{ "f12", 45, 'f', std::nullopt },
		{ "f13", 46, 'f', std::nullopt },
		{ "f14", 47, 'f', std::nullopt },
		{ "f15", 48, 'f', std::nullopt },
		{ "f16", 49, 'f', std::nullopt },
		{ "f17", 50, 'f', std::nullopt },
		{ "f18", 51, 'f', std::nullopt },
		{ "f19", 52, 'f', std::nullopt },
		{ "f20", 53, 'f', std::nullopt },
		{ "f21", 54, 'f', std::nullopt },
		{ "f22", 55, 'f', std::nullopt },
		{ "f23", 56, 'f', std::nullopt },
		{ "f24", 57, 'f', std::nullopt },
		{ "f25", 58, 'f', std::nullopt },
		{ "f26", 59, 'f', std::nullopt },
		{ "f27", 60, 'f', std::nullopt },
		{ "f28", 61, 'f', std::nullopt },
		{ "f29", 62, 'f', std::nullopt },
		{ "f30", 63, 'f', std::nullopt },
		{ "f31", 64, 'f', std::nullopt },
#endif

		{ "mstatus", CSR_MSTATUS, 'c',
		  std::vector<std::tuple<std::string, uint8_t, uint8_t>>{
			  { "SIE", 1, 1 },
			  { "MIE", 3, 3 },
			  { "SPIE", 5, 5 },
			  { "UBE", 6, 6 },
			  { "MPIE", 7, 7 },
			  { "SPP", 8, 8 },
			  { "VS", 9, 10 },
			  { "MPP", 11, 12 },
			  { "FS", 13, 14 },
			  { "XS", 15, 16 },
			  { "MPRV", 17, 17 },
			  { "SUM", MSTATUS_SUM, MSTATUS_SUM },
			  { "MXR", MSTATUS_MXR, MSTATUS_MXR },
			  { "TVM", 20, 20 },
			  { "TW", 21, 21 },
			  { "TSR", 22, 22 },
		  } },
		{ "cycle", CSR_CYCLE, 'c', std::nullopt },
		{ "time", CSR_TIME, 'c', std::nullopt },
		{ "instret", CSR_INSTRET, 'c', std::nullopt },
		{ "misa", CSR_MISA, 'c', std::nullopt },
		{ "medeleg", CSR_MEDELEG, 'c', std::nullopt },
		{ "mideleg", CSR_MIDELEG, 'c', std::nullopt },
		{ "mie", CSR_MIE, 'c', std::nullopt },
		{ "mip", CSR_MIP, 'c', std::nullopt },
		{ "mtvec", CSR_MTVEC, 'c', std::nullopt },
		{ "mcounteren", CSR_MCOUNTEREN, 'c', std::nullopt },
		{ "mscratch", CSR_MSCRATCH, 'c', std::nullopt },
		{ "mhartid", CSR_MHARTID, 'c', std::nullopt },
		{ "mepc", CSR_MEPC, 'c', std::nullopt },
		{ "mcause", CSR_MCAUSE, 'c', std::nullopt },
		{ "mtval", CSR_MTVAL, 'c', std::nullopt },
		{ "sstatus", CSR_SSTATUS, 'c',
		  std::vector<std::tuple<std::string, uint8_t, uint8_t>>{
			  { "SIE", 1, 1 },
			  { "SPIE", 5, 5 },
			  { "UBE", 6, 6 },
			  { "SPP", 8, 8 },
			  { "VS", 9, 10 },
			  { "FS", 13, 14 },
			  { "XS", 15, 16 },
		  } },
		{ "sedeleg", CSR_SEDELEG, 'c', std::nullopt },
		{ "sideleg", CSR_SIDELEG, 'c', std::nullopt },
		{ "sie", CSR_SIE, 'c', std::nullopt },
		{ "sip", CSR_SIP, 'c', std::nullopt },
		{ "stvec", CSR_STVEC, 'c', std::nullopt },
		{ "scounteren", CSR_SCOUNTEREN, 'c', std::nullopt },
		{ "sscratch", CSR_SSCRATCH, 'c', std::nullopt },
		{ "sepc", CSR_SEPC, 'c', std::nullopt },
		{ "scause", CSR_SCAUSE, 'c', std::nullopt },
		{ "stval", CSR_STVAL, 'c', std::nullopt },
		{ "stimecmp", CSR_STIMECMP, 'c', std::nullopt },
/*{ "satp", CSR_SATP, 'c',
  vector<tuple<string, uint8_t, uint8_t>>{
	  { "MODE", SATP_MODE_LOW, SATP_MODE_HIGH },
	  { "ASID", SATP_ASID_LOW, SATP_ASID_HIGH },
	  { "PPN", SATP_PPN_LOW, SATP_PPN_HIGH },
  } },*/
// mmu
#ifdef USE_FPU
		{ "fcsr", CSR_FCSR + 5000, 'c',
		  std::vector<std::tuple<std::string, uint8_t, uint8_t>>{
			  { "FRM", 5, 7 },
			  { "NX", 0, 0 },
			  { "UF", 1, 1 },
			  { "OF", 2, 2 },
			  { "DZ", 3, 3 },
			  { "NV", 4, 4 },
		  } },
#endif
		{ "priv", 65, 'v', std::nullopt },
};

std::string Machine::GDBStub::create_xml()
{
	std::ostringstream output;
	output << R"(<?xml version="1.0"?>
    <!DOCTYPE target SYSTEM "gdb-target.dtd">
    <target version="1.0">
    <architecture>riscv:rv64</architecture>
    <feature name="org.gnu.gdb.riscv.cpu">)"
		   << std::endl;

	for(size_t i = 0; i < xml_data.size(); i++)
	{
		const auto& cur_reg = xml_data.at(i);
		if(std::get<2>(cur_reg) == 'g')
		{
			output << std::format(R"(    <reg name="{}" bitsize="{}" regnum="{}")",
								  std::get<0>(cur_reg), 64, std::get<1>(cur_reg));
			if(std::get<3>(cur_reg).has_value())
			{
				output << ">" << std::endl;
				for(const auto& dat : *std::get<3>(cur_reg))
				{
					output << std::format(R"(      <field name="{}" start="{}" end="{}"/>)",
										  std::get<0>(dat), std::get<1>(dat), std::get<2>(dat))
						   << std::endl;
				}
				output << "    </reg>" << std::endl;
			}
			else
			{
				output << "/>" << std::endl;
			}
		}
	}
#ifdef USE_FPU
	output << "  </feature>" << std::endl;
	output << R"(  <feature name="org.gnu.gdb.riscv.fpu">)" << std::endl;
	output << R"(    <union id="riscv_double">)" << std::endl;
	output << R"(      <field name="float" type="ieee_single"/>)" << std::endl;
	output << R"(      <field name="double" type="ieee_double"/>)" << std::endl;
	output << R"(    </union>)" << std::endl;
	for(size_t i = 0; i < xml_data.size(); i++)
	{
		const auto& cur_reg = xml_data.at(i);
		if(std::get<2>(cur_reg) == 'f')
		{
			output << std::format(R"(    <reg name="{}" bitsize="{}" type="riscv_double"/>)",
								  std::get<0>(cur_reg), 64)
				   << std::endl;
		}
	}
#endif
	output << "  </feature>" << std::endl;
	output << R"(  <feature name="org.gnu.gdb.riscv.virtual">)" << std::endl;
	for(size_t i = 0; i < xml_data.size(); i++)
	{
		const auto& cur_reg = xml_data.at(i);
		if(std::get<2>(cur_reg) == 'v')
		{
			output << std::format(R"(    <reg name="{}" bitsize="{}")", std::get<0>(cur_reg), 64);
			if(std::get<3>(cur_reg).has_value())
			{
				output << ">" << std::endl;
				for(const auto& dat : *std::get<3>(cur_reg))
				{
					output << std::format(R"(      <field name="{}" start="{}" end="{}"/>)",
										  std::get<0>(dat), std::get<1>(dat), std::get<2>(dat))
						   << std::endl;
				}
				output << "    </reg>" << std::endl;
			}
			else
			{
				output << "/>" << std::endl;
			}
		}
	}
	output << "  </feature>" << std::endl;
	output << R"(  <feature name="org.gnu.gdb.riscv.csr">)" << std::endl;
	for(size_t i = 0; i < xml_data.size(); i++)
	{
		const auto& cur_reg = xml_data.at(i);
		if(std::get<2>(cur_reg) == 'c')
		{
			output << std::format(R"(    <reg name="{}" bitsize="{}" regnum="{}")",
								  std::get<0>(cur_reg), 64, std::get<1>(cur_reg));
			if(std::get<3>(cur_reg).has_value())
			{
				output << ">" << std::endl;
				for(const auto& dat : *std::get<3>(cur_reg))
				{
					output << std::format(R"(      <field name="{}" start="{}" end="{}"/>)",
										  std::get<0>(dat), std::get<1>(dat), std::get<2>(dat))
						   << std::endl;
				}
				output << "    </reg>" << std::endl;
			}
			else
			{
				output << "/>" << std::endl;
			}
		}
	}
	output << "  </feature>" << std::endl;
	output << "</target>" << std::endl;
	return output.str();
}

void Machine::GDBStub::start(uint16_t port)
{
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	int op	  = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op));

	// Automatically target the first CPU core (hart 0) as default debugger context
	if(!machine_ctx->harts.empty())
	{
		active_hart = &machine_ctx->harts[0];
	}

	sockaddr_in serverAddress{};
	serverAddress.sin_family	  = AF_INET;
	serverAddress.sin_port		  = htons(port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int t = bind(server_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if(t < 0)
	{
		perror("[GDB] Bind failed! ");
		close(server_fd);
		server_fd = 0;
		return;
	}

	std::cout << "[GDB] Stub created on port " << port << std::endl;

	xml_target_description = create_xml();

	is_executing = true;

	worker_thread = std::thread(&Machine::GDBStub::execution_loop, this);
}

void Machine::GDBStub::stop()
{
	if(is_executing)
	{
		is_executing = false;
		std::cout << "[GDB] Disconnecting client" << std::endl;

		if(server_fd)
		{
			shutdown(server_fd, SHUT_RDWR);
			close(server_fd);
			server_fd = 0;
		}
		if(client_fd)
		{
			shutdown(client_fd, SHUT_RDWR);
			close(client_fd);
			client_fd = 0;
		}

		if(worker_thread.joinable())
		{
			worker_thread.join();
		}
	}
}

uint32_t Machine::GDBStub::send_raw(const std::string& buffer, uint32_t flags)
{
	size_t total = 0;
	while(total < buffer.size())
	{
		ssize_t sent = send(client_fd, buffer.c_str() + total, buffer.size() - total, flags);
		if(sent <= 0)
			return 0;
		total += sent;
	}
	return static_cast<uint32_t>(total);
}

uint32_t Machine::GDBStub::send_packet(std::string buffer, uint32_t flags)
{
	uint8_t sum = 0;
	for(char& ch : buffer)
	{
		sum += ch;
	}

	buffer = std::format("${}#{:02x}", buffer, sum);

	return send_raw(buffer, flags);
}

std::string Machine::GDBStub::unformat_packet(const std::string& buffer)
{
	if(buffer.size() < 4) return "";
	return buffer.substr(1, buffer.size() - 4);
}

void Machine::GDBStub::execution_loop()
{
	while(is_executing)
	{
		listen(server_fd, 5);

		client_fd = accept(server_fd, nullptr, nullptr);

		// if machine stopped before someone ever connected
		if(!is_executing)
		{
			if(client_fd)
			{
				close(client_fd);
				client_fd = 0;
			}
			return;
		}

		int flag = 1;
		setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
		std::cout << "[GDB] Got connection" << std::endl;

		char buffer[4096];
		while(is_executing)
		{
			std::memset(&buffer, 0, sizeof(buffer));
			ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);
			if(received <= 0)
			{
				break;
			}

			if(buffer[0] == '$')
			{
				send_raw("+", 0);
				parse_packet(buffer);
			}
			// Ctrl+C handle
			else if(buffer[0] == 3)
			{
				send_raw("+", 0);
				received_sigint.store(true, std::memory_order_release);
				machine_ctx->state.store(MachineState::Halted, std::memory_order_release);
			}
			// Ack check
			else if(buffer[0] == '-' || buffer[0] == '+')
			{
				if(buffer[1] == '$')
				{
					std::string str(buffer);
					str.erase(0, 1);
					send_raw("+", 0);
					parse_packet(str);
				}
			}
		}

		// close client FD before next connection
		if(client_fd)
		{
			close(client_fd);
			client_fd = 0;
		}
	}
}

void Machine::GDBStub::parse_packet(const std::string& buffer)
{
	if(buffer.empty() || buffer[0] != '$') return;

	std::string packet = unformat_packet(buffer);
	if(packet.empty()) return;

	// Get guest features
	if(packet.starts_with("qSupported:"))
	{
		send_packet("PacketSize=4096;qXfer:features:read+;vContSupported+;swbreak+;hwbreak+;");
		return;
	}

	// Send target xml
	if(packet.starts_with("qXfer:features:read:target.xml:"))
	{
		std::string dat = packet.substr(31);
		size_t comma	= dat.find(',');
		if(comma == std::string::npos) return;

		uint64_t startp = std::stoul(dat.substr(0, comma), nullptr, 16);
		uint64_t endp	= std::stoul(dat.substr(comma + 1), nullptr, 16);

		std::string response_prefix = ((startp + endp) >= xml_target_description.size()) ? "l" : "m";
		send_packet(response_prefix + xml_target_description.substr(startp, endp));
		return;
	}

	// Get all vCont features
	if(packet.starts_with("vCont?"))
	{
		send_packet("vCont;s;S;c;C");
		return;
	}

	if(packet.starts_with("vCont;"))
	{
		std::string args = packet.substr(6);
		if(args.empty()) return;

		switch(args[0])
		{
			case 'c': // Continue
				received_sigint.store(false, std::memory_order_release);
				machine_ctx->gdb_single_step = false;
				machine_ctx->state.store(MachineState::Running, std::memory_order_release);
				break;

			case 's': // Single step
				machine_ctx->gdb_single_step = true;
				machine_ctx->state.store(MachineState::Running, std::memory_order_release);
				send_packet("S05");
				break;

			default:
				send_packet("");
				break;
		}
		return;
	}

	// Kill
	if(packet.starts_with("k"))
	{
		is_executing = false;
		std::cout << "[GDB] Killed by GDB stub" << std::endl;
		machine_ctx->state.store(MachineState::Off, std::memory_order_release);
		return;
	}

	// Continue
	if(packet.starts_with("c"))
	{
		received_sigint.store(false, std::memory_order_release);
		machine_ctx->gdb_single_step = false;
		machine_ctx->state.store(MachineState::Running, std::memory_order_release);
		return;
	}

	// Single step
	if(packet.starts_with("s"))
	{
		machine_ctx->gdb_single_step = true;
		machine_ctx->state.store(MachineState::Running, std::memory_order_release);
		send_packet("S05");
		return;
	}

	// Get trap reason
	if(packet.starts_with("?"))
	{
		if(active_hart)
		{
			send_packet(std::format("S{:02d}", active_hart->csrs[CSR_MCAUSE]));
		}
		else
		{
			send_packet("S05");
		}
		return;
	}

	if(packet.starts_with("qfThreadInfo"))
	{
		send_packet("m1");
		return;
	}
	if(packet.starts_with("Hc"))
	{
		send_packet("OK");
		return;
	}
	if(packet.starts_with("qC"))
	{
		send_packet(std::format("QC{:x}", 1));
		return;
	}
	if(packet.starts_with("qAttached"))
	{
		send_packet("0");
		return;
	}
	if(packet.starts_with("Hg"))
	{
		send_packet("OK");
		return;
	} // Thread switch stub

	// Read single reg
	if(packet.starts_with("p"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		uint64_t idx = std::stoul(packet.substr(1), nullptr, 16);

		if(idx <= 31)
		{
			send_packet(to_little_endian_hex(active_hart->GPR[idx]));
		}
		else if(idx == 32)
		{
			send_packet(to_little_endian_hex(active_hart->pc));
		}
		else if(idx == 33)
		{
			send_packet(to_little_endian_hex(static_cast<uint8_t>(active_hart->mode)));
		}
		else if(idx > 33 && idx < 64)
		{
#ifdef USE_FPU
			send_packet(to_little_endian_hex(std::bit_cast<uint64_t>(active_hart->FPR[idx - 34])));
#else
			send_packet(to_little_endian_hex(0));
#endif
		}
		else if(idx >= 64)
		{
			uint64_t csr_idx = (idx >= 5000) ? (idx - 5000) : idx;
			send_packet(to_little_endian_hex(active_hart->csr_read(csr_idx)));
		}
		return;
	}

	// Write single reg
	if(packet.starts_with("P"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		uint64_t idx  = std::stoul(packet.substr(1), nullptr, 16);
		size_t eq_pos = packet.find('=');
		if(eq_pos == std::string::npos)
		{
			send_packet("E01");
			return;
		}

		std::string data = packet.substr(eq_pos + 1);
		uint64_t num	 = 0;
		for(uint64_t i = 0; i < 8 && (i * 2 + 1) < data.size(); i++)
		{
			num |= (std::stoul(data.substr(i * 2, 2), nullptr, 16) << (i * 8));
		}

		if(idx <= 31)
		{
			active_hart->GPR[idx] = num;
		}
		else if(idx == 32)
		{
			active_hart->pc = num;
		}
		else if(idx == 33)
		{
			active_hart->mode = static_cast<PrivilegeMode>(num);
		}
		else if(idx > 33 && idx < 64)
		{
#ifdef USE_FPU
			active_hart->FPR[idx - 34] = std::bit_cast<double>(num);
#endif
		}
		else if(idx >= 64)
		{
			active_hart->csr_write(idx, num);
		}
		send_packet("OK");
		return;
	}

	// Read all GPR + PC
	if(packet.starts_with("g"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		std::string reg_packet;
		for(int i = 0; i < 32; i++)
		{
			reg_packet += to_little_endian_hex(active_hart->GPR[i]);
		}
		reg_packet += to_little_endian_hex(active_hart->pc);
		send_packet(reg_packet);
		return;
	}

	// Write entire GPR
	if(packet.starts_with("G"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		std::string data = packet.substr(1);
		if(data.size() < (33 * 16))
		{
			send_packet("E01");
			return;
		}

		for(uint64_t i = 0; i < 32; i++)
		{
			active_hart->GPR[i] = std::stoull(swapHexEndian(data.substr(i * 16, 16), 8), nullptr, 16);
		}
		active_hart->pc = std::stoull(swapHexEndian(data.substr(32 * 16, 16), 8), nullptr, 16);
		send_packet("OK");
		return;
	}

	// Guest memory store
	if(packet.starts_with("M"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		size_t comma = packet.find(',');
		size_t colon = packet.find(':', comma);
		if(comma == std::string::npos || colon == std::string::npos)
		{
			send_packet("E01");
			return;
		}

		uint64_t address = std::stoul(packet.substr(1, comma), nullptr, 16);
		uint64_t size	 = std::stoul(packet.substr(comma + 1, colon - comma - 1), nullptr, 16);
		std::string data = packet.substr(colon + 1);

		for(uint64_t i = 0; i < size; i++)
		{
			uint8_t byte_val = static_cast<uint8_t>(std::stoul(data.substr(i * 2, 2), nullptr, 16));
			MemoryReturn out = active_hart->mmio->write(*active_hart, address + i, MemorySize::Byte, byte_val);
			if(!out.is_success)
			{
				send_packet("E01");
				return;
			}
		}
		send_packet(std::format("{:x}", size));
		return;
	}

	// Guest memory read
	if(packet.starts_with("m"))
	{
		if(!active_hart)
		{
			send_packet("E01");
			return;
		}
		size_t comma = packet.find(',');
		if(comma == std::string::npos)
		{
			send_packet("E01");
			return;
		}

		uint64_t address = std::stoul(packet.substr(1, comma), nullptr, 16);
		uint64_t size	 = std::stoul(packet.substr(comma + 1), nullptr, 16);
		std::string resp;

		for(uint64_t i = 0; i < size; i++)
		{
			uint8_t val		 = 0;
			MemoryReturn out = active_hart->mmio->read(*active_hart, address + i, MemorySize::Byte, &val);
			if(!out.is_success)
			{
				send_packet("E01");
				return;
			}
			resp += std::format("{:02x}", val);
		}
		send_packet(resp);
		return;
	}

	// Add hw breakpoint
	if(packet.starts_with("Z0"))
	{
		size_t first_comma = packet.find(',');
		if(first_comma == std::string::npos)
		{
			send_packet("E01");
			return;
		}
		std::string dat = packet.substr(first_comma + 1);

		uint64_t addr = std::stoul(dat.substr(0, dat.find(',')), nullptr, 16);
		if(std::find(breakpoints.begin(), breakpoints.end(), addr) != breakpoints.end())
		{
			send_packet("E01");
			return;
		}
		breakpoints.push_back(addr);
		send_packet("OK");
		return;
	}

	// Remove hw breakpoint
	if(packet.starts_with("z0"))
	{
		size_t first_comma = packet.find(',');
		if(first_comma == std::string::npos)
		{
			send_packet("E01");
			return;
		}
		std::string dat = packet.substr(first_comma + 1);

		uint64_t addr = std::stoul(dat.substr(0, dat.find(',')), nullptr, 16);
		auto it		  = std::find(breakpoints.begin(), breakpoints.end(), addr);
		if(it == breakpoints.end())
		{
			send_packet("E01");
			return;
		}
		breakpoints.erase(it);
		send_packet("OK");
		return;
	}

	// unsupported
	send_packet("");
}

// Public wrapper so the user can interact with it cleanly
void Machine::handle_gdb_breakpoints()
{
	if(!gdb_server.is_executing) return;

	if(!harts.empty())
	{
		uint64_t current_pc = harts[0].pc;
		auto& bp_vec		= gdb_server.breakpoints;

		if(std::find(bp_vec.begin(), bp_vec.end(), current_pc) != bp_vec.end())
		{
			state.store(MachineState::Halted, std::memory_order_release);
			gdb_server.send_packet(gdb_server.received_sigint.load() ? "S02" : "S05");
			gdb_server.received_sigint.store(false, std::memory_order_release);
		}
	}
}

void Machine::listen_gdb(uint16_t port)
{
	if(!gdb) return;
	gdb_server.start(port);
}

#endif
