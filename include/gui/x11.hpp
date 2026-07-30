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
#ifdef __PKG_X11
#define VK_USE_PLATFORM_XLIB_KHR

#include "../devices/hid/hid_keyboard.hpp"
#include "window.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <atomic>
#include <cstring>
#include <iostream>
#include <linux/input-event-codes.h>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan_xlib.h>

static constexpr std::array<int, 256> x11_input_conv_table = []
{
	std::array<int, 256> t{};
	t[0]			  = 0x0;
	t[KEY_A]		  = 0x4;
	t[KEY_B]		  = 0x5;
	t[KEY_C]		  = 0x6;
	t[KEY_D]		  = 0x7;
	t[KEY_E]		  = 0x8;
	t[KEY_F]		  = 0x9;
	t[KEY_G]		  = 0xa;
	t[KEY_H]		  = 0xb;
	t[KEY_I]		  = 0xc;
	t[KEY_J]		  = 0xd;
	t[KEY_K]		  = 0xe;
	t[KEY_L]		  = 0xf;
	t[KEY_M]		  = 0x10;
	t[KEY_N]		  = 0x11;
	t[KEY_O]		  = 0x12;
	t[KEY_P]		  = 0x13;
	t[KEY_Q]		  = 0x14;
	t[KEY_R]		  = 0x15;
	t[KEY_S]		  = 0x16;
	t[KEY_T]		  = 0x17;
	t[KEY_U]		  = 0x18;
	t[KEY_V]		  = 0x19;
	t[KEY_W]		  = 0x1A;
	t[KEY_X]		  = 0x1B;
	t[KEY_Y]		  = 0x1C;
	t[KEY_Z]		  = 0x1D;
	t[KEY_1]		  = 0x1E;
	t[KEY_2]		  = 0x1F;
	t[KEY_3]		  = 0x20;
	t[KEY_4]		  = 0x21;
	t[KEY_5]		  = 0x22;
	t[KEY_6]		  = 0x23;
	t[KEY_7]		  = 0x24;
	t[KEY_8]		  = 0x25;
	t[KEY_9]		  = 0x26;
	t[KEY_0]		  = 0x27;
	t[KEY_ENTER]	  = 0x28;
	t[KEY_ESC]		  = 0x29;
	t[KEY_BACKSPACE]  = 0x2A;
	t[KEY_TAB]		  = 0x2B;
	t[KEY_SPACE]	  = 0x2C;
	t[KEY_MINUS]	  = 0x2D;
	t[KEY_EQUAL]	  = 0x2E;
	t[KEY_LEFTBRACE]  = 0x2F;
	t[KEY_RIGHTBRACE] = 0x30;
	t[KEY_BACKSLASH]  = 0x31;
	t[KEY_SEMICOLON]  = 0x33;
	t[KEY_APOSTROPHE] = 0x34;
	t[KEY_GRAVE]	  = 0x35;
	t[KEY_COMMA]	  = 0x36;
	t[KEY_DOT]		  = 0x37;
	t[KEY_SLASH]	  = 0x38;
	t[KEY_CAPSLOCK]	  = 0x39;
	t[KEY_F1]		  = 0x3A;
	t[KEY_F2]		  = 0x3B;
	t[KEY_F3]		  = 0x3C;
	t[KEY_F4]		  = 0x3D;
	t[KEY_F5]		  = 0x3E;
	t[KEY_F6]		  = 0x3F;
	t[KEY_F7]		  = 0x40;
	t[KEY_F8]		  = 0x41;
	t[KEY_F9]		  = 0x42;
	t[KEY_F10]		  = 0x43;
	t[KEY_F11]		  = 0x44;
	t[KEY_F12]		  = 0x45;
	t[KEY_SYSRQ]	  = 0x46;
	t[KEY_SCROLLLOCK] = 0x47;
	t[KEY_PAUSE]	  = 0x48;
	t[KEY_INSERT]	  = 0x49;
	t[KEY_HOME]		  = 0x4A;
	t[KEY_PAGEUP]	  = 0x4B;
	t[KEY_DELETE]	  = 0x4C;
	t[KEY_END]		  = 0x4D;
	t[KEY_PAGEDOWN]	  = 0x4E;
	t[KEY_RIGHT]	  = 0x4F;
	t[KEY_LEFT]		  = 0x50;
	t[KEY_DOWN]		  = 0x51;
	t[KEY_UP]		  = 0x52;
	return t;
}();

struct AppWindow
{
	Display* x11Display = nullptr;
	Window x11Window	= 0;
	InputState input;
	std::shared_ptr<rv64vm::dev::HID_Keyboard> kb;
	int width  = 800;
	int height = 600;

	GC gc				 = None;
	XImage* image		 = nullptr;
	Atom wmDeleteMessage = None;
	std::atomic<bool> running{ true };
	std::mutex* window_mutex = nullptr;
};

