#ifndef variables_define_file
#define variables_define_file

#include <stdbool.h>
#include "platform.h"

/* Save file path on Nokia 225 SD card */
//#define SAVE_VAR_NAME L"E:\\elite.sav"
#define SAVE_VAR_NAME u"E:\\elite.sav"

/* Logical canvas: 320x240 landscape (software-rotated from 240x320 physical). */
extern VMINT screen_w, screen_h;
extern VMINT xor_clipWidth, xor_clipHeight;

#define FRAME_TIME        2u        /* ms per frame (~60fps ceiling; original was ~1.6ms) */
#define CURSOR_BLINK_TIME 10
#define FLTMSG_MED_TIME   25
#define HOLD_TIME         3
#define STAR_JUMP_HOLD_TIME    50
#define GALAXY_JUMP_HOLD_TIME  120
#define TUNNEL_FRAME_TIME      400u /* ms */
#define TUNNEL_HOLD_TIME       1600u
#define DEATH_SCREEN_TIME      8000u

/* Dashboard: simplified 30px strip at the bottom of the screen */
#define DASH_HEIGHT       30
#define DASH_HOFFSET      0
/* Runtime macros that depend on screen_w / screen_h */
#define DASH_VOFFSET      (screen_h - DASH_HEIGHT)
#define DASH_WIDTH        screen_w
#define DASH_HOFFSET_CENTER (screen_w / 2)
#define DASH_HOFFSET_RIGHT  (screen_w * 3 / 4)

/* View area center */
#define VIEW_HCENTER      (screen_w / 2)
#define VIEW_VCENTER      ((screen_h - DASH_HEIGHT) / 2)

/* Header */
#define HEADER_Y          6
#define HEADER_DIVIDER_Y  18

/* Clip region: full screen above dashboard */
#define xor_clipX         0
#define xor_clipY         0
/* xor_clipWidth and xor_clipHeight are runtime globals in platform.h */
#define xor_textRows      (xor_clipHeight / 8)
#define xor_textCols      (xor_clipWidth  / 8)

#define LEFT_TEXT_INDENT  4

#define MM_SELBAR_WIDTH   250

/* Local map constants (scaled for 320x240 landscape) */
#define LCL_MAP_HFIX      160
#define LCL_MAP_VFIX      105
#define LCL_MAP_DXMAX     (LCL_MAP_HFIX / 4)
#define LCL_MAP_DYMAX     (LCL_MAP_VFIX / 2 - HEADER_DIVIDER_Y / 2)
#define GLX_MAP_HOFFSET   0
#define GLX_MAP_VOFFSET   GLX_MAP_HOFFSET

#define CRS_SPREAD        6
#define CRS_SIZE          6
#define SML_CRS_SPREAD    3
#define SML_CRS_SIZE      3

/* Radar: simplified, centered at bottom strip */
#define RADAR_HCENTER     (screen_w / 2)
#define RADAR_VCENTER     (screen_h - DASH_HEIGHT + 15)
#define RADAR_XSCALE      256
#define RADAR_ZSCALE      1024
#define RADAR_YSCALE      512

/* Color indices (kept for compatibility) */
#define COLOR_BLACK   0
#define COLOR_WHITE   1
#define COLOR_YELLOW  2
#define COLOR_GREEN   3
#define COLOR_RED     4

/* SOI indicator position */
#define SOI_INDIC_POS_X   (screen_w - 12)
#define SOI_INDIC_POS_Y   (screen_h - DASH_HEIGHT + 2)

/* Compass */
#define COMPASS_HCENTER   (screen_w / 2)
#define COMPASS_VCENTER   (screen_h - DASH_HEIGHT + 10)
#define COMPASS_SCALE     (256 / 9)

/* Ship drawing constants */
#define SHPPT_WIDTH       3
#define SHPPT_HEIGHT      2
#define DISP_Z_SCALE      256

/* Game constants (unchanged from original) */
#define MAX_SHIPS              16
#define STARDUST_COUNT         12
#define NUM_PLAYER_DEATH_CANS  2

#define NUM_TRADE_GOODS  17
#define NUM_UPGRADES     13

#define PLAYER_MAX_SPEED 0x1c

#define TTL_SHIP_ONE        BP_COBRA
#define TTL_SHIP_TWO        BP_VIPER
#define TTL_SHIP_START_Z    0x1200
#define TTL_SHIP_END_Z      0x0200
#define TTL_SHIP_ZOOM_RATE  0x90

#define CMDR_NAME_MAX_LENGTH  16

#define FLTMSG_MAX_LENGTH  24
#define FLTMSG_VOFFSET     (DASH_VOFFSET - 20)

#define PULSE_LASER_INTERVAL   5
#define LASER_OVERHEAT_TEMP    242
#define LASER_MAX_RANGE        682
#define LASER_LINE_SPREAD      8
#define LASER_LINE_X_ONE       10
#define LASER_LINE_X_TWO       20

#define MISSILE_LAUNCH_Y        20
#define MISSILE_LAUNCH_Z        15
#define MISSILE_LAUNCH_SPEED    24
#define MAX_MISSILE_LOCK_RANGE  (63*256)
#define MISSILE_HIT_RANGE       682
#define MISSILE_DIRECT_DAMAGE   250
#define MISSILE_SPLASH_DAMAGE   80
#define MISSILE_SPLASH_RANGE    256

#define ECM_DURATION  100

#define EXPLOSION_START_SIZE   18
#define EXPLOSION_GROW_RATE    4
#define EXPLOSION_PARTICLE_SIZE 2
#define EXPLOSION_SCALE        1024

extern char cmdr_name[CMDR_NAME_MAX_LENGTH];
extern unsigned char cmdr_name_length;

extern unsigned char drawCycle;

enum currentMenu_t {
    NONE,
    MAIN,
    STATUS,
    THIS_DATA,
    SEL_DATA,
    LOCAL_MAP,
    GALAXY_MAP,
    INVENTORY,
    MARKET,
    UPGRADES
};
#define NUM_MENU_OPTIONS 6
extern enum currentMenu_t currentMenu;

extern bool player_dead;

extern unsigned char player_fuel;
extern unsigned int player_money;
extern unsigned char player_outlaw;
extern unsigned int player_kills;
extern unsigned char player_energy;

extern unsigned char player_cargo_space;

#endif /* variables_define_file */
