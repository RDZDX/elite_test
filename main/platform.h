#ifndef platform_h
#define platform_h

#include "vmsys.h"
#include "vmgraph.h"
#include "vmio.h"

/* Logical game coordinate space (landscape 320x240, matching original TI-84 CE) */
#define LOGICAL_W  320
#define LOGICAL_H  240

/* Physical MRE screen (Nokia 225 portrait 240x320).
   The game draws into a 320x240 logical_buf; plat_FlushScreen() rotates
   it 270 degrees CW into the physical 240x320 layer_buf. */
extern VMINT screen_w, screen_h; /* physical: 240, 320 */

/* MRE layer handle and buffer (physical portrait layer) */
extern VMINT    layer_hdl;
extern VMUINT8* layer_buf;

/* Clip region (logical space, above dashboard) */
extern VMINT xor_clipWidth, xor_clipHeight;

/* Timing */
unsigned int plat_GetTicks(void);   /* ms since power-on */
void         plat_Delay(unsigned int ms);

/* Drawing — all coordinates are in logical 320x240 space */
void     plat_SetPixel(int x, int y, VMUINT16 color);
VMUINT16 plat_GetPixel(int x, int y);
void     plat_FillRect(int x, int y, int w, int h, VMUINT16 color);
void     plat_Line(int x0, int y0, int x1, int y1, VMUINT16 color);
void     plat_FlushScreen(void);  /* rotates logical_buf -> layer_buf, then flushes */

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
