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
#include <cstdint>

namespace rv64vm::dev
{
	struct I2CSlave
	{
		I2CSlave(uint8_t address, uint16_t buffer_size) : address(address), buffer_size(buffer_size), buffer_ind(0)
		{
			buffer = new uint8_t[buffer_size];
		};
		~I2CSlave()
		{
			if(buffer)
			{
				delete[] buffer;
			}
		};
		uint8_t address;
		uint16_t buffer_size;
		uint16_t buffer_ind;
		uint8_t* buffer;
		bool is_transmitting = false;

		virtual void stop_transmit()
		{
			buffer_ind		= 0;
			is_transmitting = false;
		};
		virtual uint8_t dev_read(bool m_ack)
		{
			uint8_t val = buffer[buffer_ind];
			if(buffer_ind != (buffer_size - 1) || !m_ack) buffer_ind++;
			return val;
		}
		virtual void dev_write(uint8_t val)
		{
			buffer[buffer_ind] = val;
			if(buffer_ind != (buffer_size)-1) buffer_ind++;
		}
		virtual void start_transmit()
		{
			buffer_ind		= 0;
			is_transmitting = true;
		}
	};
}
