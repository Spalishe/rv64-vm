#pragma once

// All forward decs
namespace rv64vm::runner
{
	class Machine;
	class Hart;
}

namespace rv64vm::dev
{
	class Device;
	class CLINT;
	class SYSCON;
}

namespace rv64vm::jit
{
	struct JIT_Block;
	struct JIT_Emitter;
}
