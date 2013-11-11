/*
 * Copyright 2013 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * This file is part of libnsfb, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#define _XOPEN_SOURCE 500

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "libnsfb.h"
#include "libnsfb_event.h"
#include "libnsfb_plot.h"
#include "libnsfb_plot_util.h"

#include "nsfb.h"
#include "surface.h"
#include "plot.h"
#include "cursor.h"

#include <winpr/crt.h>
#include <winpr/input.h>
#include <winpr/collections.h>

#include <freerds/freerds.h>
#include <freerds/service_helper.h>

static int connected = 0;
static DWORD SessionId = 1;
static const char* endpoint = "NetSurf";

static wQueue* InputQueue = NULL;

static int framebufferSize = 0;
static RDS_FRAMEBUFFER framebuffer = { 0 };

static rdsService* service = NULL;
static rdsModuleConnector* connector = NULL;

static DWORD KEYCODE_TO_VKCODE_NSFB[128] =
{
	0, /* 0, NSFB_KEY_UNKNOWN */
	0, /* 1 */
	0, /* 2 */
	0, /* 3 */
	0, /* 4 */
	0, /* 5 */
	0, /* 6 */
	0, /* 7 */
	VK_BACK, /* 8, NSFB_KEY_BACKSPACE */
	VK_TAB, /* 9, NSFB_KEY_TAB */
	0, /* 10, NSFB_KEY_LF */
	0, /* 11 */
	VK_CLEAR, /* 12, NSFB_KEY_CLEAR */
	VK_RETURN, /* 13, NSFB_KEY_RETURN */
	0, /* 14 */
	0, /* 15 */
	0, /* 16 */
	0, /* 17 */
	0, /* 18 */
	VK_PAUSE | KBDEXT, /* 19, NSFB_KEY_PAUSE */
	0, /* 20 */
	0, /* 21 */
	0, /* 22 */
	0, /* 23 */
	0, /* 24 */
	0, /* 25 */
	0, /* 26 */
	VK_ESCAPE, /* 27, NSFB_KEY_ESCAPE */
	0, /* 28 */
	0, /* 29 */
	0, /* 30 */
	0, /* 31 */
	VK_SPACE, /* 32, NSFB_KEY_SPACE */
	0, /* 33, NSFB_KEY_EXCLAIM */
	0, /* 34, NSFB_KEY_QUOTEDBL */
	0, /* 35, NSFB_KEY_HASH */
	0, /* 36, NSFB_KEY_DOLLAR */
	0, /* 37 */
	0, /* 38, NSFB_KEY_AMPERSAND */
	0, /* 39, NSFB_KEY_QUOTE */
	0, /* 40, NSFB_KEY_LEFTPAREN */
	0, /* 41, NSFB_KEY_RIGHTPAREN */
	0, /* 42, NSFB_KEY_ASTERISK */
	VK_OEM_PLUS, /* 43, NSFB_KEY_PLUS */
	VK_OEM_COMMA, /* 44, NSFB_KEY_COMMA */
	VK_OEM_MINUS, /* 45, NSFB_KEY_MINUS */
	VK_OEM_PERIOD, /* 46, NSFB_KEY_PERIOD */
	VK_OEM_2, /* 47, NSFB_KEY_SLASH */
	VK_KEY_0, /* 48, NSFB_KEY_0 */
	VK_KEY_1, /* 49, NSFB_KEY_1 */
	VK_KEY_2, /* 50, NSFB_KEY_2 */
	VK_KEY_3, /* 51, NSFB_KEY_3 */
	VK_KEY_4, /* 52, NSFB_KEY_4 */
	VK_KEY_5, /* 53, NSFB_KEY_5 */
	VK_KEY_6, /* 54, NSFB_KEY_6 */
	VK_KEY_7, /* 55, NSFB_KEY_7 */
	VK_KEY_8, /* 56, NSFB_KEY_8 */
	VK_KEY_9, /* 57, NSFB_KEY_9 */
	VK_OEM_1, /* 58, NSFB_KEY_COLON */
	0, /* 59, NSFB_KEY_SEMICOLON */
	0, /* 60, NSFB_KEY_LESS */
	0, /* 61, NSFB_KEY_EQUALS */
	0, /* 62, NSFB_KEY_GREATER */
	0, /* 63, NSFB_KEY_QUESTION */
	0, /* 64, NSFB_KEY_AT */
	0, /* 65 */
	0, /* 66 */
	0, /* 67 */
	0, /* 68 */
	0, /* 69 */
	0, /* 70 */
	0, /* 71 */
	0, /* 72 */
	0, /* 73 */
	0, /* 74 */
	0, /* 75 */
	0, /* 76 */
	0, /* 77 */
	0, /* 78 */
	0, /* 79 */
	0, /* 80 */
	0, /* 81 */
	0, /* 82 */
	0, /* 83 */
	0, /* 84 */
	0, /* 85 */
	0, /* 86 */
	0, /* 87 */
	0, /* 88 */
	0, /* 89 */
	0, /* 90 */
	0, /* 91, NSFB_KEY_LEFTBRACKET */
	VK_OEM_5, /* 92, NSFB_KEY_BACKSLASH */
	0, /* 93, NSFB_KEY_RIGHTBRACKET */
	0, /* 94, NSFB_KEY_CARET */
	0, /* 95, NSFB_KEY_UNDERSCORE */
	0, /* 96, NSFB_KEY_BACKQUOTE */
	VK_KEY_A, /* 97, NSFB_KEY_a */
	VK_KEY_B, /* 98, NSFB_KEY_b */
	VK_KEY_C, /* 99, NSFB_KEY_c */
	VK_KEY_D, /* 100, NSFB_KEY_d */
	VK_KEY_E, /* 101, NSFB_KEY_e */
	VK_KEY_F, /* 102, NSFB_KEY_f */
	VK_KEY_G, /* 103, NSFB_KEY_g */
	VK_KEY_H, /* 104, NSFB_KEY_h */
	VK_KEY_I, /* 105, NSFB_KEY_i */
	VK_KEY_J, /* 106, NSFB_KEY_j */
	VK_KEY_K, /* 107, NSFB_KEY_k */
	VK_KEY_L, /* 108, NSFB_KEY_l */
	VK_KEY_M, /* 109, NSFB_KEY_m */
	VK_KEY_N, /* 110, NSFB_KEY_n */
	VK_KEY_O, /* 111, NSFB_KEY_o */
	VK_KEY_P, /* 112, NSFB_KEY_p */
	VK_KEY_Q, /* 113, NSFB_KEY_q */
	VK_KEY_R, /* 114, NSFB_KEY_r */
	VK_KEY_S, /* 115, NSFB_KEY_s */
	VK_KEY_T, /* 116, NSFB_KEY_t */
	VK_KEY_U, /* 117, NSFB_KEY_u */
	VK_KEY_V, /* 118, NSFB_KEY_v */
	VK_KEY_W, /* 119, NSFB_KEY_w */
	VK_KEY_X, /* 120, NSFB_KEY_x */
	VK_KEY_Y, /* 121, NSFB_KEY_y */
	VK_KEY_Z, /* 122, NSFB_KEY_z */
	0, /* 123 */
	0, /* 124 */
	0, /* 125 */
	0, /* 126 */
	VK_DELETE | KBDEXT, /* 127, NSFB_KEY_DELETE */
};

