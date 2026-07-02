#include <stdlib.h>
#include <stdbool.h>

#include "xorgfx.h"
#include "intmath.h"
#include "trig.h"
#include "font_data.h"
#include "variables.h"
#include "platform.h"

unsigned char xor_cursorX = 0;
unsigned char xor_cursorY = 0;

bool xor_lineSpacing = false;

/* XOR a pixel within the clip region using the platform layer */
static inline void xor_pixel_internal(int x, int y)
{
    if (x < xor_clipX || y < xor_clipY) return;
    if (x >= xor_clipX + xor_clipWidth) return;
    if (y >= xor_clipY + xor_clipHeight) return;
    VMUINT16 cur = plat_GetPixel(x, y);
    plat_SetPixel(x, y, cur ^ PLT_COLOR_WHITE);
}

bool xor_InClipRegion(signed int x, signed int y)
{
    if (x < xor_clipX) return 0;
    if (y < xor_clipY) return 0;
    if (x >= xor_clipX + xor_clipWidth) return 0;
    if (y >= xor_clipY + xor_clipHeight) return 0;
    return 1;
}

void xor_PointNoClip(unsigned int x, unsigned char y)
{
    VMUINT16 cur = plat_GetPixel((int)x, (int)y);
    plat_SetPixel((int)x, (int)y, cur ^ PLT_COLOR_WHITE);
}

void xor_Point(unsigned int x, unsigned char y)
{
    if (x < (unsigned int)xor_clipX) return;
    if (y < (unsigned char)xor_clipY) return;
    if (x >= (unsigned int)(xor_clipX + xor_clipWidth)) return;
    if (y >= (unsigned char)(xor_clipY + xor_clipHeight)) return;

    VMUINT16 cur = plat_GetPixel((int)x, (int)y);
    plat_SetPixel((int)x, (int)y, cur ^ PLT_COLOR_WHITE);
}

void xor_VerticalLine(unsigned int x, unsigned char y0, unsigned char y1)
{
    if (y0 > y1)
    {
        xor_VerticalLine(x, y1, y0);
        return;
    }
    for (unsigned char y = y0; y < y1; y++) xor_PointNoClip(x, y);
}

void xor_HorizontalLine(unsigned char y, unsigned int x0, unsigned int x1)
{
    if (x0 > x1)
    {
        xor_HorizontalLine(y, x1, x0);
        return;
    }
    for (unsigned int x = x0; x < x1; x++) xor_PointNoClip(x, y);
}

static void xor_LineLow(unsigned int x0, unsigned char y0, unsigned int x1, unsigned char y1)
{
    const int dx = x1 - x0;
    int dy = y1 - y0;
    char yi = 1;
    if (dy < 0) { yi = -1; dy = -dy; }

    int D = 2 * dy - dx;
    unsigned char y = y0;
    for (unsigned int x = x0; x < x1 && x < (unsigned int)(xor_clipX + xor_clipWidth); x++)
    {
        xor_PointNoClip(x, y);
        if (D > 0) { y += yi; D += 2 * (dy - dx); }
        else D += 2 * dy;
    }
}

static void xor_LineHigh(unsigned int x0, unsigned char y0, unsigned int x1, unsigned char y1)
{
    int dx = x1 - x0;
    const int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) { xi = -1; dx = -dx; }

    int D = 2 * dx - dy;
    unsigned int x = x0;
    for (unsigned char y = y0; y < y1 && y < (unsigned char)(xor_clipY + xor_clipHeight); y++)
    {
        xor_PointNoClip(x, y);
        if (D > 0) { x += xi; D += 2 * (dx - dy); }
        else D += 2 * dx;
    }
}

