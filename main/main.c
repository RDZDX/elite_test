#include "vmsys.h"
#include "vmgraph.h"
#include "vmio.h"
#include "vmchset.h"
#include "vmstdlib.h"
#include "vmtimer.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "platform.h"
#include "variables.h"
#include "xorgfx.h"
#include "input.h"
#include "ship.h"
#include "ship_data.h"
#include "generation.h"
#include "market.h"
#include "flight.h"
#include "upgrades.h"

/* platform.h globals are defined in platform.c */

/* ---------- globals required by variables.h ---------- */
char          cmdr_name[CMDR_NAME_MAX_LENGTH];
unsigned char cmdr_name_length = 0;

bool          toExit  = false;
unsigned char drawCycle = 0;

enum currentMenu_t currentMenu;
unsigned char      menu_selOption = 0;

bool          player_dead;
unsigned char player_fuel;
unsigned int  player_money;
unsigned char player_outlaw;
unsigned int  player_kills;
unsigned char player_cargo_space;

static const int titleShipOffset[] =
{
      0,   // Sidewinder
      0,   // Viper
      0,   // Mamba
   3000,   // Cobra
   6000,   // Python
   4000,   // Thargoid
   5000,   // Coriolis
  -1000,   // Missile
    500,   // Asteroid
  -2000,   // Canister
  -1500,   // Thargon
  -2000    // Escape pod
};

/* ---------- forward declarations ---------- */
void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);
static void game_tick(VMINT tid);

//unsigned int key_count = 0;

typedef enum {
    PHASE_INIT,
    PHASE_CHECK_SAVE,
    PHASE_TITLE_LOAD,
    PHASE_NAME_CMDR,
    PHASE_TITLE_START,
    PHASE_MENU,
    PHASE_FLIGHT,
    PHASE_DEAD,
    PHASE_EXIT
} game_phase_t;

static game_phase_t game_phase = PHASE_INIT;
static VMINT timer_id = -1;

static bool has_save = false;
static bool title_initialized = false;
static bool title_accept_enter = false;
static unsigned char title_query_length = 0;
static char* title_query = NULL;
static struct ship_t* title_ship = NULL;

/* ---------- MRE entry point ---------- */
void vm_main(void)
{
    screen_w       = LOGICAL_W;
    screen_h       = LOGICAL_H;
    xor_clipWidth  = screen_w;
    xor_clipHeight = screen_h - DASH_HEIGHT;

    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);
    timer_id = vm_create_timer(FRAME_TIME, game_tick);
    srand((unsigned int)vm_get_tick_count());
}

/* ---------- system event handler ---------- */
void handle_sysevt(VMINT message, VMINT param)
{
    switch (message)
    {
    case VM_MSG_CREATE:
    case VM_MSG_ACTIVE:
        vm_switch_power_saving_mode(turn_off_mode);
        game_phase = PHASE_INIT;
        title_initialized = false;
        numShips = 0;
        if (layer_hdl < 0)
        {
            layer_hdl = vm_graphic_create_layer(0, 0, LOGICAL_W, LOGICAL_H, -1);
            layer_buf = vm_graphic_get_layer_buffer(layer_hdl);
            vm_graphic_set_clip(0, 0, LOGICAL_W, LOGICAL_H);
        }
        if (timer_id < 0) timer_id = vm_create_timer(FRAME_TIME, game_tick);
        break;

    case VM_MSG_INACTIVE:
    case VM_MSG_QUIT:
        if (timer_id >= 0)
        {
            vm_delete_timer(timer_id);
            timer_id = -1;
        }
        if (layer_hdl >= 0)
        {
            vm_graphic_delete_layer(layer_hdl);
            layer_hdl = -1;
            layer_buf = NULL;
        }
        break;

    default:
        break;
    }
}

/* ---------- keyboard callback ---------- */
void handle_keyevt(VMINT event, VMINT keycode)
{
//    key_count++;
    input_HandleKeyEvent(event, keycode);
}

/* ========================================================================
 * Game logic (ported from original main.c)
 * ======================================================================== */

static void printPlayerCondition(void)
{
    switch (player_condition)
    {
    case DOCKED:  xor_Print("Docked");  break;
    case GREEN:   xor_Print("Green");   break;
    case YELLOW:  xor_Print("Yellow");  break;
    case RED:     xor_Print("Red");     break;
    default:      xor_Print("Unknown"); break;
    }
}