static enum nsfb_key_code_e freerds_get_keycode_from_vkcode(DWORD vkcode)
{
	int index;
	DWORD keycode = NSFB_KEY_UNKNOWN;

	for (index = 0; index < 128; index++)
	{
		if (vkcode == KEYCODE_TO_VKCODE_NSFB[index])
		{
			keycode = index;
			break;
		}
	}

	if (keycode != NSFB_KEY_UNKNOWN)
		return (enum nsfb_key_code_e) keycode;

	switch (vkcode)
	{
		case VK_UP:
			keycode = NSFB_KEY_UP;
			break;

		case VK_DOWN:
			keycode = NSFB_KEY_DOWN;
			break;

		case VK_RIGHT:
			keycode = NSFB_KEY_RIGHT;
			break;

		case VK_LEFT:
			keycode = NSFB_KEY_LEFT;
			break;

		case VK_INSERT:
			keycode = NSFB_KEY_INSERT;
			break;

		case VK_HOME:
			keycode = NSFB_KEY_HOME;
			break;

		case VK_PRIOR:
			keycode = NSFB_KEY_PAGEUP;
			break;

		case VK_NEXT:
			keycode = NSFB_KEY_PAGEDOWN;
			break;
	}

	if (keycode != NSFB_KEY_UNKNOWN)
		return (enum nsfb_key_code_e) keycode;

