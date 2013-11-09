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

static DWORD SessionId = 10;
static const char* endpoint = "NS";

static rdsService* service = NULL;
static rdsModuleConnector* connector = NULL;

/**
 * FreeRDS Interface
 */

static int freerds_client_synchronize_keyboard_event(rdsModuleConnector* connector, DWORD flags)
{
	return 0;
}

static int freerds_client_scancode_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code, DWORD keyboardType)
{
	return 0;
}

static int freerds_client_virtual_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code)
{
	return 0;
}

static int freerds_client_unicode_keyboard_event(rdsModuleConnector* connector, DWORD flags, DWORD code)
{
	return 0;
}

static int freerds_client_mouse_event(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y)
{
	return 0;
}

static int freerds_client_extended_mouse_event(rdsModuleConnector* connector, DWORD flags, DWORD x, DWORD y)
{
	return 0;
}

static int freerds_service_accept(rdsService* service)
{
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
	size_t size;

	fprintf(stderr, "libnsfb_freerds_initialise\n");

	size = (nsfb->width * nsfb->height * nsfb->bpp) / 8;

	nsfb->ptr = realloc(nsfb->ptr, size);
	nsfb->linelen = (nsfb->width * nsfb->bpp) / 8;

	service = freerds_service_new(SessionId, endpoint);
	connector = (rdsModuleConnector*) service;

	service->custom = (void*) nsfb;
	service->Accept = freerds_service_accept;

	connector->client->SynchronizeKeyboardEvent = freerds_client_synchronize_keyboard_event;
	connector->client->ScancodeKeyboardEvent = freerds_client_scancode_keyboard_event;
	connector->client->VirtualKeyboardEvent = freerds_client_virtual_keyboard_event;
	connector->client->UnicodeKeyboardEvent = freerds_client_unicode_keyboard_event;
	connector->client->MouseEvent = freerds_client_mouse_event;
	connector->client->ExtendedMouseEvent = freerds_client_extended_mouse_event;

	freerds_service_start(service);

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
	fprintf(stderr, "libnsfb_freerds_update\n");

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
