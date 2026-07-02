#include "platform.h"

/* Globals defined in main.c */
VMINT layer_hdl = -1;
VMUINT8* layer_buf = NULL;
VMINT screen_w = 128;
VMINT screen_h = 160;
VMINT xor_clipWidth = 128;
VMINT xor_clipHeight = 130; /* screen_h - DASH_HEIGHT */

unsigned int plat_GetTicks(void)
{
    //return (unsigned int)vm_get_sys_time_ms();
    return (unsigned int)vm_get_tick_count();
}

void plat_Delay(unsigned int ms)
{
    unsigned int start = plat_GetTicks();
    while (plat_GetTicks() - start < ms);
}

void plat_SetPixel(int x, int y, VMUINT16 color)
{
    if (!layer_buf) return;
    if (x < 0 || y < 0 || x >= screen_w || y >= screen_h) return;
    ((VMUINT16*)layer_buf)[y * screen_w + x] = color;
}

VMUINT16 plat_GetPixel(int x, int y)
{
    if (!layer_buf) return PLT_COLOR_BLACK;
    if (x < 0 || y < 0 || x >= screen_w || y >= screen_h) return PLT_COLOR_BLACK;
    return ((VMUINT16*)layer_buf)[y * screen_w + x];
}

void plat_FillRect(int x, int y, int w, int h, VMUINT16 color)
{
    if (!layer_buf) return;
    int x1 = x + w;
    int y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > screen_w) x1 = screen_w;
    if (y1 > screen_h) y1 = screen_h;
    for (int yy = y; yy < y1; yy++)
    {
        VMUINT16* row = (VMUINT16*)layer_buf + yy * screen_w;
        for (int xx = x; xx < x1; xx++)
        {
            row[xx] = color;
        }
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

void plat_FlushScreen(void)
{
    if (layer_hdl < 0) return;
    vm_graphic_flush_layer(&layer_hdl, 1);
}

VMFILE plat_FileOpen(const VMWCHAR* path, int mode)
{
    return vm_file_open(path, mode, VM_TRUE);
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
    VMINT attr = vm_file_get_attributes(path);
    return attr >= 0 ? 1 : 0;
}