	switch (vkcode)
	{
		case VK_NUMPAD0:
			keycode = NSFB_KEY_KP0;
			break;

		case VK_NUMPAD1:
			keycode = NSFB_KEY_KP1;
			break;

		case VK_NUMPAD2:
			keycode = NSFB_KEY_KP2;
			break;

		case VK_NUMPAD3:
			keycode = NSFB_KEY_KP3;
			break;

		case VK_NUMPAD4:
			keycode = NSFB_KEY_KP4;
			break;

		case VK_NUMPAD5:
			keycode = NSFB_KEY_KP5;
			break;

		case VK_NUMPAD6:
			keycode = NSFB_KEY_KP6;
			break;

		case VK_NUMPAD7:
			keycode = NSFB_KEY_KP7;
			break;

		case VK_NUMPAD8:
			keycode = NSFB_KEY_KP8;
			break;

		case VK_NUMPAD9:
			keycode = NSFB_KEY_KP9;
			break;

		case VK_DECIMAL:
			keycode = NSFB_KEY_KP_PERIOD;
			break;

		case VK_DIVIDE | KBDEXT:
			keycode = NSFB_KEY_KP_DIVIDE;
			break;

		case VK_MULTIPLY:
			keycode = NSFB_KEY_KP_MULTIPLY;
			break;

		case VK_SUBTRACT:
			keycode = NSFB_KEY_KP_MINUS;
			break;

		case VK_ADD:
			keycode = NSFB_KEY_KP_PLUS;
			break;

		case VK_RETURN | KBDEXT:
			keycode = NSFB_KEY_KP_ENTER;
			break;

		case VK_SEPARATOR:
			keycode = NSFB_KEY_KP_EQUALS;
			break;
	}

	if (keycode != NSFB_KEY_UNKNOWN)
		return (enum nsfb_key_code_e) keycode;

	switch (vkcode)
	{
		case VK_F1:
			keycode = NSFB_KEY_F1;
			break;

		case VK_F2:
			keycode = NSFB_KEY_F2;
			break;

		case VK_F3:
			keycode = NSFB_KEY_F3;
			break;

		case VK_F4:
			keycode = NSFB_KEY_F4;
			break;

		case VK_F5:
			keycode = NSFB_KEY_F5;
			break;

		case VK_F6:
			keycode = NSFB_KEY_F6;
			break;

		case VK_F7:
			keycode = NSFB_KEY_F7;
			break;

		case VK_F8:
			keycode = NSFB_KEY_F8;
			break;

		case VK_F9:
			keycode = NSFB_KEY_F9;
			break;

		case VK_F10:
			keycode = NSFB_KEY_F10;
			break;

		case VK_F11:
			keycode = NSFB_KEY_F11;
			break;

		case VK_F12:
			keycode = NSFB_KEY_F12;
			break;

		case VK_F13:
			keycode = NSFB_KEY_F13;
			break;

		case VK_F14:
			keycode = NSFB_KEY_F14;
			break;

		case VK_F15:
			keycode = NSFB_KEY_F15;
			break;
	}

	if (keycode != NSFB_KEY_UNKNOWN)
		return (enum nsfb_key_code_e) keycode;

	switch (vkcode)
	{
		case VK_NUMLOCK:
			keycode = NSFB_KEY_NUMLOCK;
			break;

		case VK_CAPITAL:
			keycode = NSFB_KEY_CAPSLOCK;
			break;

		case VK_SCROLL:
			keycode = NSFB_KEY_SCROLLOCK;
			break;

		case VK_RSHIFT:
			keycode = NSFB_KEY_RSHIFT;
			break;

		case VK_LSHIFT:
			keycode = NSFB_KEY_LSHIFT;
			break;

		case VK_RCONTROL:
			keycode = NSFB_KEY_RCTRL;
			break;

		case VK_LCONTROL:
			keycode = NSFB_KEY_LCTRL;
			break;

		case VK_RMENU | KBDEXT:
			keycode = NSFB_KEY_RALT;
			break;

		case VK_LMENU:
			keycode = NSFB_KEY_LALT;
			break;
	}

