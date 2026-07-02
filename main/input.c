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
        case VM_KEY_UP:              return KID_UP;
        case VM_KEY_DOWN:            return KID_DOWN;
        case VM_KEY_LEFT:            return KID_LEFT;
        case VM_KEY_RIGHT:           return KID_RIGHT;
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

/* Simple number-key to character mapping for commander name entry */
char getChar(void)
{
    /* Multi-tap not implemented; single key = one letter */
    if (alpha == 1)  return 'A';
    if (prgm == 1)   return 'B';
    if (vars == 1)   return 'C';
    if (apps == 1)   return 'D';
    if (graphVar == 1) return 'E';
    if (math == 1)   return 'F';
    if (del == 1)    return 'G';
    if (stat == 1)   return 'H';
    if (mode == 1)   return 'I';
    if (second == 1) return 'J';
    return '\0';
}
