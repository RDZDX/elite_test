#include "vmsys.h"
#include "vmgraph.h"
#include "input.h"
#include "vmio.h"

/* Key state counters: 0 = not pressed, N = held for N frames */
unsigned char yequ    = 0;
unsigned char graph   = 0;
unsigned char up      = 0;
unsigned char down    = 0;
unsigned char left    = 0;
unsigned char right   = 0;
unsigned char enter   = 0;
unsigned char math    = 0;
unsigned char prgm    = 0;
unsigned char vars    = 0;
unsigned char clear   = 0;
unsigned char mode    = 0;
unsigned char del     = 0;
unsigned char graphVar = 0;
unsigned char stat    = 0;
unsigned char apps    = 0;
unsigned char alpha   = 0;
unsigned char second  = 0;

/* Raw key-down state (boolean array indexed by a small key-id) */
#define NUM_KEYS 18
typedef enum {
    KID_UP = 0,
    KID_DOWN,
    KID_LEFT,
    KID_RIGHT,
    KID_ENTER,
    KID_GRAPH,
    KID_YEQU,
    KID_CLEAR,
    KID_MODE,
    KID_SECOND,
    KID_ALPHA,
    KID_PRGM,
    KID_VARS,
    KID_APPS,
    KID_GRAPHVAR,
    KID_MATH,
    KID_DEL,
    KID_STAT
} key_id_t;

static unsigned char key_down[NUM_KEYS] = {0};
static unsigned char* const key_counter[NUM_KEYS] = {
    &up, &down, &left, &right, &enter, &graph, &yequ, &clear,
    &mode, &second, &alpha, &prgm, &vars, &apps, &graphVar, &math,
    &del, &stat
};

/* Map an MRE keycode to a key_id_t, or return -1 if not mapped */
static int mapKeycode(VMINT keycode)
{
    switch (keycode)
    {
        case VM_KEY_UP:              return KID_LEFT;  // keep your remap
        case VM_KEY_DOWN:            return KID_RIGHT;
        case VM_KEY_LEFT:            return KID_DOWN;
        case VM_KEY_RIGHT:           return KID_UP;
        case VM_KEY_OK:              return KID_ENTER;
        case VM_KEY_LEFT_SOFTKEY:    return KID_GRAPH;
        case VM_KEY_RIGHT_SOFTKEY:   return KID_YEQU;
        case VM_KEY_STAR:            return KID_CLEAR;
        case VM_KEY_NUM0:            return KID_MODE;
        case VM_KEY_POUND:           return KID_SECOND;
        case VM_KEY_NUM1:            return KID_ALPHA;
        case VM_KEY_NUM2:            return KID_PRGM;
        case VM_KEY_NUM3:            return KID_VARS;
        case VM_KEY_NUM4:            return KID_APPS;
        case VM_KEY_NUM5:            return KID_GRAPHVAR;
        case VM_KEY_NUM6:            return KID_MATH;
        case VM_KEY_NUM7:            return KID_DEL;
        case VM_KEY_NUM8:            return KID_STAT;
        default:                     return -1;
    }
}

void input_HandleKeyEvent(VMINT event, VMINT keycode)
{
    int kid = mapKeycode(keycode);
    if (kid < 0) return;

    if (event == VM_KEY_EVENT_DOWN || event == VM_KEY_EVENT_LONG_PRESS)
        key_down[kid] = 1;
    else if (event == VM_KEY_EVENT_UP)
        key_down[kid] = 0;
}

void updateKeys(void)
{
    for (int i = 0; i < NUM_KEYS; i++)
    {
        if (key_down[i])
        {
            if (*key_counter[i] < 255) (*key_counter[i])++;
        }
        else
        {
            *key_counter[i] = 0;
        }
    }
}

/* ------------------------------------------------------------------
 * Multi-tap text input for commander name (standard phone keypad)
 * 1 = symbol cycle
 * 2..9 = ABC..WXYZ
 * 0 = space
 * # (second) = toggle upper/lower
 * ------------------------------------------------------------------ */