	if (keycode != NSFB_KEY_UNKNOWN)
		return (enum nsfb_key_code_e) keycode;

	switch (vkcode)
	{
		case VK_HELP:
			keycode = NSFB_KEY_HELP;
			break;

		case VK_SNAPSHOT:
			keycode = NSFB_KEY_PRINT;
			break;

		case VK_LWIN:
			keycode = NSFB_KEY_MENU;
			break;

		case VK_POWER:
			keycode = NSFB_KEY_POWER;
			break;
	}

	return (enum nsfb_key_code_e) keycode;
}

static int freerds_check_shared_framebuffer()
{
	if (!connected)
		return 0;

	if (!framebuffer.fbAttached)
	{
		RDS_MSG_SHARED_FRAMEBUFFER msg;

		msg.attach = 1;
		msg.width = framebuffer.fbWidth;
		msg.height = framebuffer.fbHeight;
		msg.scanline = framebuffer.fbScanline;
		msg.segmentId = framebuffer.fbSegmentId;
		msg.bitsPerPixel = framebuffer.fbBitsPerPixel;
		msg.bytesPerPixel = framebuffer.fbBytesPerPixel;

		msg.type = RDS_SERVER_SHARED_FRAMEBUFFER;
		connector->server->SharedFramebuffer(connector, &msg);

		framebuffer.fbAttached = 1;
	}

	return 0;
}

/**
 * FreeRDS Interface
 */

static int freerds_client_synchronize_keyboard_event(rdsModuleConnector* connector, DWORD flags)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return 0;
}

static int freerds_client_scancode_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code, DWORD keyboardType)
{
	DWORD vkcode;
	nsfb_event_t* event;
	enum nsfb_key_code_e keycode;

	fprintf(stderr, "%s\n", __FUNCTION__);

	if (!connected)
		return 0;

	vkcode = GetVirtualKeyCodeFromVirtualScanCode(code, keyboardType);
	keycode = freerds_get_keycode_from_vkcode(vkcode);

	event = (nsfb_event_t*) malloc(sizeof(nsfb_event_t));
	ZeroMemory(event, sizeof(nsfb_event_t));

	if (flags & KBD_FLAGS_DOWN)
		event->type = NSFB_EVENT_KEY_DOWN;
	else
		event->type = NSFB_EVENT_KEY_UP;

	event->value.keycode = keycode;

	Queue_Enqueue(InputQueue, (void*) event);

	return 0;
}

static int freerds_client_virtual_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return 0;
}

static int freerds_client_unicode_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return 0;
}

