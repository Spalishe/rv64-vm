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
#include "../../../include/devices/hid/hid-over-usb.hpp"

namespace rv64vm::dev
{
	USBHIDDevice::USBHIDDevice(std::vector<uint8_t> report_descriptor, uint16_t vid, uint16_t pid)
		: report_desc(std::move(report_descriptor))
	{
		build_descriptors(vid, pid);
	}

	void USBHIDDevice::build_descriptors(uint16_t vid, uint16_t pid)
	{
		// 1. USB Device Descriptor (18 bytes)
		device_desc = {
			0x12,		// bLength
			0x01,		// bDescriptorType (Device)
			0x00, 0x02, // bcdUSB (USB 2.0)
			0x00,		// bDeviceClass (Defined at Interface level)
			0x00,		// bDeviceSubClass
			0x00,		// bDeviceProtocol
			64,			// bMaxPacketSize0 (EP0 max size)
			static_cast<uint8_t>(vid & 0xFF), static_cast<uint8_t>(vid >> 8),
			static_cast<uint8_t>(pid & 0xFF), static_cast<uint8_t>(pid >> 8),
			0x00, 0x01, // bcdDevice
			0, 0, 0,	// iManufacturer, iProduct, iSerialNumber
			1			// bNumConfigurations
		};

		// 2. USB Configuration Descriptor + Interface + HID + Endpoint IN
		uint16_t report_len = report_desc.size();

		config_desc = {
			// Configuration Descriptor (9 bytes)
			0x09, 0x02,
			0x22, 0x00, // wTotalLength (9+9+9+7 = 34 bytes)
			0x01,		// bNumInterfaces
			0x01,		// bConfigurationValue
			0x00,		// iConfiguration
			0xA0,		// bmAttributes (Bus Powered, Remote Wakeup)
			50,			// bMaxPower (100 mA)

			// Interface Descriptor (9 bytes)
			0x09, 0x04,
			0x00, // bInterfaceNumber
			0x00, // bAlternateSetting
			0x01, // bNumEndpoints (1 Interrupt IN)
			0x03, // bInterfaceClass (HID)
			0x01, // bInterfaceSubClass (1 = Boot Interface, 0 = No Boot)
			0x01, // bInterfaceProtocol (1 = Keyboard, 2 = Mouse)
			0x00, // iInterface

			// HID Descriptor (9 bytes)
			0x09, 0x21,
			0x11, 0x01, // bcdHID (v1.11)
			0x00,		// bCountryCode
			0x01,		// bNumDescriptors
			0x22,		// bDescriptorType (Report Descriptor)
			static_cast<uint8_t>(report_len & 0xFF),
			static_cast<uint8_t>((report_len >> 8) & 0xFF),

			// Endpoint Descriptor (EP1 IN Interrupt - 7 bytes)
			0x07, 0x05,
			0x81,	  // bEndpointAddress (EP 1 IN)
			0x03,	  // bmAttributes (Interrupt)
			64, 0x00, // wMaxPacketSize
			0x0A	  // bInterval (10 ms polling)
		};
	}

	std::vector<uint8_t> USBHIDDevice::handle_control_request(const usb_setup_packet_t& setup)
	{
		std::vector<uint8_t> response;

		uint8_t req_type  = setup.bmRequestType;
		uint8_t req		  = setup.bRequest;
		uint8_t desc_type = setup.wValue >> 8;

		// Standard Requests
		if((req_type & 0x60) == 0x00) // Standard
		{
			if(req == 0x06) // GET_DESCRIPTOR
			{
				switch(desc_type)
				{
					case 0x01: // Device Descriptor
						response = device_desc;
						break;
					case 0x02: // Configuration Descriptor
						response = config_desc;
						break;
					case 0x22: // HID Report Descriptor (вызывается хостом для чтения векторов отчета)
						response = report_desc;
						break;
				}
			}
		}
		// Class Requests (HID)
		else if((req_type & 0x60) == 0x20) // Class Request
		{
			switch(req)
			{
				case 0x0A: // SET_IDLE
					idle_rate = setup.wValue >> 8;
					break;
				case 0x0B: // SET_PROTOCOL
					protocol = setup.wValue & 0xFF;
					break;
				case 0x01: // GET_REPORT
					if(!report_queue.empty())
					{
						response = report_queue.front();
					}
					break;
			}
		}

		if(response.size() > setup.wLength)
		{
			response.resize(setup.wLength);
		}

		return response;
	}

	bool USBHIDDevice::get_next_input_report(std::vector<uint8_t>& out_report)
	{
		if(report_queue.empty())
			return false;

		out_report = report_queue.front();
		report_queue.pop();
		return true;
	}
}