void xor_LineNoClip(unsigned int x0, unsigned char y0, unsigned int x1, unsigned char y1)
{
    if (x0 == x1) { xor_VerticalLine(x0, y0, y1); return; }
    if (y0 == y1) { xor_HorizontalLine(y0, x0, x1); return; }

    unsigned char dx = (x1 > x0) ? (unsigned char)(x1 - x0) : (unsigned char)(x0 - x1);
    unsigned char dy = (y1 > y0) ? (unsigned char)(y1 - y0) : (unsigned char)(y0 - y1);

    if (dy < dx)
    {
        if (x0 > x1) xor_LineLow(x1, y1, x0, y0);
        else xor_LineLow(x0, y0, x1, y1);
    }
    else
    {
        if (y0 > y1) xor_LineHigh(x1, y1, x0, y0);
        else xor_LineHigh(x0, y0, x1, y1);
    }
}

void xor_Line(signed int x0, signed int y0, signed int x1, signed int y1)
{
    signed int const xor_clipX1 = xor_clipX + xor_clipWidth;
    signed int const xor_clipY1 = xor_clipY + xor_clipHeight;

    const signed int dx = (x1 - x0);
    const signed int dy = (y1 - y0);

    if (x0 - xor_clipWidth >= xor_clipX1 && x1 - xor_clipWidth >= xor_clipX1) return;
    if (x0 + xor_clipWidth < xor_clipX && x1 + xor_clipWidth < xor_clipX) return;
    if (y0 - xor_clipHeight >= xor_clipY1 && y1 - xor_clipHeight >= xor_clipY1) return;
    if (y0 + xor_clipHeight < xor_clipY && y1 + xor_clipHeight < xor_clipY) return;

    if (x0 < xor_clipX)
    {
        if (x1 < xor_clipX) return;
        y0 += (xor_clipX - x0) * dy / dx;
        x0 = xor_clipX;
    }
    else if (x0 >= xor_clipX1)
    {
        if (x1 >= xor_clipX1) return;
        y0 -= (x0 - xor_clipX1) * dy / dx;
        x0 = xor_clipX + xor_clipWidth - 1;
    }

    if (y0 < xor_clipY)
    {
        if (y1 < xor_clipY) return;
        x0 += (xor_clipY - y0) * dx / dy;
        y0 = xor_clipY;
    }
    else if (y0 >= xor_clipY1)
    {
        if (y1 >= xor_clipY1) return;
        x0 -= (y0 - xor_clipY1) * dx / dy;
        y0 = xor_clipY + xor_clipHeight - 1;
    }

    if (x1 >= xor_clipX && x1 < xor_clipX1 && y1 >= xor_clipY && y1 < xor_clipY1)
    {
        xor_LineNoClip(x0, y0, x1, y1);
    }
    else
    {
        xor_Line(x1, y1, x0, y0);
    }
}

void xor_Crosshair(unsigned int x, unsigned char y, unsigned char spread, unsigned char size)
{
    unsigned char const end = spread + size;
    for (unsigned char i = spread; i < end; i++)
    {
        xor_Point(x,     y + i);
        xor_Point(x,     y - i);
        xor_Point(x + i, y);
        xor_Point(x - i, y);
    }
}

void xor_Rectangle(unsigned int x, unsigned char y, unsigned int width, unsigned char height)
{
    xor_HorizontalLine(y,          x, x + width);
    xor_HorizontalLine(y + height, x, x + width);
    xor_VerticalLine(x,         y + 1, y + height - 1);
    xor_VerticalLine(x + width, y + 1, y + height - 1);
}

void xor_FillRectangle(signed int x, signed int y, unsigned int width, unsigned char height)
{
    if (x < xor_clipX)
    {
        if ((signed int)(x + width) < (signed int)xor_clipX) return;
        xor_FillRectangle(xor_clipX, y, width - (xor_clipX - x), height);
        return;
    }
    if (x + (signed int)width >= xor_clipX + xor_clipWidth)
    {
        if (x >= xor_clipX + xor_clipWidth) return;
        xor_FillRectangle(x, y, xor_clipX + xor_clipWidth - x - 1, height);
        return;
    }
    if (y < xor_clipY)
    {
        if (y + height < (unsigned char)xor_clipY) return;
        xor_FillRectangle(x, xor_clipY, width, height - (xor_clipY - y));
        return;
    }
    if (y + height >= xor_clipY + xor_clipHeight)
    {
        if (y >= xor_clipY + xor_clipHeight) return;
        xor_FillRectangle(x, y, width, xor_clipY + xor_clipHeight - y - 1);
        return;
    }

    for (signed int yy = y; yy < y + (signed int)height; yy++)
        xor_HorizontalLine((unsigned char)yy, (unsigned int)x, (unsigned int)(x + width - 1));
}

