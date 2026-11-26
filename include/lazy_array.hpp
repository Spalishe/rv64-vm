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
#include <cstddef>
#include <new>
#include <sys/mman.h>

namespace rv64vm::runner
{
	// Demand-zero sparse tables; avoids a large startup memset.
	template<class T>
	inline T* map_zero_array(size_t count)
	{
		void* p = mmap(nullptr, count * sizeof(T), PROT_READ | PROT_WRITE,
					   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if(p == MAP_FAILED)
			throw std::bad_alloc();
		return static_cast<T*>(p);
	}

	template<class T>
	inline void unmap_array(T* p, size_t count)
	{
		if(p != nullptr)
			munmap(p, count * sizeof(T));
	}

	template<class T, size_t N>
	class LazyArray
	{
	  public:
		LazyArray()
			: data_(map_zero_array<T>(N))
		{
		}
		~LazyArray()
		{
			unmap_array(data_, N);
		}
		LazyArray(const LazyArray&)			 = delete;
		LazyArray& operator=(const LazyArray&) = delete;
		LazyArray(LazyArray&& other) noexcept
			: data_(other.data_)
		{
			other.data_ = nullptr;
		}
		LazyArray& operator=(LazyArray&& other) noexcept
		{
			if(this != &other)
			{
				unmap_array(data_, N);
				data_		= other.data_;
				other.data_ = nullptr;
			}
			return *this;
		}

		T* data() const { return data_; }
		constexpr size_t size() const { return N; }
		T& operator[](size_t i) { return data_[i]; }
		const T& operator[](size_t i) const { return data_[i]; }

	  private:
		T* data_;
	};
}