static void drawMenu(bool resetCrs)
{
    /* Background */
    plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);
    drawDashboard();

    /* Outer frame */
    xor_Rectangle(DASH_HOFFSET, 0, DASH_WIDTH - 1, DASH_VOFFSET - 1);
    xor_HorizontalLine(HEADER_DIVIDER_Y, DASH_HOFFSET + 1, DASH_HOFFSET + DASH_WIDTH - 2);

    if (resetCrs) menu_selOption = 0;

    switch (currentMenu)
    {
    case MAIN:
        xor_CenterText("SHIP COMPUTER", 13, HEADER_Y);
//        xor_SetCursorPos(0, 4);
        xor_SetCursorPos(0, 3);
        xor_lineSpacing = true;
        xor_Print("Status\n");
        xor_Print("Navigation\n");
        xor_Print("System Data\n");
        if (player_condition == DOCKED)
        {
            xor_Print("Sell Cargo\n");
            xor_Print("Leave Station\n");
            xor_Print("Save & Quit");
        }
        else
        {
            xor_Print("Cargo Hold\n");
            xor_Print("Return\n");
            xor_Print("Quit");
        }
        xor_FillRectangle(xor_clipX + LEFT_TEXT_INDENT - 2, 21 + 16 * menu_selOption, MM_SELBAR_WIDTH, 11);
//                          29 + 16 * menu_selOption, MM_SELBAR_WIDTH, 11);
        break;

    case STATUS:
        xor_CenterTextOffset("COMMANDER", 9, HEADER_Y, 1 + cmdr_name_length);
        xor_CenterTextOffset(cmdr_name, cmdr_name_length, HEADER_Y, -10);
//        xor_SetCursorPos(0, 3);
        xor_SetCursorPos(0, 1);
//        xor_lineSpacing = false;
        xor_lineSpacing = true;
//        xor_Print("Present System");
        xor_Print("\nPresent System: ");
//        xor_SetCursorPos(20, 3);
//        xor_PrintChar(':');
        gen_PrintName(&currentSeed, true);
//        xor_Print("\nHyperspace System");
        xor_Print("\nHyperspace System: ");
//        xor_SetCursorPos(20, 4);
//        xor_PrintChar(':');
        gen_PrintName(&selectedSeed, true);
//        xor_Print("\nCondition");
        xor_Print("\nCondition: ");
//        xor_SetCursorPos(20, 5);
//        xor_PrintChar(':');
        printPlayerCondition();
        xor_Print("\nFuel: ");
        xor_PrintUInt8Tenths(player_fuel, 1);
        xor_Print(" Light Years");
        xor_Print("\nCash: ");
        xor_PrintUInt24Tenths(player_money);
        xor_Print(" Cr");
        xor_Print("\nLegal Status: ");
        if      (player_outlaw == 0)  xor_Print("Clean");
        else if (player_outlaw < 50)  xor_Print("Offender");
        else                           xor_Print("Fugitive");
        xor_Print("\nRating: ");
        if      (player_kills < 4)    xor_Print("Harmless");
        else if (player_kills < 8)    xor_Print("Mostly Harmless");
        else if (player_kills < 16)   xor_Print("Poor");
        else if (player_kills < 32)   xor_Print("Average");
        else if (player_kills < 256)  xor_Print("Above Average");
        else if (player_kills < 512)  xor_Print("Competent");
        else if (player_kills < 2560) xor_Print("Dangerous");
        else if (player_kills < 6400) xor_Print("Deadly");
        else                           xor_Print("---- E L I T E ----");
//        xor_Print("\n\nEQUIPMENT:");
        xor_Print("\nEquipment: ");
        upg_DisplayEquipment();
        break;

    case THIS_DATA:
    case SEL_DATA:
        {
            const struct gen_sysData_t* relevantData = currentMenu == THIS_DATA ? &thisSystemData : &selectedSystemData;
            const struct gen_seed_t* relevantSeed = currentMenu == THIS_DATA ? &currentSeed : &selectedSeed;
            xor_CenterTextOffset("DATA ON", 7, HEADER_Y, 1 + (signed char)strlen(relevantData->name));
            xor_CenterTextOffset(relevantData->name, (unsigned char)strlen(relevantData->name), HEADER_Y, -8);
//            xor_SetCursorPos(0, 6);
            xor_SetCursorPos(0, 3);
            xor_lineSpacing = true;
            xor_Print("Economy: ");      gen_PrintEconomy(relevantData);
            xor_Print("\nGovernment: "); gen_PrintGovernment(relevantData);
            xor_Print("\nTech. Level: "); gen_PrintTechnology(relevantData);
            xor_Print("\nPopulation: "); gen_PrintPopulation(relevantData, relevantSeed);
            xor_Print("\nGross Productivity: "); gen_PrintProductivity(relevantData);
            xor_Print("\nAverage Radius: ");     gen_PrintRadius(relevantSeed);
        }
        break;

    case LOCAL_MAP:
        xor_CenterText("SHORT RANGE CHART", 17, HEADER_Y);
        gen_DrawLocalMap();
        break;

    case GALAXY_MAP:
        xor_CenterTextOffset("GALAXY CHART", 12, HEADER_Y, 2);
        xor_CenterTextOffset((char[]){ (char)('1' + gen_currentGalaxy) }, 1, HEADER_Y, -13);
        gen_DrawGalaxyMap();
        break;

    case MARKET:
        xor_CenterTextOffset(thisSystemData.name, (unsigned char)strlen(thisSystemData.name), HEADER_Y, 12);
        xor_CenterTextOffset("MARKET LINK", 11, HEADER_Y, -1 - (signed char)strlen(thisSystemData.name));
//        xor_rowHeight = 9;
        mkt_PrintMarketTable();
//        xor_rowHeight = 8;
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                         25 + 9 * menu_selOption, DASH_WIDTH - 10, 10);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          21 + 9 * menu_selOption, DASH_WIDTH - 10, 8);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          ((menu_selOption + 3) * 9) - 7 , DASH_WIDTH - 10, 7); // geras
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          ((menu_selOption + 3) * 9) - 6, DASH_WIDTH - 10, 7);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          ((menu_selOption + 3) * 9) - 8, DASH_WIDTH - 10, 8);
        xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
        break;


