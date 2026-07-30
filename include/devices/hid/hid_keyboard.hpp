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
#include "../../utils/hid_report_descriptor.hpp"
#include "hid-over-i2c.hpp"
#include <thread>

struct HID_Keyboard : HIDOverI2C
{
  public:
	void update(uint8_t modifiers, uint8_t key_1, uint8_t key_2, uint8_t key_3, uint8_t key_4, uint8_t key_5, uint8_t key_6, bool rollover);
	HID_Keyboard(Machine& cpu, fdt_node* fdt);

  private:
	inline static const std::vector<HIDItem> report_descriptor_items = {
		{ GLOBAL, USAGE_PAGE_TAG,	  0x01 }, // Generic Desktop Page
		{ LOCAL,	 USAGE_TAG,			0x06 }, // Keyboard
		{ MAIN,	COLLECTION_TAG,		0x01 }, // Main collection

		{ GLOBAL, USAGE_PAGE_TAG,	  0x07 },
		{ LOCAL,	 USAGE_MIN_TAG,		0xE0 },
		{ LOCAL,	 USAGE_MAX_TAG,		0xE7 },
		{ GLOBAL, LOGICAL_MIN_TAG,	   0	 },
		{ GLOBAL, LOGICAL_MAX_TAG,	   1	 },
		{ GLOBAL, REPORT_SIZE_TAG,	   1	 },
		{ GLOBAL, REPORT_COUNT_TAG,	8	  },
		{ MAIN,	INPUT_TAG,		   0x02 },

		{ GLOBAL, REPORT_SIZE_TAG,	   8	 },
		{ GLOBAL, REPORT_COUNT_TAG,	1	  },
		{ MAIN,	INPUT_TAG,		   0x01 },

		{ GLOBAL, REPORT_SIZE_TAG,	   8	 },
		{ GLOBAL, REPORT_COUNT_TAG,	6	  },
		{ GLOBAL, LOGICAL_MIN_TAG,	   0	 },
		{ GLOBAL, LOGICAL_MAX_TAG,	   255  },
		{ LOCAL,	 USAGE_MIN_TAG,		0x00 },
		{ LOCAL,	 USAGE_MAX_TAG,		255	},
		{ MAIN,	INPUT_TAG,		   0x00 },

		{ MAIN,	END_COLLECTION_TAG, 0x0	},
	};

	void hid_event_output_report_write();
	void hid_event_data_register_write();
	void hid_event_command_register_write();
};
