/*-----------------------------------------------------------------------------
 *
 *  Copyright (C) 2026 Jakob Jungreuthmayer
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * DESCRIPTION:
 *      A headless platform: the screen in memory, scripted input and a
 *      simulated clock. Prints a hash of every frame shown and can save one
 *      frame as a PPM picture.
 *
 *-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "i_platform.h"

#if !defined TEXTURE_SHIFT
#define TEXTURE_SHIFT 0
#endif

#if !defined SPRITE_SHIFT
#define SPRITE_SHIFT 0
#endif

#define COLEXTRABITS 7
#define COLBITS      9

#define DOOM_WIDTH   320
#define DOOM_HEIGHT  200

#define QUEUE        8


static uint8_t screen[SCREEN_HEIGHT][SCREEN_WIDTH];

static const uint8_t* palette;
static const uint8_t* pendingpalette;

static uint32_t now_ms;
static uint32_t start_ms;

static uint32_t zone_size = 262144;

static struct
{
	int16_t key;
	boolean down;
} queue[QUEUE];
static int queue_head;
static int queue_len;


//
// System
//

uint8_t* I_ZoneBase(uint32_t* heapSize)
{
	uint8_t* zone = calloc(zone_size + 16, 1);
	if (!zone)
		I_Error("I_ZoneBase: no memory for a %lu byte zone", (unsigned long)zone_size);

	*heapSize = zone_size;
	return zone + (16 - (uintptr_t)zone % 16) % 16;
}


static uint32_t clock_ms(void)
{
	return ++now_ms;
}


int32_t I_GetTime(void)
{
	return (int32_t)((uint64_t)(clock_ms() - start_ms) * TIC_RATE / 1000);
}


static void key(int16_t k, boolean down)
{
	if (queue_len < QUEUE)
	{
		queue[(queue_head + queue_len) % QUEUE].key  = k;
		queue[(queue_head + queue_len) % QUEUE].down = down;
		queue_len++;
	}
}


void I_StartTic(void)
{
	while (queue_len)
	{
		event_t ev = {0};
		ev.type  = queue[queue_head].down ? ev_keydown : ev_keyup;
		ev.data1 = queue[queue_head].key;
		queue_head = (queue_head + 1) % QUEUE;
		queue_len--;
		D_PostEvent(&ev);
	}
}


void I_Quit(void)
{
	exit(0);
}


void I_Log(const char* text)
{
	fputs(text, stdout);
}


void I_Fatal(const char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);

	exit(1);
}


#if defined W_FETCH
void W_Fetch(const void* src, void* dst, uint16_t len)
{
	memcpy(dst, src, len);
}
#endif


//
// Sound
//

void DMX_Play(sfxenum_t id)
{
	(void)id;
}


//
// Frame
//

void I_SetPalette(const uint8_t* pal)
{
	if (palette)
		pendingpalette = pal;
	else
		palette = pal;
}


void I_FinishUpdate(void)
{
	if (pendingpalette)
	{
		palette = pendingpalette;
		pendingpalette = NULL;
	}
}


void I_StartWipe(void)
{
}


void I_FinishWipe(void)
{
	I_FinishUpdate();
}


//
// 3D view, at the top left of the screen
//

static void view_pixel(int x, int y, uint8_t color)
{
	if (0 <= x && x < VIEW_WINDOW_WIDTH && 0 <= y && y < VIEW_WINDOW_HEIGHT)
		screen[y][x] = color;
}


static uint16_t first_frac(const draw_column_vars_t* dcvars)
{
	return (uint16_t)((dcvars->texturemid >> COLEXTRABITS) + (dcvars->yl - CENTER_Y) * (int32_t)dcvars->fracstep);
}


static void draw_column(const draw_column_vars_t* dcvars, int shift)
{
	int count = dcvars->yh - dcvars->yl + 1;
	if (count > SCREEN_HEIGHT)
		count = SCREEN_HEIGHT;

	uint16_t frac = first_frac(dcvars);
	for (int i = 0; i < count; i++)
	{
		view_pixel(dcvars->x, dcvars->yl + i, dcvars->colormap[dcvars->source[frac >> (COLBITS + shift)]]);
		frac += dcvars->fracstep;
	}
}


void R_DrawColumnWall(const draw_column_vars_t* dcvars)
{
	draw_column(dcvars, TEXTURE_SHIFT);
}


void R_DrawColumnSprite(const draw_column_vars_t* dcvars)
{
	draw_column(dcvars, SPRITE_SHIFT);
}


void R_DrawColumnFlat(uint8_t color, const draw_column_vars_t* dcvars)
{
	for (int y = dcvars->yl; y <= dcvars->yh && y < dcvars->yl + SCREEN_HEIGHT; y++)
		view_pixel(dcvars->x, y, color);
}


static const boolean fuzz[50] =
{
	1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1,
	1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1
};

static int fuzzpos;


void R_DrawFuzzColumn(const draw_column_vars_t* dcvars)
{
	int yl = dcvars->yl < 1 ? 1 : dcvars->yl;
	int yh = dcvars->yh > VIEW_WINDOW_HEIGHT - 2 ? VIEW_WINDOW_HEIGHT - 2 : dcvars->yh;
	const uint8_t* shade = fullcolormap + 6 * 256;

	uint16_t frac = first_frac(dcvars) + (yl - dcvars->yl) * dcvars->fracstep;
	for (int y = yl; y <= yh; y++)
	{
		if (fuzz[fuzzpos])
			view_pixel(dcvars->x, y, shade[dcvars->source[frac >> (COLBITS + SPRITE_SHIFT)]]);

		frac += dcvars->fracstep;
		fuzzpos = (fuzzpos + 1) % 50;
	}
}


void V_ClearViewWindow(void)
{
	for (int y = 0; y < VIEW_WINDOW_HEIGHT; y++)
		memset(screen[y], 0, VIEW_WINDOW_WIDTH);
}


//
// Automap
//

void V_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
	int dx  =  abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy  = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	for (;;)
	{
		view_pixel(x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;
		if (e2 >= dy)
		{
			err += dy;
			x0  += sx;
		}
		if (e2 <= dx)
		{
			err += dx;
			y0  += sy;
		}
	}
}


//
// Pictures
//

static void screen_pixel(int x, int y, uint8_t color)
{
	if (0 <= x && x < SCREEN_WIDTH && 0 <= y && y < SCREEN_HEIGHT)
		screen[y][x] = color;
}


static const uint8_t* patch_column(const patch_t* patch, int column)
{
	return (const uint8_t*)patch + (uint16_t)patch->columnofs[column];
}


void V_DrawPatchNotScaled(int16_t x, int16_t y, const patch_t* patch)
{
	x -= patch->leftoffset;
	y -= patch->topoffset;

	for (int column = 0; column < patch->width; column++)
	{
		for (const uint8_t* post = patch_column(patch, column); post[0] != 0xff; post += post[1] + 4)
		{
			for (int i = 0; i < post[1]; i++)
				screen_pixel(x + column, y + post[0] + i, post[3 + i]);
		}
	}
}


#define DX  ((SCREEN_WIDTH << FRACBITS) / DOOM_WIDTH)
#define DXI ((uint16_t)(((DOOM_WIDTH << FRACBITS) / SCREEN_WIDTH) >> 8))
#define DY  (((SCREEN_HEIGHT << FRACBITS) + (1 << FRACBITS) - 1) / DOOM_HEIGHT)
#define DYI ((uint16_t)(((DOOM_HEIGHT << FRACBITS) / SCREEN_HEIGHT) >> 8))


void V_DrawPatchScaled(int16_t x0, int16_t y0, const patch_t* patch)
{
	int x = x0 - patch->leftoffset;
	int y = y0 - patch->topoffset;

	int left   = (x * DX) >> FRACBITS;
	int right  = ((x + patch->width) * DX) >> FRACBITS;
	int bottom = ((y + patch->height) * DY) >> FRACBITS;

	uint16_t col = 0;
	for (int dc_x = left; dc_x < right; dc_x++, col += DXI)
	{
		if (dc_x < 0)
			continue;
		if (dc_x >= SCREEN_WIDTH)
			break;

		for (const uint8_t* post = patch_column(patch, col >> 8); post[0] != 0xff; post += post[1] + 4)
		{
			int dc_yl = ((y + post[0]) * DY) >> FRACBITS;
			if (dc_yl >= SCREEN_HEIGHT || dc_yl > bottom)
				break;

			int dc_yh = ((y + post[0] + post[1]) * DY) >> FRACBITS;
			uint16_t frac = 0;
			for (int dc_y = dc_yl; dc_y < dc_yh && dc_y < dc_yl + SCREEN_HEIGHT; dc_y++, frac += DYI)
				screen_pixel(dc_x, dc_y, post[3 + (frac >> 8)]);
		}
	}
}


void V_DrawBackground(const uint8_t* tile)
{
	for (int y = 0; y < SCREEN_HEIGHT; y++)
		for (int x = 0; x < SCREEN_WIDTH; x++)
			screen[y][x] = tile[(y & 63) * 64 + (x & 63)];
}


void V_DrawRaw(uint32_t offset, const uint8_t* pixels, uint32_t length)
{
	if (offset < sizeof(screen))
		memcpy(&screen[0][0] + offset, pixels, length < sizeof(screen) - offset ? length : sizeof(screen) - offset);
}


void V_DrawRawRect(const uint8_t* pixels, int16_t x, int16_t y, int16_t w, int16_t h)
{
	for (int dy = 0; dy < h; dy++)
		for (int dx = 0; dx < w; dx++)
			screen_pixel(x + dx, y + dy, pixels[dy * SCREEN_WIDTH + dx]);
}


//
// The host
//

static uint64_t fnv(uint64_t hash, const uint8_t* rgb, size_t len)
{
	for (size_t i = 0; i < len; i++)
		hash = (hash ^ rgb[i]) * 0x100000001b3ULL;
	return hash;
}


static uint64_t hash_frame(uint64_t hash, const uint8_t* pal)
{
	static uint8_t rgb[SCREEN_HEIGHT * SCREEN_WIDTH * 3];

	for (size_t i = 0; i < sizeof(screen); i++)
		memcpy(&rgb[i * 3], &pal[(&screen[0][0])[i] * 3], 3);

	return fnv(hash, rgb, sizeof(rgb));
}


static void save_ppm(const char* path, const uint8_t* pal)
{
	FILE* file = fopen(path, "wb");
	if (!file)
		I_Error("cannot write %s", path);

	fprintf(file, "P6 %d %d 255\n", SCREEN_WIDTH, SCREEN_HEIGHT);
	for (size_t i = 0; i < sizeof(screen); i++)
		fwrite(&pal[(&screen[0][0])[i] * 3], 1, 3, file);
	fclose(file);
}


static const uint8_t* load(const char* path)
{
	FILE* file = fopen(path, "rb");
	if (!file)
		I_Error("cannot read %s", path);

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* data = malloc(size);
	if (!data || fread(data, 1, size, file) != (size_t)size)
		I_Error("cannot read %s", path);

	fclose(file);
	return data;
}


// Starts a game, turns right now and then, fires and opens doors; shows the automap.
static void script(uint32_t f, int skill_down, boolean automap)
{
	if (f == 50 || f == 70 || f == 90)
		key(KEYD_A, true);
	if (f == 51 || f == 71 || f == 91)
		key(KEYD_A, false);
	for (int n = 0; n < skill_down; n++)
	{
		if (f == 80u + 2 * n)
			key(KEYD_DOWN, true);
		if (f == 81u + 2 * n)
			key(KEYD_DOWN, false);
	}
	if (f == 150)
		key(KEYD_UP, true);
	if (f > 150)
	{
		if (f % 80 == 0)
			key(KEYD_RIGHT, true);
		if (f % 80 == 12)
			key(KEYD_RIGHT, false);
		if (f % 30 == 0)
			key(KEYD_B, true);
		if (f % 30 == 3)
			key(KEYD_B, false);
		if (f % 400 == 200)
			key(KEYD_A, true);
		if (f % 400 == 201)
			key(KEYD_A, false);
	}
	if (automap && f == 400)
		key(KEYD_SELECT, true);
	if (automap && f == 401)
		key(KEYD_SELECT, false);
}


static void usage(void)
{
	fprintf(stderr,
		"usage: host GRAPHICS.WAD MAPS.WAD [-frames N] [-skill N] [-demo] [-automap]\n"
		"            [-zone BYTES] [-shot FRAME FILE.ppm]\n");
	exit(2);
}


int main(int argc, char** argv)
{
	uint32_t frames = 1000;
	int skill_down = 0;
	boolean demo = false;
	boolean automap = false;
	long shot = -1;
	const char* shot_file = NULL;

	if (argc < 3)
		usage();

	for (int i = 3; i < argc; i++)
	{
		if (!strcmp(argv[i], "-frames") && i + 1 < argc)
			frames = strtoul(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "-skill") && i + 1 < argc)
			skill_down = atoi(argv[++i]);
		else if (!strcmp(argv[i], "-demo"))
			demo = true;
		else if (!strcmp(argv[i], "-automap"))
			automap = true;
		else if (!strcmp(argv[i], "-zone") && i + 1 < argc)
			zone_size = strtoul(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "-shot") && i + 2 < argc)
		{
			shot = atol(argv[++i]);
			shot_file = argv[++i];
		}
		else
			usage();
	}

	const char* const engine_argv[] = {"femto-doom"};

	start_ms = clock_ms();
	W_SetWads(load(argv[1]), load(argv[2]));
	D_DoomInit(1, engine_argv);

	uint64_t hash = 0xcbf29ce484222325ULL;
	for (uint32_t f = 0; f < frames; f++)
	{
		if (!demo)
			script(f, skill_down, automap);

		const uint8_t* shown = palette;
		D_DoomStep();

		hash = hash_frame(hash, shown);
		if ((long)f == shot)
			save_ppm(shot_file, shown);
	}

	printf("frames %lu hash %016llx\n", (unsigned long)frames, (unsigned long long)hash);
	return 0;
}