void xor_Circle(signed int cX, signed int cY, unsigned int r)
{
    if (r < 8) xor_SteppedCircle(cX, cY, r, 8);
    else if (r < 60) xor_SteppedCircle(cX, cY, r, 4);
    else xor_SteppedCircle(cX, cY, r, 2);
}

void xor_SteppedCircle(signed int cX, signed int cY, unsigned int r, unsigned char step)
{
    signed int lastX = cX + r - 1;
    signed int lastY = cY;

    for (unsigned char i = 0; i < 64; i += step)
    {
        const signed int newX = cX + trig_cos(i) * (signed int)r / 256;
        const signed int newY = cY - trig_sin(i) * (signed int)r / 256;

        xor_Line(lastX, lastY, newX, newY);

        lastX = newX;
        lastY = newY;
    }

    xor_Line(lastX, lastY, cX + r - 1, cY);
    xor_Point(cX + trig_cos(24) * (signed int)r / 256,
              cY - trig_sin(24) * (signed int)r / 256);
    xor_Point(cX + trig_cos(56) * (signed int)r / 256,
              cY - trig_sin(56) * (signed int)r / 256);
}

static void xor_RaggedLine(const signed int cX, const signed int cY,
                            const signed int thisRadius, const unsigned char fringeSize)
{
    if (cY < xor_clipY) return;
    if (cY >= xor_clipY + xor_clipHeight) return;

    const signed int thisModRadius = thisRadius + (fringeSize > 0 ? rand() % fringeSize : 0);

    signed int thisLeft = cX - thisModRadius + 1;
    if (thisLeft >= xor_clipX + xor_clipWidth) return;
    if (thisLeft < xor_clipX) thisLeft = xor_clipX;

    signed int thisRight = cX + thisModRadius;
    if (thisRight < xor_clipX) return;
    if (thisRight >= xor_clipX + xor_clipWidth) thisRight = xor_clipX + xor_clipWidth - 1;

    xor_HorizontalLine((unsigned char)cY, (unsigned int)thisLeft, (unsigned int)thisRight);
}

void xor_FillCircle(signed int cX, signed int cY, unsigned char r)
{
    const unsigned char fringeSize = r >= 96 ? 8
                                   : r >= 40 ? 4
                                   : r >= 16 ? 2
                                   : 0;

    signed int x = r;
    signed int y = 0;
    signed int P = 1 - r;

    while (x > y)
    {
        y++;
        if (P > 0)
        {
            x--;
            P -= 2 * x;
            if (x < y) break;
            xor_RaggedLine(cX, cY + x, y, fringeSize);
            xor_RaggedLine(cX, cY - x, y, fringeSize);
        }
        if (x <= y) break;
        P += 2 * y + 1;
        xor_RaggedLine(cX, cY + y, x, fringeSize);
        xor_RaggedLine(cX, cY - y, x, fringeSize);
    }

    xor_RaggedLine(cX, cY, r, fringeSize);
}

void xor_Ellipse(signed int cX, signed int cY,
                 signed int uX, signed int uY,
                 signed int vX, signed int vY,
                 unsigned char end)
{
    unsigned char const stepTest = intabs(uX) | intabs(uY) | intabs(vX) | intabs(vY);
    unsigned char const step = stepTest < 8 ? 8
                             : stepTest < 60 ? 4
                             : 2;

    signed int lastX = cX + uX * 255 / 256;
    signed int lastY = cY + uY * 255 / 256;

    for (unsigned char i = 0; i < end; i += step)
    {
        const signed int newX = cX + uX * trig_cos(i) / 256 + vX * trig_sin(i) / 256;
        const signed int newY = cY + uY * trig_cos(i) / 256 + vY * trig_sin(i) / 256;
        xor_Line(lastX, lastY, newX, newY);
        lastX = newX;
        lastY = newY;
    }

    xor_Line(lastX, lastY, cX + uX * 255 / 256, cY + uY * 255 / 256);
}

