#include "vmsys.h"
#include "vmgraph.h"
#include "vmio.h"
#include "vmchset.h"
#include "vmstdlib.h"

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

/* ---------- forward declarations ---------- */
void handle_sysevt(VMINT message, VMINT param);
void handle_keyevt(VMINT event, VMINT keycode);
static void game_run(void);

/* ---------- MRE entry point ---------- */
void vm_main(void)
{
    screen_w       = vm_graphic_get_screen_width();
    screen_h       = vm_graphic_get_screen_height();
    xor_clipWidth  = screen_w;
    xor_clipHeight = screen_h - DASH_HEIGHT;

    vm_reg_sysevt_callback(handle_sysevt);
    vm_reg_keyboard_callback(handle_keyevt);

    srand((unsigned int)vm_get_sys_time_ms());
}

/* ---------- system event handler ---------- */
void handle_sysevt(VMINT message, VMINT param)
{
    switch (message)
    {
    case VM_MSG_CREATE:
    case VM_MSG_ACTIVE:
        if (layer_hdl < 0)
        {
            layer_hdl = vm_graphic_create_layer(0, 0, screen_w, screen_h, -1);
            layer_buf = vm_graphic_get_layer_buffer(layer_hdl);
            vm_graphic_set_clip(0, 0, screen_w, screen_h);
        }
        game_run();
        break;

    case VM_MSG_INACTIVE:
    case VM_MSG_QUIT:
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
        xor_SetCursorPos(0, 4);
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
        xor_FillRectangle(xor_clipX + LEFT_TEXT_INDENT - 2,
                          29 + 16 * menu_selOption, MM_SELBAR_WIDTH, 11);
        break;

    case STATUS:
        xor_CenterTextOffset("COMMANDER", 9, HEADER_Y, 1 + cmdr_name_length);
        xor_CenterTextOffset(cmdr_name, cmdr_name_length, HEADER_Y, -10);
        xor_SetCursorPos(0, 3);
        xor_lineSpacing = false;
        xor_Print("Present System");
        xor_SetCursorPos(20, 3);
        xor_PrintChar(':');
        gen_PrintName(&currentSeed, true);
        xor_Print("\nHyperspace System");
        xor_SetCursorPos(20, 4);
        xor_PrintChar(':');
        gen_PrintName(&selectedSeed, true);
        xor_Print("\nCondition");
        xor_SetCursorPos(20, 5);
        xor_PrintChar(':');
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
        xor_Print("\n\nEQUIPMENT:");
        upg_DisplayEquipment();
        break;

    case THIS_DATA:
    case SEL_DATA:
        {
            const struct gen_sysData_t* relevantData =
                currentMenu == THIS_DATA ? &thisSystemData : &selectedSystemData;
            const struct gen_seed_t* relevantSeed =
                currentMenu == THIS_DATA ? &currentSeed : &selectedSeed;
            xor_CenterTextOffset("DATA ON", 7, HEADER_Y,
                                 1 + (signed char)strlen(relevantData->name));
            xor_CenterTextOffset(relevantData->name,
                                 (unsigned char)strlen(relevantData->name),
                                 HEADER_Y, -8);
            xor_SetCursorPos(0, 6);
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
        xor_CenterTextOffset(thisSystemData.name,
                             (unsigned char)strlen(thisSystemData.name), HEADER_Y, 12);
        xor_CenterTextOffset("MARKET LINK", 11, HEADER_Y,
                             -1 - (signed char)strlen(thisSystemData.name));
        mkt_PrintMarketTable();
        xor_FillRectangle(DASH_HOFFSET + 5,
                          HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
        break;

    case INVENTORY:
        xor_CenterText(player_condition == DOCKED ? "SELL CARGO" : "CARGO HOLD", 10, HEADER_Y);
        mkt_PrintInventoryTable();
        if (player_condition == DOCKED && !mkt_InventoryEmpty())
        {
            xor_FillRectangle(DASH_HOFFSET + 5,
                              HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
        }
        break;

    case UPGRADES:
        xor_CenterText("OUTFITTING", 10, HEADER_Y);
        upg_PrintOutfittingTable();
        xor_FillRectangle(DASH_HOFFSET + 5,
                          HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
        break;

    default:
        xor_CenterText("UNDEFINED MENU", 14, HEADER_Y);
        break;
    }

    plat_FlushScreen();
}

static bool doMenuInput(void)
{
    unsigned int frameTimer = plat_GetTicks();

    updateKeys();

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
                              29 + 16 * menu_selOption,  MM_SELBAR_WIDTH, 11);
            xor_FillRectangle(xor_clipX + LEFT_TEXT_INDENT - 2,
                              29 + 16 * prevSelOpt, MM_SELBAR_WIDTH, 11);
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
        if (enter == 1) { mkt_Buy(menu_selOption); drawMenu(false); }
        if (menu_selOption != prevSelOpt)
        {
            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
            plat_FlushScreen();
        }
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
            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
            xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
            plat_FlushScreen();
        }
        break;

    case INVENTORY:
        if (player_condition == DOCKED && !mkt_InventoryEmpty())
        {
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
            if (menu_selOption != prevSelOpt)
            {
                xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * prevSelOpt,    DASH_WIDTH - 10, 9);
                xor_FillRectangle(DASH_HOFFSET + 5, HEADER_DIVIDER_Y + 10 + 8 * menu_selOption, DASH_WIDTH - 10, 9);
                plat_FlushScreen();
            }
            if (enter == 1) { mkt_Sell(menu_selOption); drawMenu(true); }
        }
        /* fall through */
    default:
        returnMenu = MAIN;
        break;
    }

    if (gen_crsX != prevCrsX || gen_crsY != prevCrsY)
        gen_RedrawCursorPosition(prevCrsX, prevCrsY);

    if (yequ == 1) currentMenu = returnMenu;

    /* Frame timing */
    while (plat_GetTicks() - frameTimer < FRAME_TIME);

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

static unsigned char titleScreen(unsigned char shipType,
                                  char query[], unsigned char queryLength,
                                  bool acceptEnter)
{
    numShips = 0;
    struct ship_t* titleShip = NewShip(shipType,
                                       (struct vector_t){ 0, 0, TTL_SHIP_START_Z },
                                       Matrix(-256,0,0, 0,0,256, 0,256,0));
    titleShip->pitch = 127;
    titleShip->roll  = 127;

    unsigned char returnVal = 0;

    while (true)
    {
        plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);

        xor_CenterText("---- E L I T E ----", 19, HEADER_Y);
        xor_CenterText(query, queryLength, (unsigned char)(xor_clipY + xor_clipHeight - 3 * HEADER_Y - 8));

        if (titleShip->position.z > TTL_SHIP_END_Z)
            titleShip->position.z -= TTL_SHIP_ZOOM_RATE;
        MoveShip(titleShip);
        DrawShip(titleShip);
        titleShip->orientation = orthonormalize(titleShip->orientation);

        if (!acceptEnter)
        {
            xor_SetCursorPos(5, xor_textRows);
            xor_Print("(N)");
            xor_SetCursorPos(xor_textCols - 10, xor_textRows);
            xor_Print("(Y)");
        }

        plat_FlushScreen();

        updateKeys();

        if (clear == 1) { numShips = 0; return 2; }

        if (!acceptEnter)
        {
            if (yequ == 1)  { returnVal = 0; break; }
            if (graph == 1) { returnVal = 1; break; }
        }
        else if (enter == 1) break;
    }

    numShips = 0;
    return acceptEnter ? 1 : returnVal;
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

static bool nameCmdr(void)
{
    plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);
    xor_CenterText("---- E L I T E ----", 19, HEADER_Y);
    xor_SetCursorPos(1, xor_textRows - 1);
    xor_Print("Commander Name: ");

    unsigned char resetX = xor_cursorX;
    cmdr_name_length = 0;

    do
    {
        plat_FlushScreen();
        updateKeys();

        char typedChar = getChar();
        if (typedChar != '\0' && cmdr_name_length < CMDR_NAME_MAX_LENGTH)
        {
            xor_PrintChar(typedChar);
            cmdr_name[cmdr_name_length] = typedChar;
            cmdr_name_length++;
            cmdr_name[cmdr_name_length] = '\0';
        }

        if (clear == 1)
        {
            if (cmdr_name_length > 0)
            {
                xor_SetCursorPos(resetX, xor_cursorY);
                xor_Print(cmdr_name);
                xor_SetCursorPos(resetX, xor_cursorY);
                cmdr_name_length = 0;
            }
            else return true;
        }
    }
    while (enter == 0);

    return false;
}

static bool run_once(void)
{
    clear = 1; /* prevent immediate exit */

    begin();

    /* Check for saved game */
    int saveExists = plat_FileExists(SAVE_VAR_NAME);

    if (saveExists)
    {
        unsigned char tsResponse = titleScreen(BP_COBRA, "Load Saved Commander?", 21, false);
        if (tsResponse == 2) return false;
        if (tsResponse == 1) loadGame();
        else if (nameCmdr()) return true;
    }
    else if (nameCmdr()) return true;

    /* Always-shown title screen */
    if (titleScreen(BP_MAMBA, "Press ENTER, CMDR.", 18, true) == 2) return false;

    /* Core game loop */
    while (true)
    {
        drawMenu(true);
        while (doMenuInput());

        if (player_condition == DOCKED) saveGame();
        if (toExit) break;

        flt_ResetPlayerCondition();
        doFlight();

        if (player_dead) break;
    }

    return player_dead;
}

/* ---------- game_run: called once per VM_MSG_ACTIVE ---------- */
static void game_run(void)
{
    toExit = false;
    while (run_once());

    vm_exit_app();
}
