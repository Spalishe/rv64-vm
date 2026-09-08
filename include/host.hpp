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

#undef HOST_TARGET_X86_64
#undef HOST_TARGET_AARCH64

#if defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)
#define HOST_TARGET_X86_64 1
#endif
#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
#define HOST_TARGET_AARCH64 1
#endif