static int freerds_client_mouse_event(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y)
{
	nsfb_event_t* event;

	fprintf(stderr, "%s\n", __FUNCTION__);

	if (!connected)
		return 0;

	if (flags & PTR_FLAGS_MOVE)
	{
		event = (nsfb_event_t*) malloc(sizeof(nsfb_event_t));
		ZeroMemory(event, sizeof(nsfb_event_t));

		event->type = NSFB_EVENT_MOVE_ABSOLUTE;

		event->value.vector.x = x;
		event->value.vector.y = y;
		event->value.vector.z = 0;

		Queue_Enqueue(InputQueue, (void*) event);
	}

	if (flags & PTR_FLAGS_WHEEL)
	{
		if (flags & PTR_FLAGS_WHEEL_NEGATIVE)
		{

		}
		else
		{

		}
	}
	else if (flags & PTR_FLAGS_BUTTON1)
	{
		/* Left Mouse Button */

		event = (nsfb_event_t*) malloc(sizeof(nsfb_event_t));
		ZeroMemory(event, sizeof(nsfb_event_t));

		event->value.keycode = NSFB_KEY_MOUSE_1;

		if (flags & PTR_FLAGS_DOWN)
			event->type = NSFB_EVENT_KEY_DOWN;
		else
			event->type = NSFB_EVENT_KEY_UP;

		Queue_Enqueue(InputQueue, (void*) event);
	}
	else if (flags & PTR_FLAGS_BUTTON2)
	{
		/* Right Mouse Button */

		event = (nsfb_event_t*) malloc(sizeof(nsfb_event_t));
		ZeroMemory(event, sizeof(nsfb_event_t));

		event->value.keycode = NSFB_KEY_MOUSE_3;

		if (flags & PTR_FLAGS_DOWN)
			event->type = NSFB_EVENT_KEY_DOWN;
		else
			event->type = NSFB_EVENT_KEY_UP;

		Queue_Enqueue(InputQueue, (void*) event);
	}
	else if (flags & PTR_FLAGS_BUTTON3)
	{
		/* Middle Mouse Button */

		event = (nsfb_event_t*) malloc(sizeof(nsfb_event_t));
		ZeroMemory(event, sizeof(nsfb_event_t));

		event->value.keycode = NSFB_KEY_MOUSE_2;

		if (flags & PTR_FLAGS_DOWN)
			event->type = NSFB_EVENT_KEY_DOWN;
		else
			event->type = NSFB_EVENT_KEY_UP;

		Queue_Enqueue(InputQueue, (void*) event);
	}

	return 0;
}

static int freerds_client_extended_mouse_event(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	return 0;
}

static int freerds_service_accept(rdsService* service)
{
	fprintf(stderr, "%s\n", __FUNCTION__);
	connected = 1;
	return 0;
}

/**
 * NSFB Interface
 */

static int freerds_defaults(nsfb_t* nsfb)
{
	fprintf(stderr, "libnsfb_freerds_defaults\n");

	nsfb->width = 1024;
	nsfb->height = 768;
	nsfb->format = NSFB_FMT_ABGR8888;

	select_plotters(nsfb);

	return 0;
}

