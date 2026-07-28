/* Copyright 2026 Spalishe

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
#define RVJIT_MIN_INSTRUCTIONS 2
#define RVJIT_MAX_INSTRUCTIONS 128
#define RVJIT_FUNC_SIZE		   0x1000 // DONT CHANGE IT IF YOU DONT KNOW WHAT YOU'RE DOING! If emitted function will overflow arena's buffer, it will be your fault
#define RVJIT_ARENA_PAGES	   0x400  // Linux default page size is 4096, then 1024 * 4096 = 4194304 bytes, 4 MB