#define MTAP_COMMIT_FRAMES 12  /* ~600ms at FRAME_TIME=50ms */

static const char* mtap_groups_upper[9] = {
    ".,'-?!\"():;+/\\=_", /* key 1 (alpha) symbols */
    "ABC",               /* key 2 (prgm)  */
    "DEF",               /* key 3 (vars)  */
    "GHI",               /* key 4 (apps)  */
    "JKL",               /* key 5 (graphVar) */
    "MNO",               /* key 6 (math)  */
    "PQRS",              /* key 7 (del)   */
    "TUV",               /* key 8 (stat)  */
    "WXYZ"               /* key 9 (yequ)  */
};

static const char* mtap_groups_lower[9] = {
    ".,'-?!\"():;+/\\=_",
    "abc","def","ghi","jkl","mno","pqrs","tuv","wxyz"
};

static signed char mtap_active_key = -1;   /* 0..8, or -1 none */
static unsigned char mtap_index = 0;
static unsigned char mtap_timer = 0;
static unsigned char mtap_upper = 1;       /* 1 upper, 0 lower */
static unsigned char second_prev = 0;

static char mtap_current_char(void)
{
    if (mtap_active_key < 0 || mtap_active_key > 8) return '\0';
    const char* grp = mtap_upper ? mtap_groups_upper[mtap_active_key]
                                 : mtap_groups_lower[mtap_active_key];
    return grp[mtap_index];
}

static char mtap_commit_and_clear(void)
{
    char out = mtap_current_char();
    mtap_active_key = -1;
    mtap_index = 0;
    mtap_timer = 0;
    return out;
}

char getChar(void)
{
    char out = '\0';

    /* # toggles case */
    if (second == 1 && second_prev == 0)
        mtap_upper ^= 1;
    second_prev = (second > 0) ? 1 : 0;

    /* 0 => immediate space (commit pending first) */
    if (mode == 1)
    {
        if (mtap_active_key >= 0) return mtap_commit_and_clear();
        return ' ';
    }

    /* keypad mapping:
       alpha=1, prgm=2, vars=3, apps=4, graphVar=5, math=6, del=7, stat=8, yequ=9 */
    signed char pressed_key = -1;
    if (alpha == 1)         pressed_key = 0; /* key 1 symbols */
    else if (prgm == 1)     pressed_key = 1; /* key 2 ABC */
    else if (vars == 1)     pressed_key = 2; /* key 3 DEF */
    else if (apps == 1)     pressed_key = 3; /* key 4 GHI */
    else if (graphVar == 1) pressed_key = 4; /* key 5 JKL */
    else if (math == 1)     pressed_key = 5; /* key 6 MNO */
    else if (del == 1)      pressed_key = 6; /* key 7 PQRS */
    else if (stat == 1)     pressed_key = 7; /* key 8 TUV */
    else if (yequ == 1)     pressed_key = 8; /* key 9 WXYZ */

    if (pressed_key >= 0)
    {
        if (mtap_active_key == -1)
        {
            mtap_active_key = pressed_key;
            mtap_index = 0;
            mtap_timer = 0;
            return '\0';
        }

        if (mtap_active_key == pressed_key)
        {
            const char* grp = mtap_upper ? mtap_groups_upper[pressed_key]
                                         : mtap_groups_lower[pressed_key];
            unsigned char len = 0;
            while (grp[len] != '\0') len++;
            mtap_index = (unsigned char)((mtap_index + 1) % len);
            mtap_timer = 0;
            return '\0';
        }

        out = mtap_commit_and_clear();
        mtap_active_key = pressed_key;
        mtap_index = 0;
        mtap_timer = 0;
        return out;
    }

    /* Enter commits pending character quickly */
    if (enter == 1 && mtap_active_key >= 0)
        return mtap_commit_and_clear();

    /* Auto commit by timeout */
    if (mtap_active_key >= 0)
    {
        if (mtap_timer < 255) mtap_timer++;
        if (mtap_timer >= MTAP_COMMIT_FRAMES)
            return mtap_commit_and_clear();
    }

    return '\0';
}