inline bool InitializeNativeWindow(AppWindow& appWindow, const std::string& title)
{
	appWindow.x11Display = XOpenDisplay(nullptr);
	if(!appWindow.x11Display)
		return false;

	appWindow.window_mutex = new std::mutex();

	int screen	= DefaultScreen(appWindow.x11Display);
	Window root = RootWindow(appWindow.x11Display, screen);

	appWindow.x11Window = XCreateSimpleWindow(
		appWindow.x11Display, root,
		0, 0, appWindow.width, appWindow.height, 1,
		BlackPixel(appWindow.x11Display, screen),
		WhitePixel(appWindow.x11Display, screen));

	XStoreName(appWindow.x11Display, appWindow.x11Window, title.c_str());

	XSelectInput(appWindow.x11Display, appWindow.x11Window,
				 KeyPressMask | KeyReleaseMask | StructureNotifyMask);

	appWindow.wmDeleteMessage = XInternAtom(appWindow.x11Display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(appWindow.x11Display, appWindow.x11Window, &appWindow.wmDeleteMessage, 1);

	appWindow.gc = XCreateGC(appWindow.x11Display, appWindow.x11Window, 0, nullptr);

	XMapWindow(appWindow.x11Display, appWindow.x11Window);
	XFlush(appWindow.x11Display);

	appWindow.running = true;
	return true;
}

inline bool CreateVulkanSurface(VkInstance instance, const AppWindow& appWindow, VkSurfaceKHR& outSurface)
{
	VkXlibSurfaceCreateInfoKHR createInfo{};
	createInfo.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	createInfo.dpy	  = appWindow.x11Display;
	createInfo.window = appWindow.x11Window;

	VkResult result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
	return result == VK_SUCCESS;
}

inline void TerminateNativeWindow(AppWindow& appWindow)
{
	appWindow.running = false;

	if(appWindow.x11Display)
	{
		if(appWindow.image)
		{
			XDestroyImage(appWindow.image);
			appWindow.image = nullptr;
		}
		if(appWindow.gc != None)
		{
			XFreeGC(appWindow.x11Display, appWindow.gc);
			appWindow.gc = None;
		}
		XDestroyWindow(appWindow.x11Display, appWindow.x11Window);
		XCloseDisplay(appWindow.x11Display);
		appWindow.x11Display = nullptr;
	}

	if(appWindow.window_mutex)
	{
		delete appWindow.window_mutex;
		appWindow.window_mutex = nullptr;
	}
}

inline void StartEventLoop(AppWindow& appWindow)
{
	std::thread([&appWindow]()
	{
		XEvent ev;
		while(appWindow.running)
		{
			if(!appWindow.x11Display)
				break;

			XNextEvent(appWindow.x11Display, &ev);

			std::lock_guard<std::mutex> lock(*appWindow.window_mutex);

			switch(ev.type)
			{
				case KeyPress:
				case KeyRelease:
				{
					XKeyEvent* keyEv = &ev.xkey;
					uint8_t key		 = keyEv->keycode - 8;

					if(ev.type == KeyPress)
						key_press(appWindow.input, key);
					else
						key_release(appWindow.input, key);

					uint8_t out[6] = { 0 };
					build_report_keys(appWindow.input, out);

					uint8_t key1 = x11_input_conv_table[out[0]];
					uint8_t key2 = x11_input_conv_table[out[1]];
					uint8_t key3 = x11_input_conv_table[out[2]];
					uint8_t key4 = x11_input_conv_table[out[3]];
					uint8_t key5 = x11_input_conv_table[out[4]];
					uint8_t key6 = x11_input_conv_table[out[5]];

					uint8_t mods = (appWindow.input.lctrl << 0)
								   | (appWindow.input.lshift << 1)
								   | (appWindow.input.lalt << 2)
								   | (appWindow.input.rctrl << 4)
								   | (appWindow.input.rshift << 5)
								   | (appWindow.input.ralt << 6);

					if(appWindow.kb)
					{
						appWindow.kb->update(mods, key1, key2, key3, key4, key5, key6,
											 appWindow.input.pressed_count > 6);
					}
					break;
				}

				case ClientMessage:
				{
					if((Atom)ev.xclient.data.l[0] == appWindow.wmDeleteMessage)
					{
						appWindow.running = false;
					}
					break;
				}

				default:
					break;
			}
		}
	}).detach();
}

inline void UpdateFrame(AppWindow& appWindow, const uint8_t* raw_pixels)
{
	if(!appWindow.running || !appWindow.x11Display || !appWindow.x11Window || !raw_pixels)
		return;

	std::lock_guard<std::mutex> lock(*appWindow.window_mutex);

	if(!appWindow.image || appWindow.image->width != appWindow.width || appWindow.image->height != appWindow.height)
	{
		if(appWindow.image)
			XDestroyImage(appWindow.image);

		int depth = DefaultDepth(appWindow.x11Display, DefaultScreen(appWindow.x11Display));

		appWindow.image = XCreateImage(
			appWindow.x11Display,
			DefaultVisual(appWindow.x11Display, DefaultScreen(appWindow.x11Display)),
			depth,
			ZPixmap,
			0,
			nullptr,
			appWindow.width,
			appWindow.height,
			32,
			appWindow.width * 4);
		if(!appWindow.image)
			return;

		if(appWindow.image->data == nullptr)
		{
			appWindow.image->data = (char*)malloc(appWindow.image->bytes_per_line * appWindow.height);
			if(!appWindow.image->data)
			{
				XDestroyImage(appWindow.image);
				appWindow.image = nullptr;
				return;
			}
		}
	}

	memcpy(appWindow.image->data, raw_pixels, appWindow.image->bytes_per_line * appWindow.height);

	XPutImage(appWindow.x11Display, appWindow.x11Window, appWindow.gc,
			  appWindow.image, 0, 0, 0, 0, appWindow.width, appWindow.height);

	XFlush(appWindow.x11Display);
}

#endif // __PKG_X11
