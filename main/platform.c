#include "platform.h"
#include "vmgraph.h"

/* Globals defined in main.c */
VMINT layer_hdl = -1;
VMUINT8* layer_buf = NULL;
VMINT screen_w = LOGICAL_W;
VMINT screen_h = LOGICAL_H;
VMINT xor_clipWidth = LOGICAL_W;
VMINT xor_clipHeight = LOGICAL_H - 30; /* screen_h - DASH_HEIGHT */
static VMUINT16 logical_buf[LOGICAL_W * LOGICAL_H];

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
    if (layer_hdl < 0 || !layer_buf) return;
    vm_graphic_rotate(
        (VMBYTE*)layer_buf,
        0, 0,
        (VMBYTE*)logical_buf,
        0,
        VM_ROTATE_DEGREE_270
    );
    vm_graphic_flush_layer(&layer_hdl, 1);
}

VMFILE plat_FileOpen(const VMWCHAR* path, int mode)
{
    //return vm_file_open(path, mode, VM_TRUE);
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
    //VMINT attr = vm_file_get_attributes(path);
    //return attr >= 0 ? 1 : 0;
    VMINT attr = vm_file_get_attributes((VMWSTR)path);
    return (attr >= 0);
}