static void xor_Char(unsigned int x, unsigned char y, char toPrint)
{
    const unsigned char* charData = font_data + (toPrint - 32) * 8;
    for (unsigned char yy = 0; yy < 8; yy++)
    {
        for (unsigned char xx = 0; xx < 8; xx++)
        {
            if ((charData[yy] & (0x80 >> xx)) == 0) continue;
            xor_PointNoClip(x + xx, y + yy);
        }
    }
}

void xor_CenterText(char toPrint[], unsigned char length, unsigned char y)
{
    for (unsigned int x = 0; x < length; x++)
        xor_Char(VIEW_HCENTER - 4 * length + 8 * x, y, toPrint[x]);
}

void xor_CenterTextOffset(const char toPrint[], unsigned char length, unsigned char y, signed char offset)
{
    for (unsigned int x = 0; x < length; x++)
        xor_Char(VIEW_HCENTER - 4 * length - 4 * offset + 8 * x, y, toPrint[x]);
}

void xor_SetCursorPos(unsigned char x, unsigned char y)
{
    xor_cursorX = x;
    xor_cursorY = y;
}

static void xor_CRLF(void)
{
    xor_cursorY++;
    if (xor_lineSpacing) xor_cursorY++;
    xor_cursorX = 0;
}

void xor_PrintChar(char toPrint)
{
    if (toPrint == '\n')
    {
        xor_CRLF();
        return;
    }
    xor_Char(xor_cursorX * 8 + xor_clipX + 8, xor_cursorY * 8 + xor_clipY - 2, toPrint);
    xor_cursorX++;
    if (xor_cursorX >= (unsigned char)xor_textCols) xor_CRLF();
}

void xor_Print(char const str[])
{
    for (unsigned char i = 0; str[i] != '\0'; i++) xor_PrintChar(str[i]);
}

void xor_PrintUInt8(unsigned char toPrint, unsigned char maxLength)
{
    unsigned char dividend = toPrint;
    bool leading = true;
    for (unsigned char divisor = (unsigned char)intpow(10, maxLength - 1); divisor >= 1; divisor /= 10)
    {
        char nextChar = '0' + dividend / divisor;
        if (nextChar == '0' && leading && divisor > 1) nextChar = ' ';
        else leading = false;
        xor_PrintChar(nextChar);
        dividend %= divisor;
    }
}

void xor_PrintUInt8Tenths(unsigned char toPrint, unsigned char maxLength)
{
    xor_PrintUInt8(toPrint / 10, maxLength);
    xor_PrintChar('.');
    xor_PrintUInt8(toPrint % 10, 1);
}

void xor_PrintUInt24(unsigned int toPrint, unsigned char maxLength)
{
    unsigned int dividend = toPrint;
    bool leading = true;
    for (unsigned int divisor = intpow(10, maxLength - 1); divisor >= 1; divisor /= 10)
    {
        char nextChar = '0' + (char)(dividend / divisor);
        if (nextChar == '0' && leading && divisor > 1) nextChar = ' ';
        else leading = false;
        xor_PrintChar(nextChar);
        dividend %= divisor;
    }
}

void xor_PrintUInt24Adaptive(unsigned int toPrint)
{
    if (toPrint == 0)
    {
        xor_PrintChar('0');
    }
    else
    {
        unsigned char trialLength;
        for (trialLength = 8; intpow(10, trialLength - 1) > toPrint; trialLength--);
        xor_PrintUInt24(toPrint, trialLength);
    }
}

void xor_PrintUInt24Tenths(unsigned int toPrint)
{
    xor_PrintUInt24Adaptive(toPrint / 10);
    xor_PrintChar('.');
    xor_PrintUInt8(toPrint % 10, 1);
}
