#ifndef platform_h
#define platform_h

#include "vmsys.h"
#include "vmgraph.h"
#include "vmio.h"

#define LOGICAL_W  320
#define LOGICAL_H  240

/* Screen dimensions (runtime) */
extern VMINT screen_w, screen_h;

/* MRE layer handle and buffer */
extern VMINT layer_hdl;
extern VMUINT8* layer_buf;

/* Clip region dimensions (runtime: screen_w x (screen_h - DASH_HEIGHT)) */
extern VMINT xor_clipWidth, xor_clipHeight;

/* Timing */
unsigned int plat_GetTicks(void);   /* returns ms since start */
void plat_Delay(unsigned int ms);

/* Drawing - all draw to layer_buf (the active MRE layer) */
void plat_SetPixel(int x, int y, VMUINT16 color);
VMUINT16 plat_GetPixel(int x, int y);
void plat_FillRect(int x, int y, int w, int h, VMUINT16 color);
void plat_Line(int x0, int y0, int x1, int y1, VMUINT16 color);
void plat_FlushScreen(void);

/* File I/O */
VMFILE plat_FileOpen(const VMWCHAR* path, int mode);
int    plat_FileRead(VMFILE f, void* buf, int size);
int    plat_FileWrite(VMFILE f, const void* buf, int size);
void   plat_FileClose(VMFILE f);
int    plat_FileExists(const VMWCHAR* path);

/* Color constants (RGB565) */
#define PLT_COLOR_BLACK   ((VMUINT16)0x0000)
#define PLT_COLOR_WHITE   ((VMUINT16)0xFFFF)
#define PLT_COLOR_YELLOW  ((VMUINT16)0xFFE0)
#define PLT_COLOR_GREEN   ((VMUINT16)0x07E0)
#define PLT_COLOR_RED     ((VMUINT16)0xF800)

#endif /* platform_h */
