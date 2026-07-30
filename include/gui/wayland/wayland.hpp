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
#include <wayland-client-protocol.h>
#ifdef __PKG_WAYLAND
#include "../window.hpp"
extern "C"
{
#include "xdg-shell-client-protocol.h"
}
#include <cstring>
#include <iostream>
#include <vulkan/vulkan_wayland.h>
#include <wayland-client-core.h>
#include <wayland-client.h>

#include "../../devices/hid/hid_keyboard.hpp"
#include <algorithm>
#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <mutex>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

static constexpr std::array<int, 256> wl_input_conv_table = []
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

struct WaylandWindow
{
	// Core Wayland Handles
	struct wl_display* display		 = nullptr;
	struct wl_registry* registry	 = nullptr;
	struct wl_compositor* compositor = nullptr;
	struct xdg_wm_base* xdg_wm_base	 = nullptr;

	// Window Surface Handles
	struct wl_surface* wl_surface	  = nullptr;
	struct xdg_surface* xdg_surface	  = nullptr;
	struct xdg_toplevel* xdg_toplevel = nullptr;

	struct wl_shm* shm				   = nullptr;
	struct wl_event_queue* event_queue = nullptr;
	bool configured					   = false;
	bool running					   = true;

	struct wl_seat* seat			= nullptr;
	struct wl_keyboard* wl_keyboard = nullptr;

	std::unique_ptr<std::mutex> window_mutex = std::make_unique<std::mutex>();
	struct wl_buffer* wl_buffer				 = nullptr;
	struct wl_shm_pool* wl_shm_pool			 = nullptr;
	uint32_t* pixel_shared_memory			 = nullptr;
	size_t shm_size							 = 0;
	int shm_fd								 = -1;
	int width								 = 0;
	int height								 = 0;

	InputState* input;
	std::shared_ptr<rv64vm::dev::HID_Keyboard> kb;
};
namespace
{
	static void xdg_surface_handle_configure(void* data, struct xdg_surface* xdg_surface, uint32_t serial)
	{
		xdg_surface_ack_configure(xdg_surface, serial);

		auto* window	   = static_cast<WaylandWindow*>(data);
		window->configured = true; // Signal that the window is ready for rendering
	}

	static const struct xdg_surface_listener xdg_surface_listener = {
		.configure = xdg_surface_handle_configure
	};

	static void xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
	{
		// respond to compositor pings to prove your application has not frozen
		xdg_wm_base_pong(xdg_wm_base, serial);
	}

	static const struct xdg_wm_base_listener xdg_wm_base_listener = {
		.ping = xdg_wm_base_ping
	};

	inline void xdg_toplevel_handle_close(void* data, struct xdg_toplevel* xdg_toplevel)
	{
		auto* window	= static_cast<WaylandWindow*>(data);
		window->running = false;
	}

	inline const struct xdg_toplevel_listener xdg_toplevel_listener = {
		.configure = [](void*, struct xdg_toplevel*, int32_t, int32_t, struct wl_array*) {},
		.close	   = xdg_toplevel_handle_close
	};