case MARKET_QTY:

    xor_CenterTextOffset(thisSystemData.name, (unsigned char)strlen(thisSystemData.name), HEADER_Y, 12);

    xor_CenterTextOffset("MARKET LINK", 11, HEADER_Y, -1 - (signed char)strlen(thisSystemData.name));

//    mkt_PrintMarketTable();

    if (mkt_qtyBuy)
        mkt_PrintMarketTable();
    else
        mkt_PrintInventoryTable();

//    xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(mkt_qtyGood), DASH_WIDTH - 10, 8);
//    xor_FillRectangle(DASH_HOFFSET + 5, INVENTORY_SEL_Y(mkt_GetInventoryRow(mkt_qtyGood)), DASH_WIDTH - 10, 8);

if (mkt_qtyBuy)
{
    xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(mkt_qtyGood), DASH_WIDTH - 10, 8);
}
else
{
    xor_FillRectangle(DASH_HOFFSET + 5,INVENTORY_SEL_Y(mkt_GetInventoryRow(mkt_qtyGood)), DASH_WIDTH - 10, 8);
}


//    mkt_PrintQtyQuery(mkt_qtyGood, true);
    mkt_PrintQtyQuery(mkt_qtyGood, mkt_qtyBuy);

    xor_PrintUInt24Adaptive(mkt_qtyValue);

    break;

    case INVENTORY:
        xor_CenterText(player_condition == DOCKED ? "SELL CARGO" : "CARGO HOLD", 10, HEADER_Y);
        mkt_PrintInventoryTable();
        if (player_condition == DOCKED && !mkt_InventoryEmpty())
        {
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
            xor_FillRectangle(DASH_HOFFSET + 5, INVENTORY_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
        }
        break;

    case UPGRADES:
        xor_CenterText("OUTFITTING", 10, HEADER_Y);
        upg_PrintOutfittingTable();
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          HEADER_DIVIDER_Y + 11 + 9 * menu_selOption, DASH_WIDTH - 10, 8);
//                          HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          UPGRADE_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
//        xor_FillRectangle(DASH_HOFFSET + 5,
//                          UPGRADE_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
        xor_FillRectangle(DASH_HOFFSET + 5, UPGRADE_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
        break;

    default:
        xor_CenterText("UNDEFINED MENU", 14, HEADER_Y);
        break;
    }

    plat_FlushScreen();
}

