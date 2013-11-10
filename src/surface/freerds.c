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

#include <freerds/freerds.h>
#include <freerds/service_helper.h>

static int connected = 0;
static DWORD SessionId = 1;
static const char* endpoint = "NetSurf";

static int framebufferSize = 0;
static RDS_FRAMEBUFFER framebuffer = { 0 };

static rdsService* service = NULL;
static rdsModuleConnector* connector = NULL;

int freerds_check_shared_framebuffer(void)
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
	fprintf(stderr, "%s\n", __FUNCTION__);
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
	fprintf(stderr, "%s\n", __FUNCTION__);
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
	//fprintf(stderr, "libnsfb_freerds_input\n");

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

	fprintf(stderr, "libnsfb_freerds_geometry\n");

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
		nsfb->ptr = realloc(nsfb->ptr, endsize);

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