	static void keyboard_handle_keymap(void* data, struct wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
	{
		auto* window = static_cast<WaylandWindow*>(data);
	}
	static void keyboard_handle_enter(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys)
	{
	}
	static void keyboard_handle_leave(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, struct wl_surface* surface)
	{
		auto* window = static_cast<WaylandWindow*>(data);
	}
	static void keyboard_handle_key(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
	{
		auto* window = static_cast<WaylandWindow*>(data);
		if(window && window->kb)
		{
			if(state == WL_KEYBOARD_KEY_STATE_PRESSED)
				key_press(*window->input, key);
			else
				key_release(*window->input, key);

			uint8_t out[6] = { 0 };
			build_report_keys(*window->input, out);
			uint8_t key1 = wl_input_conv_table[out[0]];
			uint8_t key2 = wl_input_conv_table[out[1]];
			uint8_t key3 = wl_input_conv_table[out[2]];
			uint8_t key4 = wl_input_conv_table[out[3]];
			uint8_t key5 = wl_input_conv_table[out[4]];
			uint8_t key6 = wl_input_conv_table[out[5]];

			uint8_t mods = (window->input->lctrl << 0) | (window->input->lshift << 1) | (window->input->lalt << 2) | (window->input->rctrl << 4) | (window->input->rshift << 5) | (window->input->ralt << 6);
			window->kb->update(mods, key1, key2, key3, key4, key5, key6, window->input->pressed_count > 6);
		}
	}
	static void keyboard_handle_modifiers(void* data, struct wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
	{
		auto* window = static_cast<WaylandWindow*>(data);
	}
	static void keyboard_handle_repeat_info(void* data, struct wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
	{
	}

	inline const struct wl_keyboard_listener keyboard_listener = {
		.keymap		 = keyboard_handle_keymap,
		.enter		 = keyboard_handle_enter,
		.leave		 = keyboard_handle_leave,
		.key		 = keyboard_handle_key,
		.modifiers	 = keyboard_handle_modifiers,
		.repeat_info = keyboard_handle_repeat_info
	};

	static void seat_handle_capabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
	{
		auto* window = static_cast<WaylandWindow*>(data);

		if(capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
		{
			if(!window->wl_keyboard)
			{
				window->wl_keyboard = wl_seat_get_keyboard(seat);

				wl_keyboard_add_listener(window->wl_keyboard, &keyboard_listener, window);
			}
		}
		else
		{
			if(window->wl_keyboard)
			{
				wl_keyboard_release(window->wl_keyboard);
				window->wl_keyboard = nullptr;
			}
		}
	}

	static void seat_handle_name(void* data, struct wl_seat* seat, const char* name)
	{
	}

	static const struct wl_seat_listener seat_listener = {
		.capabilities = seat_handle_capabilities,
		.name		  = seat_handle_name,
	};

	static void registry_global_handler(void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version)
	{
		auto* window = static_cast<WaylandWindow*>(data);

		if(std::strcmp(interface, "wl_compositor") == 0)
		{
			window->compositor = static_cast<struct wl_compositor*>(
				wl_registry_bind(registry, id, &wl_compositor_interface, 4));
		}

		else if(std::strcmp(interface, "xdg_wm_base") == 0)
		{
			window->xdg_wm_base = static_cast<struct xdg_wm_base*>(
				wl_registry_bind(registry, id, &xdg_wm_base_interface, 1));
			xdg_wm_base_add_listener(window->xdg_wm_base, &xdg_wm_base_listener, window);
		}

		else if(std::strcmp(interface, "wl_shm") == 0)
		{
			window->shm = static_cast<struct wl_shm*>(
				wl_registry_bind(registry, id, &wl_shm_interface, 1));
		}
		else if(std::strcmp(interface, "wl_seat") == 0)
		{
			window->seat = static_cast<struct wl_seat*>(
				wl_registry_bind(registry, id, &wl_seat_interface, 1));
			wl_seat_add_listener(window->seat, &seat_listener, window);
		}
	}

	static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t id)
	{
		// i dont think you gonna physically disconnect your monitor while ts works, soooooooo...
	}

	static const struct wl_registry_listener registry_listener = {
		.global		   = registry_global_handler,
		.global_remove = registry_global_remove
	};

	static bool InitializeWaylandScreen(WaylandWindow& window, int width, int height)
	{
		window.display = wl_display_connect(nullptr);
		if(!window.display)
		{
			std::cerr << "Failed to connect to a Wayland display socket.\n";
			return false;
		}
		window.registry = wl_display_get_registry(window.display);
		wl_registry_add_listener(window.registry, &registry_listener, &window);

		wl_display_roundtrip(window.display);
		wl_display_roundtrip(window.display);

		if(!window.compositor || !window.xdg_wm_base)
		{
			std::cerr << "Critical Error: Wayland compositor missing components.\n";
			return false;
		}

		window.wl_surface = wl_compositor_create_surface(window.compositor);
		if(!window.wl_surface) return false;

		window.xdg_surface = xdg_wm_base_get_xdg_surface(window.xdg_wm_base, window.wl_surface);
		xdg_surface_add_listener(window.xdg_surface, &xdg_surface_listener, &window);

		window.xdg_toplevel = xdg_surface_get_toplevel(window.xdg_surface);
		xdg_toplevel_add_listener(window.xdg_toplevel, &xdg_toplevel_listener, &window);

		xdg_toplevel_set_title(window.xdg_toplevel, "rv64-vm");
		xdg_toplevel_set_min_size(window.xdg_toplevel, width, height);
		xdg_toplevel_set_max_size(window.xdg_toplevel, width, height);
		wl_surface_commit(window.wl_surface);
		while(!window.configured)
		{
			if(wl_display_dispatch(window.display) < 0)
			{
				return false;
			}
		}
		window.width	= width;
		window.height	= height;
		int stride		= width * 4;
		window.shm_size = stride * height;

		window.shm_fd = memfd_create("wayland-shm-pool", MFD_CLOEXEC);
		if(window.shm_fd >= 0)
		{
			if(ftruncate(window.shm_fd, window.shm_size) >= 0)
			{
				window.pixel_shared_memory = (uint32_t*)mmap(nullptr, window.shm_size,
															 PROT_READ | PROT_WRITE, MAP_SHARED, window.shm_fd, 0);

				if(window.pixel_shared_memory != MAP_FAILED)
				{
					std::fill_n(window.pixel_shared_memory, width * height, 0xFF000000);

					window.wl_shm_pool = wl_shm_create_pool(window.shm, window.shm_fd, window.shm_size);
					window.wl_buffer   = wl_shm_pool_create_buffer(window.wl_shm_pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);

					wl_surface_attach(window.wl_surface, window.wl_buffer, 0, 0);
					wl_surface_damage(window.wl_surface, 0, 0, width, height);
					wl_surface_commit(window.wl_surface);

					wl_display_flush(window.display);
					wl_display_dispatch_pending(window.display);
				}
			}
		}

		return true;
	}

	inline void StartWaylandEventThread(WaylandWindow& window)
	{
		window.event_queue = wl_display_create_queue(window.display);

		if(window.registry) wl_proxy_set_queue((struct wl_proxy*)window.registry, window.event_queue);
		if(window.xdg_wm_base) wl_proxy_set_queue((struct wl_proxy*)window.xdg_wm_base, window.event_queue);
		if(window.xdg_surface) wl_proxy_set_queue((struct wl_proxy*)window.xdg_surface, window.event_queue);
		if(window.xdg_toplevel) wl_proxy_set_queue((struct wl_proxy*)window.xdg_toplevel, window.event_queue);
		if(window.seat) wl_proxy_set_queue((struct wl_proxy*)window.seat, window.event_queue);
		if(window.wl_keyboard) wl_proxy_set_queue((struct wl_proxy*)window.wl_keyboard, window.event_queue);

		std::thread([&window]()
		{
			while(window.running)
			{
				{
					std::lock_guard<std::mutex> lock(*window.window_mutex);
					while(wl_display_prepare_read_queue(window.display, window.event_queue) != 0)
					{
						wl_display_dispatch_queue_pending(window.display, window.event_queue);
					}
					wl_display_flush(window.display);
				}

				if(wl_display_read_events(window.display) < 0)
				{
					break;
				}

				{
					std::lock_guard<std::mutex> lock(*window.window_mutex);
					wl_display_dispatch_queue_pending(window.display, window.event_queue);
				}
			}
		}).detach();
	}

	static void CleanupWaylandScreen(WaylandWindow& window)
	{
		if(window.display)
		{
			wl_display_flush(window.display);
		}

		if(window.wl_keyboard) wl_keyboard_release(window.wl_keyboard);
		if(window.seat) wl_seat_release(window.seat);

		if(window.xdg_toplevel) xdg_toplevel_destroy(window.xdg_toplevel);
		if(window.xdg_surface) xdg_surface_destroy(window.xdg_surface);
		if(window.wl_surface) wl_surface_destroy(window.wl_surface);

		if(window.wl_buffer) wl_buffer_destroy(window.wl_buffer);
		if(window.wl_shm_pool) wl_shm_pool_destroy(window.wl_shm_pool);
		if(window.pixel_shared_memory && window.pixel_shared_memory != MAP_FAILED) munmap(window.pixel_shared_memory, window.shm_size);
		if(window.shm_fd >= 0) close(window.shm_fd);

		if(window.xdg_wm_base) xdg_wm_base_destroy(window.xdg_wm_base);
		if(window.shm) wl_shm_destroy(window.shm);

		if(window.registry) wl_registry_destroy(window.registry);

		if(window.event_queue) wl_event_queue_destroy(window.event_queue);

		if(window.display) wl_display_disconnect(window.display);
	}

	inline void UpdateWaylandFrame(WaylandWindow& window, const uint8_t* raw_pixels)
	{
		if(!window.running || !window.wl_surface) return;
		if(!window.pixel_shared_memory || window.pixel_shared_memory == MAP_FAILED)
		{
			std::cerr << "[Wayland] Error: Shared memory is not mapped.\n";
			return;
		}

		if(!raw_pixels)
		{
			std::cerr << "[Wayland] Error: Source raw_pixels pointer is null.\n";
			return;
		}

		std::memcpy(window.pixel_shared_memory, raw_pixels, window.shm_size);

		wl_surface_damage(window.wl_surface, 0, 0, window.width, window.height);
		wl_surface_attach(window.wl_surface, window.wl_buffer, 0, 0);
		wl_surface_commit(window.wl_surface);

		wl_display_flush(window.display);
	}
}

struct AppWindow
{
	WaylandWindow wl;
	InputState input;
	std::shared_ptr<rv64vm::dev::HID_Keyboard> kb;
	int width  = 800;
	int height = 600;
};
inline bool InitializeNativeWindow(AppWindow& appWindow, const std::string& title)
{
	(void)title;
	WaylandWindow window = {};
	appWindow.wl		 = std::move(window);
	appWindow.wl.kb		 = appWindow.kb;
	appWindow.wl.input	 = &appWindow.input;
	bool out			 = InitializeWaylandScreen(appWindow.wl, appWindow.width, appWindow.height);
	return out;
}

inline bool CreateVulkanSurface(VkInstance instance, const AppWindow& appWindow, VkSurfaceKHR& outSurface)
{
	VkWaylandSurfaceCreateInfoKHR createInfo{};
	createInfo.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.display = appWindow.wl.display;
	createInfo.surface = appWindow.wl.wl_surface;

	VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &outSurface);
	return result == VK_SUCCESS;
}

inline void TerminateNativeWindow(AppWindow& appWindow)
{
	appWindow.wl.running = false;
	CleanupWaylandScreen(appWindow.wl);
}
inline void StartEventLoop(AppWindow& appWindow)
{
	StartWaylandEventThread(appWindow.wl);
}
inline void UpdateFrame(AppWindow& appWindow, const uint8_t* raw_pixels)
{
	UpdateWaylandFrame(appWindow.wl, raw_pixels);
}
#include <wayland-util.h>

extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface xdg_popup_interface;

inline const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy",			 "",	 nullptr },
	{ "create_positioner", "n",	nullptr },
	{ "get_xdg_surface",	 "no", nullptr },
	{ "pong",			  "u",  nullptr },
};

inline const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", nullptr },
};

inline const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base",
	1,
	4,
	xdg_wm_base_requests,
	1,
	xdg_wm_base_events,
};

#endif
