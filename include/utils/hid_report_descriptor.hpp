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
#include <tuple>
#include <vector>

enum HIDType : uint8_t
{
	MAIN   = 0x00,
	GLOBAL = 0x04,
	LOCAL  = 0x08
};

enum HIDTag : uint8_t
{
	// Main items
	INPUT_TAG		   = 0x80,
	OUTPUT_TAG		   = 0x90,
	COLLECTION_TAG	   = 0xA0,
	END_COLLECTION_TAG = 0xC0,

	// Global items
	USAGE_PAGE_TAG	 = 0x00,
	LOGICAL_MIN_TAG	 = 0x10,
	LOGICAL_MAX_TAG	 = 0x20,
	REPORT_SIZE_TAG	 = 0x70,
	REPORT_COUNT_TAG = 0x90,

	// Local items
	USAGE_TAG	  = 0x00,
	USAGE_MIN_TAG = 0x10,
	USAGE_MAX_TAG = 0x20
};

using HIDItem = std::tuple<HIDType, HIDTag, uint32_t>;

inline std::vector<uint8_t> generate_report_descriptor(const std::vector<HIDItem>& items)
{
	std::vector<uint8_t> descriptor;

	for(const auto& item : items)
	{
		HIDType type   = std::get<0>(item);
		HIDTag tag	   = std::get<1>(item);
		uint32_t value = std::get<2>(item);

		// END_COLLECTION does not have any data
		if(tag == END_COLLECTION_TAG && type == MAIN)
		{
			descriptor.push_back(0xC0);
			continue;
		}

		uint8_t bsize_bits = 0;
		int bytes_to_write = 0;

		if(value <= 0xFF)
		{
			bsize_bits	   = 0x01;
			bytes_to_write = 1;
		}
		else if(value <= 0xFFFF)
		{
			bsize_bits	   = 0x02;
			bytes_to_write = 2;
		}
		else
		{
			bsize_bits	   = 0x03;
			bytes_to_write = 4;
		}

		uint8_t prefix = (uint8_t)tag | (uint8_t)type | bsize_bits;
		descriptor.push_back(prefix);

		for(int i = 0; i < bytes_to_write; ++i)
		{
			descriptor.push_back((value >> (i * 8)) & 0xFF);
		}
	}

	return descriptor;
}