static int freerds_initialise(nsfb_t* nsfb)
{
	fprintf(stderr, "libnsfb_freerds_initialise\n");

	if (nsfb->ptr)
	{
		fprintf(stderr, "libnsfb_freerds_initialise: unexpected nsfb->ptr: %p\n", nsfb->ptr);
		return -1;
	}

	if (!InputQueue)
	{
		InputQueue = Queue_New(TRUE, 0, 0);
	}

	framebuffer.fbWidth = nsfb->width;
	framebuffer.fbHeight = nsfb->height;
	framebuffer.fbAttached = 0;
	framebuffer.fbBitsPerPixel = nsfb->bpp;
	framebuffer.fbBytesPerPixel = (nsfb->bpp / 8);
	framebuffer.fbScanline = nsfb->width * framebuffer.fbBytesPerPixel;
	framebuffer.image = NULL;

	framebufferSize = framebuffer.fbScanline * framebuffer.fbHeight;

	framebuffer.fbSegmentId = shmget(IPC_PRIVATE, framebufferSize,
			IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	framebuffer.fbSharedMemory = (BYTE*) shmat(framebuffer.fbSegmentId, 0, 0);

	nsfb->ptr = framebuffer.fbSharedMemory;
	nsfb->linelen = framebuffer.fbScanline;

	SessionId = 1;
	service = freerds_service_new(SessionId, endpoint);
	connector = (rdsModuleConnector*) service;

	fprintf(stderr, "freerds_service_new: %d service: %p\n", (int) SessionId, service);

	service->custom = (void*) nsfb;
	service->Accept = freerds_service_accept;

	connector->client->SynchronizeKeyboardEvent = freerds_client_synchronize_keyboard_event;
	connector->client->ScancodeKeyboardEvent = freerds_client_scancode_keyboard_event;
	connector->client->VirtualKeyboardEvent = freerds_client_virtual_keyboard_event;
	connector->client->UnicodeKeyboardEvent = freerds_client_unicode_keyboard_event;
	connector->client->MouseEvent = freerds_client_mouse_event;
	connector->client->ExtendedMouseEvent = freerds_client_extended_mouse_event;

	if (freerds_service_start(service) < 0)
	{
		fprintf(stderr, "failed to start FreeRDS service\n");
	}

	fprintf(stderr, "NetSurf FreeRDS service started\n");

	return 0;
}

static int freerds_finalise(nsfb_t* nsfb)
{
	fprintf(stderr, "libnsfb_freerds_finalise\n");

	free(nsfb->ptr);

	return 0;
}

static bool freerds_input(nsfb_t* nsfb, nsfb_event_t* event, int timeout)
{
	nsfb_event_t* input;

	input = (nsfb_event_t*) Queue_Dequeue(InputQueue);

	if (input)
	{
		CopyMemory(event, input, sizeof(nsfb_event_t));
		free(input);
		return true;
	}

	return false;
}

static int freerds_claim(nsfb_t* nsfb, nsfb_bbox_t* box)
{
	fprintf(stderr, "libnsfb_freerds_claim\n");

	return 0;
}

static int freerds_update(nsfb_t* nsfb, nsfb_bbox_t* box)
{
	INT32 x, y;
	UINT32 w, h;
	RDS_MSG_PAINT_RECT msg;

	fprintf(stderr, "libnsfb_freerds_update\n");

	freerds_check_shared_framebuffer();

	x = box->x0;
	y = box->y0;
	w = box->x1 - box->x0 + 1;
	h = box->y1 - box->y0 + 1;

	if (x < 0)
		x = 0;

	if (x > framebuffer.fbWidth - 1)
		x = framebuffer.fbWidth - 1;

	w += x % 16;
	x -= x % 16;

	w += w % 16;

	if (x + w > framebuffer.fbWidth)
		w = framebuffer.fbWidth - x;

	if (y < 0)
		y = 0;

	if (y > framebuffer.fbHeight - 1)
		y = framebuffer.fbHeight - 1;

	h += y % 16;
	y -= y % 16;

	h += h % 16;

	if (h > framebuffer.fbHeight)
		h = framebuffer.fbHeight;

	if (y + h > framebuffer.fbHeight)
		h = framebuffer.fbHeight - y;

	if (w * h < 1)
		return 0;

	msg.nLeftRect = x;
	msg.nTopRect = y;
	msg.nWidth = w;
	msg.nHeight = h;
	msg.nXSrc = 0;
	msg.nYSrc = 0;

	msg.fbSegmentId = framebuffer.fbSegmentId;
	msg.bitmapData = NULL;
	msg.bitmapDataLength = 0;

	fprintf(stderr, "libnsfb_freerds_update: x: %d y: %d width: %d height: %d\n", x, y, w, h);

	msg.type = RDS_SERVER_PAINT_RECT;
	connector->server->PaintRect(connector, &msg);

	return 0;
}

static int freerds_cursor(nsfb_t* nsfb, struct nsfb_cursor_s* cursor)
{
	fprintf(stderr, "libnsfb_freerds_cursor\n");

	return true;
}

static int freerds_set_geometry(nsfb_t* nsfb, int width, int height, enum nsfb_format_e format)
{
	int startsize; 
	int endsize;

	fprintf(stderr, "libnsfb_freerds_set_geometry\n");

	startsize = (nsfb->width * nsfb->height * nsfb->bpp) / 8;

	if (width > 0)
		nsfb->width = width;

	if (height > 0)
		nsfb->height = height;

	if (format != NSFB_FMT_ANY)
		nsfb->format = format;

	select_plotters(nsfb);

	endsize = (nsfb->width * nsfb->height * nsfb->bpp) / 8;

	if ((nsfb->ptr) && (startsize != endsize))
	{

	}

	nsfb->linelen = (nsfb->width * nsfb->bpp) / 8;

	return 0;
}

const nsfb_surface_rtns_t freerds_rtns =
{
	.defaults = freerds_defaults,
	.initialise = freerds_initialise,
	.finalise = freerds_finalise,
	.input = freerds_input,
	.claim = freerds_claim,
	.update = freerds_update,
	.cursor = freerds_cursor,
	.geometry = freerds_set_geometry,
};

NSFB_SURFACE_DEF(freerds, NSFB_SURFACE_FREERDS, &freerds_rtns)

/*
 * Local variables:
 *  c-basic-offset: 4
 *  tab-width: 8
 * End:
 */