static bool doMenuInput(void)
{
    const enum currentMenu_t lastMenu    = currentMenu;
    const unsigned char      prevSelOpt  = menu_selOption;
    const signed int         prevCrsX    = gen_crsX;
    const signed int         prevCrsY    = gen_crsY;

    enum currentMenu_t returnMenu;

    switch (currentMenu)
    {
    case MAIN:
        returnMenu = player_condition == DOCKED ? MARKET : NONE;
        if (up == 1 || up > HOLD_TIME)
        {
            if (menu_selOption > 0) menu_selOption--;
            else menu_selOption = NUM_MENU_OPTIONS - 1;
        }
        if (down == 1 || down > HOLD_TIME)
        {
            menu_selOption++;
            if (menu_selOption >= NUM_MENU_OPTIONS) menu_selOption = 0;
        }
        if (enter == 1 || graph == 1)
        {
            switch (menu_selOption)
            {
            case 0: currentMenu = STATUS; break;
            case 1:
                currentMenu = LOCAL_MAP;
                gen_ResetCursorPosition(true);
                break;
            case 2: currentMenu = THIS_DATA; break;
            case 3: currentMenu = INVENTORY; break;
            case 5: toExit = true;  /* fall through */
            case 4: return false;
            }
        }
        if (menu_selOption != prevSelOpt)
        {
            xor_FillRectangle(xor_clipX + LEFT_TEXT_INDENT - 2,
                              21 + 16 * menu_selOption,  MM_SELBAR_WIDTH, 11);
//                              29 + 16 * menu_selOption,  MM_SELBAR_WIDTH, 11);
            xor_FillRectangle(xor_clipX + LEFT_TEXT_INDENT - 2,
                              21 + 16 * prevSelOpt, MM_SELBAR_WIDTH, 11);
//                              29 + 16 * prevSelOpt, MM_SELBAR_WIDTH, 11);
            plat_FlushScreen();
        }
        break;

    case LOCAL_MAP:
        returnMenu = MAIN;
        if (left  == 1 || left  > HOLD_TIME)
        {
            gen_crsX -= 4;
            while (gen_crsX <= -4 * LCL_MAP_DXMAX) gen_crsX++;
        }
        else if (right == 1 || right > HOLD_TIME)
        {
            gen_crsX += 4;
            while (gen_crsX >= 4 * LCL_MAP_DXMAX) gen_crsX--;
        }
        if (up == 1 || up > HOLD_TIME)
        {
            gen_crsY -= 4;
            while (gen_crsY <= -2 * LCL_MAP_DYMAX + SML_CRS_SIZE + SML_CRS_SPREAD) gen_crsY++;
        }
        else if (down == 1 || down > HOLD_TIME)
        {
            gen_crsY += 4;
            while (gen_crsY >= 2 * LCL_MAP_DYMAX) gen_crsY--;
        }
        if (enter == 1) gen_SelectNearestSystem(true);
        if (graph == 1) { currentMenu = GALAXY_MAP; gen_ResetCursorPosition(false); }
        break;

    case GALAXY_MAP:
        returnMenu = LOCAL_MAP;
        if (left  == 1 || left  > HOLD_TIME) { gen_crsX -= 4; while (gen_crsX < 0)   gen_crsX++; }
        else if (right == 1 || right > HOLD_TIME) { gen_crsX += 4; while (gen_crsX > 256) gen_crsX--; }
        if (up   == 1 || up   > HOLD_TIME) { gen_crsY -= 4; while (gen_crsY < 0)   gen_crsY++; }
        else if (down == 1 || down > HOLD_TIME) { gen_crsY += 4; while (gen_crsY > 128) gen_crsY--; }
        if (enter == 1) gen_SelectNearestSystem(false);
        else if (yequ == 1) gen_ResetCursorPosition(true);
        else if (graph == 1) currentMenu = SEL_DATA;
        break;

    case SEL_DATA:
        returnMenu = GALAXY_MAP;
        break;

    case MARKET:
        returnMenu = UPGRADES;
        if (graph == 1) currentMenu = MAIN;
        if (up == 1 || up > HOLD_TIME)
        {
            menu_selOption--;
            if (menu_selOption >= NUM_TRADE_GOODS) menu_selOption = NUM_TRADE_GOODS - 1;
        }
        else if (down == 1 || down > HOLD_TIME)
        {
            menu_selOption++;
            if (menu_selOption >= NUM_TRADE_GOODS) menu_selOption = 0;
        }
//        if (enter == 1) { mkt_Buy(menu_selOption); drawMenu(false); }
        if (enter == 1) {mkt_qtyGood = menu_selOption; mkt_qtyValue = 0; mkt_qtyBuy = true; currentMenu = MARKET_QTY;}
//        if (enter == 1) { mkt_qtyGood = menu_selOption; mkt_qtyValue = 0; mkt_qtyBuy = true; enter = 0; currentMenu = MARKET_QTY; }

//        if (enter == 1) { mkt_qtyGood = mkt_GetInventoryGood(menu_selOption); mkt_qtyValue = 0; mkt_qtyBuy = false; currentMenu = MARKET_QTY; }

        if (menu_selOption != prevSelOpt)
        {
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
            xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(prevSelOpt), DASH_WIDTH - 10, 8);
            xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
            plat_FlushScreen();
        }
        break;

case MARKET_QTY:

//    if (clear == 1)
//    {
//        currentMenu = MARKET;
//        break;
//    }

if (clear == 1)
{
    currentMenu = mkt_qtyBuy ? MARKET : INVENTORY;
    break;
}

    if (alpha == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 1;

    if (prgm == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 2;

    if (vars == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 3;

    if (apps == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 4;

    if (graphVar == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 5;

    if (math == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 6;

    if (del == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 7;

    if (stat == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 8;

    if (mode == 1)
        mkt_qtyValue = mkt_qtyValue * 10 + 0;

    if (mkt_qtyValue > 200)
        mkt_qtyValue = 200;

//    if (enter == 1) {
//        mkt_BuyAmount(mkt_qtyGood, mkt_qtyValue);
//        currentMenu = MARKET;
////        drawMenu(false);

//        if (lastMenu != currentMenu) {
//            drawMenu(true);
//        } else {
//            drawMenu(false);
//        }

//    }

//    if (enter == 1 && mkt_qtyValue > 0) {
//        mkt_BuyAmount(mkt_qtyGood, mkt_qtyValue);

//        currentMenu = MARKET;
//        drawMenu(false);
//        return true;
//    }


if (enter == 1 && mkt_qtyValue > 0)
{
    if (mkt_qtyBuy)
        mkt_BuyAmount(mkt_qtyGood, mkt_qtyValue);
    else
        mkt_SellAmount(mkt_qtyGood, mkt_qtyValue);

    currentMenu = mkt_qtyBuy ? MARKET : INVENTORY;

    drawMenu(false);
    return true;
}


    drawMenu(false);

    break;

    case UPGRADES:
        returnMenu = UPGRADES;
        {
            const unsigned char displayedOptions = thisSystemData.techLevel < 10
                                                 ? thisSystemData.techLevel + 2
                                                 : NUM_UPGRADES + 1;
            if (up == 1 || up > HOLD_TIME)
            {
                menu_selOption--;
                if (menu_selOption > displayedOptions) menu_selOption = displayedOptions;
            }
            else if (down == 1 || down > HOLD_TIME)
            {
                menu_selOption++;
                if (menu_selOption > displayedOptions) menu_selOption = 0;
            }
        }
        if (enter == 1) { if (upg_Buy(menu_selOption)) drawMenu(false); }
        if (graph == 1) currentMenu = MARKET;
        if (menu_selOption != prevSelOpt)
        {
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 11 + 9 * prevSelOpt, DASH_WIDTH - 10, 8);
//            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 11 + 9 * menu_selOption, DASH_WIDTH - 10, 8);
            xor_FillRectangle(DASH_HOFFSET + 5, UPGRADE_SEL_Y(prevSelOpt), DASH_WIDTH - 10, 8);
            xor_FillRectangle(DASH_HOFFSET + 5, UPGRADE_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
            plat_FlushScreen();
        }
        break;

    case INVENTORY:

        unsigned char count = mkt_GetInventoryCount();

        if (player_condition == DOCKED && !mkt_InventoryEmpty())
        {
            if (up == 1 || up > HOLD_TIME)
            {
//                menu_selOption--;
//                if (menu_selOption >= NUM_TRADE_GOODS) menu_selOption = NUM_TRADE_GOODS - 1;

    if (menu_selOption > 0)
        menu_selOption--;
    else
        menu_selOption = count - 1;

            }
            else if (down == 1 || down > HOLD_TIME)
            {
                menu_selOption++;
//                if (menu_selOption >= NUM_TRADE_GOODS) menu_selOption = 0;
                if (menu_selOption >= count) menu_selOption = 0;
            }
            if (menu_selOption != prevSelOpt)
            {
//                xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
//                xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
//                xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(prevSelOpt), DASH_WIDTH - 10, 8);
//                xor_FillRectangle(DASH_HOFFSET + 5, MARKET_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);

//                xor_FillRectangle(DASH_HOFFSET + 5, mkt_GetInventoryRowY(prevSelOpt), DASH_WIDTH - 10, 8);
//                xor_FillRectangle(DASH_HOFFSET + 5, mkt_GetInventoryRowY(menu_selOption), DASH_WIDTH - 10, 8);

                xor_FillRectangle(DASH_HOFFSET + 5, INVENTORY_SEL_Y(prevSelOpt), DASH_WIDTH - 10, 8);
                xor_FillRectangle(DASH_HOFFSET + 5, INVENTORY_SEL_Y(menu_selOption), DASH_WIDTH - 10, 8);
                plat_FlushScreen();
            }
//            if (enter == 1) { mkt_Sell(menu_selOption); drawMenu(true); }
//              if (enter == 1) { mkt_qtyGood = menu_selOption; mkt_qtyValue = 0; mkt_qtyBuy = false; currentMenu = MARKET_QTY; }


if (enter == 1)
{
    mkt_qtyGood = mkt_GetInventoryGood(menu_selOption);

    mkt_qtyValue = 0;
    mkt_qtyBuy = false;

    currentMenu = MARKET_QTY;
}


        }
        /* fall through */
    default:
        returnMenu = MAIN;
        break;
    }

    if (gen_crsX != prevCrsX || gen_crsY != prevCrsY)
        gen_RedrawCursorPosition(prevCrsX, prevCrsY);

    if (yequ == 1) currentMenu = returnMenu;

    if (currentMenu == NONE) return false;
    if (lastMenu != currentMenu) drawMenu(true);

    return true;
}

static void begin(void)
{
    currentMenu = STATUS;
    drawCycle   = 0;

    originSeed  = (struct gen_seed_t){ 0x5a4a, 0x0248, 0xb753 };
    currentSeed = (struct gen_seed_t){ 0xad38, 0x149c, 0x151d };

    gen_currentGalaxy = 0;
    gen_SetSystemData(&thisSystemData, &currentSeed);
    selectedSeed       = currentSeed;
    selectedSystemData = thisSystemData;
    gen_ResetDistanceToTarget();

    marketSeed = rand() % 256;
    mkt_ResetLocalMarket();

    player_dead        = false;
    player_fuel        = 70;
    player_money       = 1000;
    player_outlaw      = 0;
    player_cargo_space = 25;

    flt_Init();
}

static void titleScreen_begin(unsigned char shipType,
                              char query[],
                              unsigned char queryLength,
                              bool acceptEnter)
{

    numShips = 0;
//    title_ship = NewShip(shipType,
//                         (struct vector_t){ 0, 0, TTL_SHIP_START_Z },
//                         Matrix(-256,0,0, 0,0,256, 0,256,0));

    title_ship = NewShip(shipType,
                        (struct vector_t){ 0, 0, TTL_SHIP_START_Z + titleShipOffset[shipType]},
                         Matrix(-256,0,0, 0,0,256, 0,256,0));

    title_ship->pitch = 127;
    title_ship->roll  = 127;
    title_query = query;
    title_query_length = queryLength;
    title_accept_enter = acceptEnter;
    title_initialized = true;
}

static unsigned char titleScreen_tick(void)
{
    if (!title_initialized || !title_ship) return 0;

    plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);

    xor_CenterText("---- E L I T E ----", 19, HEADER_Y);
    xor_CenterText(title_query, title_query_length,
                   (unsigned char)(xor_clipY + xor_clipHeight - 3 * HEADER_Y - 8));

    if (title_ship->position.z > TTL_SHIP_END_Z)
        title_ship->position.z -= TTL_SHIP_ZOOM_RATE;

    MoveShip(title_ship);
    DrawShip(title_ship);
    title_ship->orientation = orthonormalize(title_ship->orientation);

    xor_Rectangle(DASH_HOFFSET, 0, DASH_WIDTH - 1, DASH_VOFFSET - 1); //?????????????????
    drawDashboard(); //????????????????????

    if (!title_accept_enter)
    {

        xor_SetCursorPos(9, xor_textRows + 1);
//        xor_Print("Load Saved Commander ?");

////        xor_SetCursorPos(5, xor_textRows);
        xor_SetCursorPos(5, xor_textRows + 4);
//        xor_Print("(Y)"); //(N)
////        xor_SetCursorPos(xor_textCols - 10, xor_textRows);
        xor_SetCursorPos(xor_textCols - 10, xor_textRows + 4);
//        xor_Print("(N)"); //(Y)
    }

    plat_FlushScreen();

    if (clear == 1) return 2;

    if (!title_accept_enter)
    {
        if (yequ == 1)  return 0;
        if (graph == 1) return 1;
        return 3;
    }

    if (enter == 1) return 1;
    return 3;
}

static void loadGame(void)
{
    VMFILE f = plat_FileOpen(SAVE_VAR_NAME, VM_FS_READ_ONLY);
    if (f < 0) return;

    plat_FileRead(f, &cmdr_name,          CMDR_NAME_MAX_LENGTH);
    plat_FileRead(f, &gen_currentGalaxy,  1);
    plat_FileRead(f, &originSeed,         6); /* 3 x uint16 */
    plat_FileRead(f, &currentSeed,        6);
    plat_FileRead(f, &selectedSeed,       6);
    plat_FileRead(f, &marketSeed,         1);
    plat_FileRead(f, &player_fuel,        1);
    plat_FileRead(f, &player_money,       sizeof(player_money));
    plat_FileRead(f, &player_outlaw,      1);
    plat_FileRead(f, &player_kills,       sizeof(player_kills));
    plat_FileRead(f, &player_cargo_space, 1);
    plat_FileRead(f, &player_missiles,    1);
    plat_FileRead(f, &player_lasers,      sizeof(player_lasers));
    plat_FileRead(f, &player_upgrades,    sizeof(player_upgrades));
    plat_FileRead(f, &inventory,          NUM_TRADE_GOODS);

    gen_SetSystemData(&thisSystemData,    &currentSeed);
    gen_SetSystemData(&selectedSystemData, &selectedSeed);
    gen_ResetDistanceToTarget();

    mkt_ResetLocalMarket();
    plat_FileRead(f, &mkt_localQuantities, NUM_TRADE_GOODS);

    plat_FileClose(f);

    cmdr_name_length = 0;
    for (unsigned char i = 0; cmdr_name[i] != '\0'; i++) cmdr_name_length++;
}

static void saveGame(void)
{
    VMFILE f = plat_FileOpen(SAVE_VAR_NAME, MODE_CREATE_ALWAYS_WRITE);
    if (f < 0) return;

    plat_FileWrite(f, &cmdr_name,           CMDR_NAME_MAX_LENGTH);
    plat_FileWrite(f, &gen_currentGalaxy,   1);
    plat_FileWrite(f, &originSeed,          6);
    plat_FileWrite(f, &currentSeed,         6);
    plat_FileWrite(f, &selectedSeed,        6);
    plat_FileWrite(f, &marketSeed,          1);
    plat_FileWrite(f, &player_fuel,         1);
    plat_FileWrite(f, &player_money,        sizeof(player_money));
    plat_FileWrite(f, &player_outlaw,       1);
    plat_FileWrite(f, &player_kills,        sizeof(player_kills));
    plat_FileWrite(f, &player_cargo_space,  1);
    plat_FileWrite(f, &player_missiles,     1);
    plat_FileWrite(f, &player_lasers,       sizeof(player_lasers));
    plat_FileWrite(f, &player_upgrades,     sizeof(player_upgrades));
    plat_FileWrite(f, &inventory,           NUM_TRADE_GOODS);
    plat_FileWrite(f, &mkt_localQuantities, NUM_TRADE_GOODS);

    plat_FileClose(f);
}

static void nameCmdr_tick(void)
{
    plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);
    xor_CenterText("---- E L I T E ----", 19, HEADER_Y);
//    xor_SetCursorPos(1, xor_textRows - 1);
    xor_SetCursorPos(1, xor_textRows - 2);
    xor_Print("Commander Name: ");

    xor_Rectangle(DASH_HOFFSET, 0, DASH_WIDTH - 1, DASH_VOFFSET - 1); //???????????
    drawDashboard(); //????????????????????????????????

    if (cmdr_name_length > 0)
    {
        xor_Print(cmdr_name);
    }

    char typedChar = getChar();
    /* reserve one slot for '\0' so appending + terminator stays in-bounds */
    if (typedChar != '\0' && cmdr_name_length < CMDR_NAME_MAX_LENGTH - 1)
    {
        cmdr_name[cmdr_name_length] = typedChar;
        cmdr_name_length++;
        cmdr_name[cmdr_name_length] = '\0';
    }

    if (clear == 1)
    {
        if (cmdr_name_length > 0)
        {
            cmdr_name_length--;
            cmdr_name[cmdr_name_length] = '\0';
        }
        else
        {
            game_phase = PHASE_EXIT;
        }
    }
    else if (enter == 1)
    {
        game_phase = PHASE_TITLE_START;
    }

    plat_FlushScreen();
}

static void phase_init(void)
{
    clear = 1; /* prevent immediate exit */
    begin();
    toExit = false;
    game_phase = PHASE_CHECK_SAVE;
}

static void phase_check_save(void)
{
    has_save = plat_FileExists(SAVE_VAR_NAME) != 0;
    game_phase = has_save ? PHASE_TITLE_LOAD : PHASE_NAME_CMDR;
}

static void phase_title_load(void)
{
    unsigned char tsResponse = titleScreen_tick();
    if (tsResponse == 3) return;

    title_initialized = false;
    numShips = 0;

    if (tsResponse == 2) { game_phase = PHASE_EXIT; return; }
    if (tsResponse == 1) { loadGame(); game_phase = PHASE_TITLE_START; return; }
    game_phase = PHASE_NAME_CMDR;
}

static void phase_name_cmdr(void)
{
    nameCmdr_tick();
}

static void phase_title_start(void)
{
    unsigned char tsResponse = titleScreen_tick();
    if (tsResponse == 3) return;

    title_initialized = false;
    numShips = 0;

    if (tsResponse == 2) { game_phase = PHASE_EXIT; return; }
    if (tsResponse == 1) game_phase = PHASE_MENU;
}

static void phase_menu(void)
{
    if (doMenuInput()) return;

    if (player_condition == DOCKED) saveGame();
    if (toExit) { game_phase = PHASE_EXIT; return; }
    game_phase = PHASE_FLIGHT;
}

static void phase_flight(void)
{
    if (!doFlightInput())
    {
        game_phase = PHASE_MENU;
        return;
    }

    flt_DoFrame(true);
    plat_FlushScreen();
    drawCycle++;

    if (player_dead) { game_phase = PHASE_DEAD; return; }
//    if (player_dead) { game_phase = PHASE_EXIT; return; }
    if (player_condition == DOCKED) game_phase = PHASE_MENU;
}

static void phase_dead(void)
{
    if (flt_DeathTick()) game_phase = PHASE_EXIT;
}

static void phase_enter(game_phase_t phase)
{
    switch (phase)
    {
    case PHASE_TITLE_LOAD:
//        titleScreen_begin(BP_COBRA, "Load Saved Commander?", 21, false);
        titleScreen_begin(BP_COBRA, "Load Saved Commander Y/N ?", 26, false);
//        titleScreen_begin(BP_COBRA, "", 0, false);
        break;
    case PHASE_NAME_CMDR:
        cmdr_name_length = 0;
        cmdr_name[0] = '\0';
        break;
    case PHASE_TITLE_START:
//        titleScreen_begin(BP_MAMBA, "Press ENTER, CMDR.", 18, true);
        titleScreen_begin(BP_COBRA, "Press ENTER, Commander.", 23, true);
        break;
    case PHASE_MENU:
        drawMenu(true);
        break;
    case PHASE_FLIGHT:
        flt_ResetPlayerCondition();
        doFlight();
        break;
    case PHASE_DEAD:
        flt_Death();
        break;
    default:
        break;
    }
}

static void game_tick(VMINT tid)
{
    (void)tid;

    if (!layer_buf) return;

    updateKeys();

    static bool phase_initialized = false;
    static game_phase_t previous_phase = PHASE_INIT;
    if (!phase_initialized || game_phase != previous_phase)
    {
        phase_enter(game_phase);
        previous_phase = game_phase;
        phase_initialized = true;
    }

    switch (game_phase)
    {
    case PHASE_INIT:        phase_init();        break;
    case PHASE_CHECK_SAVE:  phase_check_save();  break;
    case PHASE_TITLE_LOAD:  phase_title_load();  break;
    case PHASE_NAME_CMDR:   phase_name_cmdr();   break;
    case PHASE_TITLE_START: phase_title_start(); break;
    case PHASE_MENU:        phase_menu();        break;
    case PHASE_FLIGHT:      phase_flight();      break;
    case PHASE_DEAD:        phase_dead();        break;
    case PHASE_EXIT:
        if (timer_id >= 0)
        {
            vm_delete_timer(timer_id);
            timer_id = -1;
        }
        vm_exit_app();
        break;
    }
}


