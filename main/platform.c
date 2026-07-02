#include "platform.h"

/* Globals defined in main.c */
VMINT layer_hdl = -1;
VMUINT8* layer_buf = NULL;
VMINT screen_w = LOGICAL_W;
VMINT screen_h = LOGICAL_H;
VMINT xor_clipWidth = LOGICAL_W;
VMINT xor_clipHeight = LOGICAL_H - 30; /* screen_h - DASH_HEIGHT */

/* Logical landscape framebuffer: game always draws into this 320x240 buffer.
   plat_FlushScreen() rotates it into the 240x320 physical layer_buf. */
static VMUINT16 logical_buf[LOGICAL_W * LOGICAL_H];

unsigned int plat_GetTicks(void)
{
    return (unsigned int)vm_get_tick_count();
}

void plat_Delay(unsigned int ms)
{
    unsigned int start = plat_GetTicks();
    while (plat_GetTicks() - start < ms);
}

void plat_SetPixel(int x, int y, VMUINT16 color)
{
    if (x < 0 || y < 0 || x >= LOGICAL_W || y >= LOGICAL_H) return;
    logical_buf[y * LOGICAL_W + x] = color;
}

VMUINT16 plat_GetPixel(int x, int y)
{
    if (x < 0 || y < 0 || x >= LOGICAL_W || y >= LOGICAL_H) return PLT_COLOR_BLACK;
    return logical_buf[y * LOGICAL_W + x];
}

void plat_FillRect(int x, int y, int w, int h, VMUINT16 color)
{
    int x1 = x + w;
    int y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > LOGICAL_W) x1 = LOGICAL_W;
    if (y1 > LOGICAL_H) y1 = LOGICAL_H;
    for (int yy = y; yy < y1; yy++)
    {
        VMUINT16* row = logical_buf + yy * LOGICAL_W;
        for (int xx = x; xx < x1; xx++)
            row[xx] = color;
    }
}

/* Bresenham line draw */
void plat_Line(int x0, int y0, int x1, int y1, VMUINT16 color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx > 0 ? 1 : -1;
    int sy = dy > 0 ? 1 : -1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int err = dx - dy;
    while (1)
    {
        plat_SetPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

/*
 * plat_FlushScreen — rotate logical_buf (320x240 landscape) into the
 * physical layer_buf (240x320 portrait) using a 270-degree clockwise
 * rotation, then flush to the display.
 *
 * Mapping (270° CW):  logical(lx, ly)  →  physical(ly, LOGICAL_W-1-lx)
 * Physical layout: phys_w = LOGICAL_H = 240,  phys_h = LOGICAL_W = 320
 * physical index = physical_y * phys_w + physical_x
 *                = (LOGICAL_W-1-lx) * LOGICAL_H + ly
 *
 * vm_graphic_rotate() is NOT used here because it expects an MRE image
 * resource buffer (with frame header), not a raw pixel array.
 */
void plat_FlushScreen(void)
{
    if (layer_hdl < 0 || !layer_buf) return;

    VMUINT16* phys = (VMUINT16*)layer_buf;
    for (int ly = 0; ly < LOGICAL_H; ly++)
    {
        const VMUINT16* src = logical_buf + ly * LOGICAL_W;
        int phys_x = LOGICAL_H - 1 - ly; /* physical x column */
        for (int lx = 0; lx < LOGICAL_W; lx++)
        {
            phys[lx * LOGICAL_H + phys_x] = src[lx];
        }
    }

    vm_graphic_flush_layer(&layer_hdl, 1);
}

VMFILE plat_FileOpen(const VMWCHAR* path, int mode)
{
    return vm_file_open((VMWSTR)path, mode, VM_TRUE);
}

int plat_FileRead(VMFILE f, void* buf, int size)
{
    VMUINT read;
    if (vm_file_read(f, buf, (VMUINT)size, &read) <= 0)
        return -1;
    return (int)read;
}

int plat_FileWrite(VMFILE f, const void* buf, int size)
{
    VMUINT written;
    if (vm_file_write(f, (void*)buf, (VMUINT)size, &written) <= 0)
        return -1;
    return (int)written;
}

void plat_FileClose(VMFILE f)
{
    vm_file_close(f);
}

int plat_FileExists(const VMWCHAR* path)
{
    VMINT attr = vm_file_get_attributes((VMWSTR)path);
    return (attr >= 0);
}
