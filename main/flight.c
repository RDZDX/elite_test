#include "platform.h"
#include "xorgfx.h"

#include <stdlib.h>
#include <stdbool.h>

#include "variables.h"
#include "linear.h"
#include "intmath.h"
#include "ship.h"
#include "ship_data.h"
#include "stardust.h"
#include "generation.h"
#include "flight.h"
#include "input.h"
#include "market.h"
#include "upgrades.h"

player_condition_t player_condition = DOCKED;

unsigned char player_speed;
signed char   player_roll;
signed char   player_pitch;

unsigned char player_altitude;
unsigned char player_cabin_temp;

unsigned char player_energy;
unsigned char player_fore_shield;
unsigned char player_aft_shield;

unsigned char player_laser_temp;
bool          player_laser_overheat;
unsigned char laserPulseCounter;
bool          drawLasers;

enum viewDirMode_t viewDirMode;

unsigned char junkAmt;
unsigned char extraSpawnDelay;
unsigned char ecmTimer;

enum {
    STANDBY,
    SEARCHING,
    LOCKED
} missileStatus;
unsigned char missileTarget;

char          flightMsg[FLTMSG_MAX_LENGTH];
unsigned char flightMsgLength;
unsigned char flightMsgTimer;

bool stationSoi;
bool flt_playerToDie;
static unsigned int death_start_time;

/* --------------------------------------------------------------------------
 * Tunnel/launch animations — use plat_* timing
 * -------------------------------------------------------------------------- */

static void flt_DoTunnelAnimation(unsigned char step)
{
    plat_FillRect(xor_clipX, xor_clipY, xor_clipWidth, xor_clipHeight, PLT_COLOR_BLACK);
    xor_Rectangle(xor_clipX, xor_clipY, xor_clipWidth - 1, xor_clipHeight - 1);
    plat_FlushScreen();

    unsigned int timer;

    for (unsigned int inner = 8; inner < 16; inner++)
    {
        for (unsigned int current = inner; current <= 160; current <<= 1)
        {
            timer = plat_GetTicks();

            xor_SteppedCircle(VIEW_HCENTER, VIEW_VCENTER, current, step);
            plat_FlushScreen();

            while (plat_GetTicks() - timer < TUNNEL_FRAME_TIME);
        }
    }

    while (plat_GetTicks() - timer < TUNNEL_HOLD_TIME);

    /* Erase */
    for (unsigned int inner = 8; inner < 16; inner++)
    {
        for (unsigned int current = inner; current <= 160; current <<= 1)
        {
            timer = plat_GetTicks();

            xor_SteppedCircle(VIEW_HCENTER, VIEW_VCENTER, current, step);
            plat_FlushScreen();

            while (plat_GetTicks() - timer < TUNNEL_FRAME_TIME);
        }
    }
}

static void flt_DoLaunchAnimation(void)    { flt_DoTunnelAnimation(8); }
static void flt_DoHyperspaceAnimation(void){ flt_DoTunnelAnimation(2); }

/* -------------------------------------------------------------------------- */

void flt_Init(void)
{
    player_speed = 0;
    player_roll  = 0;
    player_pitch = 0;

    player_altitude    = 96;
    player_cabin_temp  = 30;

    player_missiles      = 3;
    missileStatus        = STANDBY;

    player_energy      = 255;
    player_fore_shield = 255;
    player_aft_shield  = 255;

    player_laser_temp    = 0;
    player_laser_overheat = false;
    laserPulseCounter    = 0;

    viewDirMode    = FRONT;
    flightMsgTimer = 0;
    stationSoi     = true;
    flt_playerToDie = false;
    death_start_time = 0;
}

static void launch(void)
{
    viewDirMode = FRONT;
    mkt_AdjustLegalStatus();

    player_speed = 12;

    stationSoi = true;
    InitShip(&station, BP_CORIOLIS,
             (struct vector_t){ 0, 0, -256 },
             Matrix(256,0,0, 0,256,0, 0,0,256));
    station.hasEcm = true;
    station.roll   = 127;

    InitShip(&planet, PLANET,
             (struct vector_t){ 0, 0, 49152 },
             Matrix(256,0,0, 0,256,0, 0,0,256));

    flt_DoLaunchAnimation();
}

void flt_SetMsg(char message[], unsigned char time)
{
    for (flightMsgLength = 0; message[flightMsgLength] != '\0'; flightMsgLength++)
        flightMsg[flightMsgLength] = message[flightMsgLength];
    flightMsgTimer = time;
}

/* --------------------------------------------------------------------------
 * drawSpaceView — using xorgfx (XOR drawing) for all vector graphics
 * -------------------------------------------------------------------------- */

void drawSpaceView(void)
{
    xor_SetCursorPos(11, 1);
    switch (viewDirMode)
    {
    case FRONT: xor_Print("Front"); break;
    case REAR:  xor_Print("Rear");  break;
    case LEFT:  xor_Print("Left");  break;
    case RIGHT: xor_Print("Right"); break;
    }
    xor_Print(" View");

    xor_Crosshair(VIEW_HCENTER, VIEW_VCENTER, CRS_SPREAD, CRS_SIZE);

    FlipAxes(viewDirMode);
    DrawShip(&sun);
    DrawShip(&planet);
    for (unsigned char i = 0; i < numShips; i++) DrawShip(&ships[i]);
    RestoreAxes(viewDirMode);

    stardust_Move(viewDirMode, player_speed, player_pitch, player_roll);
    stardust_Draw();

    if (flightMsgTimer == 0) return;
    flightMsgTimer--;
}

/* --------------------------------------------------------------------------
 * Radar dot — drawn with plat_* colored pixels
 * -------------------------------------------------------------------------- */

static void drawRadarDot(const struct ship_t *ship)
{
    if (ship->isExploding) return;

    if (ship->position.x < -63 * 256) return;
    if (ship->position.x >  63 * 256) return;
    if (ship->position.y < -63 * 256) return;
    if (ship->position.y >  63 * 256) return;
    if (ship->position.z < -63 * 256) return;
    if (ship->position.z >  63 * 256) return;

    int const x = RADAR_HCENTER + ship->position.x / RADAR_XSCALE;
    int const y = RADAR_VCENTER - ship->position.z / RADAR_ZSCALE;
    int dy = ship->position.y / RADAR_YSCALE;

    /* Clip to dashboard strip */
    if (y + dy < DASH_VOFFSET + 2)
        dy = DASH_VOFFSET + 2 - y;
    else if (y + dy > screen_h - 2)
        dy = screen_h - 2 - y;

    VMUINT16 dotColor = ship->isHostile ? PLT_COLOR_YELLOW : PLT_COLOR_GREEN;
    plat_Line(x, y, x, y + dy, dotColor);
    plat_SetPixel(x,     y + dy, dotColor);
    plat_SetPixel(x - 1, y + dy, dotColor);
}

/* --------------------------------------------------------------------------
 * drawDashboard — simplified, no TI sprites
 * -------------------------------------------------------------------------- */

void drawDashboard(void)
{
    const int dv = DASH_VOFFSET;   /* top of dashboard strip */
    const int dw = DASH_WIDTH;     /* = screen_w = 320 */

    /* Background */
    plat_FillRect(DASH_HOFFSET, dv, dw, DASH_HEIGHT, PLT_COLOR_BLACK);
    /* Top divider */
    plat_Line(DASH_HOFFSET, dv, DASH_HOFFSET + dw - 1, dv, PLT_COLOR_WHITE);
    /* Mid divider */
    plat_Line(DASH_HOFFSET, dv + 30, DASH_HOFFSET + dw - 1, dv + 30, PLT_COLOR_WHITE);

    /* ===== TOP HALF (dv+2 .. dv+28) ===== */

    /* --- Left: fore shield, aft shield, fuel --- */
    /* Row 1: Fore shield label + bar */
    xor_SetCursorPos(0, (dv + 2) / 8);
    xor_Print("F:");
    {
        VMUINT16 col = player_fore_shield < 64 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
        plat_FillRect(18, dv + 3, player_fore_shield / 4 + 1, 4, col);
    }
    /* Row 2: Aft shield */
    xor_SetCursorPos(0, (dv + 10) / 8);
    xor_Print("A:");
    {
        VMUINT16 col = player_aft_shield < 64 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
        plat_FillRect(18, dv + 11, player_aft_shield / 4 + 1, 4, col);
    }
    /* Row 3: Fuel */
    xor_SetCursorPos(0, (dv + 18) / 8);
    xor_Print("~:");
    {
        unsigned char fb = player_fuel <= 64 ? player_fuel : 64;
        plat_FillRect(18, dv + 19, fb, 4, PLT_COLOR_YELLOW);
    }

    /* --- Center: radar --- */
    if (stationSoi) drawRadarDot(&station);
    for (unsigned char i = 0; i < numShips; i++) drawRadarDot(&ships[i]);

    /* --- Right: speed, roll, pitch --- */
    const int rbase = dw * 3 / 4;
    xor_SetCursorPos(rbase / 8, (dv + 2) / 8);
    xor_Print("S:");
    {
        unsigned char spd = (unsigned char)(62 * player_speed / PLAYER_MAX_SPEED);
        plat_FillRect(rbase + 18, dv + 3, spd, 4, PLT_COLOR_YELLOW);
    }
    xor_SetCursorPos(rbase / 8, (dv + 10) / 8);
    xor_Print("R:");
    {
        signed char roff = player_roll / 2;
        if (roff ==  16) roff--;
        if (roff == -16) roff++;
        plat_FillRect(rbase + 18 + 16 + roff, dv + 11, 4, 4, PLT_COLOR_YELLOW);
    }
    xor_SetCursorPos(rbase / 8, (dv + 18) / 8);
    xor_Print("P:");
    {
        signed char poff = player_pitch * 2;
        if (poff ==  16) poff--;
        if (poff == -16) poff++;
        plat_FillRect(rbase + 18 + 16 + poff, dv + 19, 4, 4, PLT_COLOR_YELLOW);
    }

    /* ===== BOTTOM HALF (dv+32 .. dv+58) ===== */

    /* --- Left: cabin temp, laser temp, altitude --- */
    xor_SetCursorPos(0, (dv + 32) / 8);
    xor_Print("C:");
    {
        VMUINT16 col = player_cabin_temp > 192 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
        plat_FillRect(18, dv + 33, player_cabin_temp / 4, 4, col);
    }
    xor_SetCursorPos(0, (dv + 40) / 8);
    xor_Print("L:");
    {
        VMUINT16 col = player_laser_temp > 192 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
        plat_FillRect(18, dv + 41, player_laser_temp / 4, 4, col);
    }
    xor_SetCursorPos(0, (dv + 48) / 8);
    xor_Print("H:");
    {
        VMUINT16 col = player_altitude < 32 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
        plat_FillRect(18, dv + 49, 1 + player_altitude / 4, 4, col);
    }

    /* --- Center: SOI indicator + compass --- */
    if (stationSoi)
        plat_FillRect(SOI_INDIC_POS_X, dv + 33, 4, 4, PLT_COLOR_WHITE);
    {
        const struct vector_t cv = normalize((stationSoi ? station : planet).position);
        VMUINT16 col = cv.z > 0 ? PLT_COLOR_YELLOW : PLT_COLOR_GREEN;
        //plat_SetPixel(COMPASS_HCENTER + cv.x / COMPASS_SCALE,
        //              dv + 45 + cv.y / COMPASS_SCALE, col);
        plat_FillRect(COMPASS_HCENTER + cv.x / COMPASS_SCALE,
                      dv + 45 + cv.y / COMPASS_SCALE, 2, 2, col);
    }

    /* --- Right: energy banks + missiles --- */
    {
        unsigned char rem = player_energy / 2;
        int barY = dv + 57;
        while (rem > 0)
        {
            unsigned char seg = rem < 32 ? rem : 32;
            VMUINT16 col = seg <= 4 ? PLT_COLOR_RED : PLT_COLOR_YELLOW;
            plat_FillRect(rbase, barY, seg, 3, col);
            rem -= seg;
            barY -= 5;
        }
    }
    for (unsigned char i = 0; i < player_missiles; i++)
    {
        VMUINT16 col = PLT_COLOR_GREEN;
        if (i + 1 == player_missiles)
        {
            if      (missileStatus == LOCKED)    col = PLT_COLOR_RED;
            else if (missileStatus == SEARCHING) col = PLT_COLOR_YELLOW;
        }
        plat_FillRect(rbase - 8 - 8 * i, dv + 50, 6, 6, col);
    }
}

/* --------------------------------------------------------------------------
 * Remaining flight functions — same logic as original, with clock() replaced
 * -------------------------------------------------------------------------- */

bool flt_CanJump(void)
{
    for (unsigned char i = 0; i < numShips; i++)
    {
        if (ships[i].shipType != BP_CORIOLIS && ships[i].shipType < BP_ASTEROID
                && !ships[i].isExploding)
        {
            flt_SetMsg("Interference!", FLTMSG_MED_TIME);
            return false;
        }
    }

    if (!stationSoi)
    {
        if (sun.position.z < 0x020000 && planet.position.z < 0x020000)
        {
            flt_SetMsg("Dangerous trajectory!", FLTMSG_MED_TIME);
            return false;
        }
    }
    else if (planet.position.z > 0
          && intpow(planet.position.x >> 8, 2)
           + intpow(planet.position.y >> 8, 2) <= 9276)
    {
        flt_SetMsg("Dangerous trajectory!", FLTMSG_MED_TIME);
        return false;
    }

    if (player_speed != PLAYER_MAX_SPEED)
    {
        flt_SetMsg("Throttle up!", FLTMSG_MED_TIME);
        return false;
    }
    return true;
}

static bool flt_CanHyperdrive(void)
{
    if (prgm && !player_upgrades.galacticHyperdrive) return false;
    if (currentSeed.a == selectedSeed.a
     || currentSeed.b == selectedSeed.b
     || currentSeed.c == selectedSeed.c)
    {
        flt_SetMsg("No destination set!", FLTMSG_MED_TIME);
        return false;
    }
    return flt_CanJump();
}

static void flt_DoHyperdrive(bool intergalactic)
{
    stationSoi      = false;
    drawCycle       = 0;
    junkAmt         = 0;
    extraSpawnDelay = 0;

    if (intergalactic) gen_ChangeGalaxy();
    else               gen_ChangeSystem();

    flt_DoHyperspaceAnimation();
}

static void flt_TryInSystemJump(void)
{
    if (stationSoi) return;
    if (!flt_CanJump()) return;

    planet.position.z -= 0x010000;
    sun.position.z    -= 0x010000;
    drawCycle = 0;
}

static void flt_TryLasers(void)
{
    if (player_laser_overheat)
    {
        player_laser_overheat = player_laser_temp > 0;
        return;
    }
    if (player_laser_temp >= 242) { player_laser_overheat = true; return; }
    if (viewDirMode == LEFT || viewDirMode == RIGHT) return;

    FlipAxes(viewDirMode);

    if (laserPulseCounter > 0) { RestoreAxes(viewDirMode); return; }
    if (player_lasers[viewDirMode] == PULSE) laserPulseCounter += PULSE_LASER_INTERVAL;

    player_laser_temp += 8;
    player_energy     -= 1;
    drawLasers         = true;

    unsigned char const laserPower = 255;

    for (unsigned char i = 0; i < numShips; i++)
    {
        if (ships[i].position.z <= 0) continue;
        if (ships[i].shipType == PLANET) continue;
        if (ships[i].shipType == SUN)    continue;
        if (ships[i].isExploding) continue;

        const unsigned char* bpGeneral = bp_header_vectors[ships[i].shipType][BP_GENERAL];
        const unsigned int areaSquared = (bpGeneral[2] << 8) + bpGeneral[1];

        const unsigned int offsetSquared = ships[i].position.x * ships[i].position.x
                                         + ships[i].position.y * ships[i].position.y;
        if (offsetSquared > areaSquared) continue;

        DamageShip(&ships[i], laserPower);
    }

    RestoreAxes(viewDirMode);
}

static void flt_TryFindMissileTarget(void)
{
    if (!player_missiles) return;

    for (missileTarget = 0; missileTarget < numShips; missileTarget++)
    {
        if (ships[missileTarget].position.z <= 0) continue;
        if (ships[missileTarget].shipType > PLANET) continue;
        if (ships[missileTarget].isExploding) continue;

        const unsigned int z = intabs(ships[missileTarget].position.z);
        if (z > MAX_MISSILE_LOCK_RANGE) continue;
        if (intabs(ships[missileTarget].position.x) > (int)(z >> 3)) continue;
        if (intabs(ships[missileTarget].position.y) > (int)(z >> 3)) continue;

        missileStatus = LOCKED;
        break;
    }
}

static void flt_LaunchMissile(void)
{
    if (numShips + 1 >= MAX_SHIPS) return;

    struct ship_t* missile = NewShip(BP_MISSILE,
                                     (struct vector_t){ 0, MISSILE_LAUNCH_Y, MISSILE_LAUNCH_Z },
                                     Matrix(256,0,0, 0,256,0, 0,0,-256));
    missile->speed  = MISSILE_LAUNCH_SPEED;
    missile->target = missileTarget;
    missile->aggro  = 64;

    player_missiles--;
    if (ships[missileTarget].shipType != BP_ASTEROID) ships[missileTarget].isHostile = true;
    missileStatus = STANDBY;
}

bool doFlightInput(void)
{
    if (second > 0 && player_speed < PLAYER_MAX_SPEED) player_speed++;
    else if (alpha > 0 && player_speed > 0)             player_speed--;

    if (yequ == 0)
    {
        if (left > 0)       player_roll -= 6;
        else if (right > 0) player_roll += 6;
        if (player_roll >  32) player_roll =  32;
        if (player_roll < -32) player_roll = -32;

        if (up > 0)         player_pitch += 2;
        else if (down > 0)  player_pitch -= 2;
        if (player_pitch >  8) player_pitch =  8;
        if (player_pitch < -8) player_pitch = -8;
    }
    else
    {
        if      (up > 0)    viewDirMode = FRONT;
        else if (left > 0)  viewDirMode = LEFT;
        else if (right > 0) viewDirMode = RIGHT;
        else if (down > 0)  viewDirMode = REAR;
    }

    if (player_pitch < 0) player_pitch++;
    else if (player_pitch > 0) player_pitch--;
    if (player_roll < 0)  player_roll += 2;
    else if (player_roll > 0) player_roll -= 2;

    if (((prgm | clear) > 0) && flt_CanHyperdrive())
    {
        if      (clear >= STAR_JUMP_HOLD_TIME)   flt_DoHyperdrive(false);
        else if (prgm  >= GALAXY_JUMP_HOLD_TIME) flt_DoHyperdrive(true);
        else flt_SetMsg("Preparing jump...", 2);
    }
    else if (vars == 1) flt_TryInSystemJump();

    if (mode > 0)  flt_TryLasers();
    if (graphVar == 1 && player_missiles > 0)
    {
        if      (missileStatus == STANDBY)  missileStatus = SEARCHING;
        else if (missileStatus == LOCKED)   flt_LaunchMissile();
    }
    if (apps == 1) missileStatus = STANDBY;

    if (graph == 1) { currentMenu = MAIN; return false; }
    return true;
}

static void flt_TrySpawnStation(void)
{
    const struct vector_t stationPos =
        add(planet.position, mul(getCol(planet.orientation, 2), 2 * 96));
    if (intabs(stationPos.x) > 49152) return;
    if (intabs(stationPos.y) > 49152) return;
    if (intabs(stationPos.z) > 49152) return;

    InitShip(&station, BP_CORIOLIS, stationPos, planet.orientation);
    station.orientation.a[6] *= -1;
    station.orientation.a[7] *= -1;
    station.orientation.a[8] *= -1;
    station.roll = 127;
    stationSoi   = true;
}

static void flt_UpdatePlayerAltitude(void)
{
    for (unsigned char target = PLANET; true; target++)
    {
        if (target > SUN) { player_altitude = 255; return; }

        unsigned char targetIndex = 0;
        bool found = true;
        while (ships[targetIndex].shipType != target)
        {
            targetIndex++;
            if (targetIndex >= numShips) { found = false; break; }
        }
        if (!found) continue;

        const signed int x = ships[targetIndex].position.x / 256;
        const signed int y = ships[targetIndex].position.y / 256;
        const signed int z = ships[targetIndex].position.z / 256;

        unsigned int altitude = intsqrt((unsigned int)(x*x + y*y + z*z));
        if (altitude >= 255 + 96) continue;

        player_altitude = (unsigned char)(altitude - 96);
        if (altitude <= 96) flt_Death();
        break;
    }
}

static void flt_UpdateCabinTemperature(void)
{
    player_cabin_temp = 30;
    if (stationSoi) return;

    unsigned int const sunX = intabs(sun.position.x) >> 8;
    unsigned int const sunY = intabs(sun.position.y) >> 8;
    unsigned int const sunZ = intabs(sun.position.z) >> 8;
    if (sunX > 256 || sunY > 256 || sunZ > 256) return;

    unsigned int sunDistance = sunX * sunX + sunY * sunY + sunZ * sunZ;
    if (sunDistance > 256) return;

    player_cabin_temp += (unsigned char)(sunDistance ^ 0xff);
    if (player_cabin_temp < 30) flt_Death();
}

static unsigned char flt_CheckForDocking(void)
{
    if (player_speed == 0) return 1;
    if (station.position.z < 0) return 1;

    bool successful = true;
    if (station.orientation.a[8] > -115)                         successful = false;
    if (station.position.z < 119)                                successful = false;
    if (station.orientation.a[3] < 107 && station.orientation.a[3] > -107) successful = false;
    if (station.isHostile)                                        successful = false;

    if (successful) return 0;

    if (player_speed <= 5) { player_speed = 0; flt_DamagePlayer(15, FRONT); }
    else                     flt_playerToDie = true;
    return 1;
}

static struct vector_t flt_GetSpawnPos(void)
{
    struct vector_t sp = { rand() % 256, rand() % 256, 38 * 256 };
    if (rand() % 2) sp.x += 512;
    if (sp.x & 0x80) sp.x = -sp.x;
    if (sp.y & 0x80) sp.y = -sp.y;
    return sp;
}

void flt_ResetPlayerCondition(void)
{
    if (player_condition == DOCKED) launch();
    player_condition = GREEN;
    for (unsigned char i = 0; i < numShips; i++)
        if (ships[i].shipType != BP_ASTEROID) player_condition = YELLOW;
    if (player_condition == GREEN) return;
    if (player_energy < 255) player_condition = RED;
}

static bool flt_UpdateShip(struct ship_t *ship)
{
    ship->position.z -= player_speed;

    signed int oldY = ship->position.y - (player_roll  * ship->position.x) / 256;
    ship->position.z += (player_pitch * oldY) / 256;
    ship->position.y  = oldY - (player_pitch * ship->position.z) / 256;
    ship->position.x += (player_roll  * ship->position.y) / 256;

    for (unsigned char j = 0; j < 9; j += 3)
    {
        ship->orientation.a[j+1] -= player_roll  * ship->orientation.a[j+0] / 256;
        ship->orientation.a[j+0] += player_roll  * ship->orientation.a[j+1] / 256;
        ship->orientation.a[j+1] -= player_pitch * ship->orientation.a[j+2] / 256;
        ship->orientation.a[j+2] += player_pitch * ship->orientation.a[j+1] / 256;
    }

    MoveShip(ship);

    if (player_dead) return false;

    if (ship->position.x >  191) return false;
    if (ship->position.x < -191) return false;
    if (ship->position.y >  191) return false;
    if (ship->position.y < -191) return false;
    if (ship->position.z >  191) return false;
    if (ship->position.z < -191) return false;

    if (ship != &station) return false;
    if (flt_CheckForDocking()) return false;

    player_condition = DOCKED;
    currentMenu      = STATUS;
    numShips         = 0;
    player_pitch     = 0;
    player_roll      = 0;
    flt_DoLaunchAnimation();
    return true;
}

static void flt_TrySpawnShips(void)
{
    if (stationSoi)          return;
    if (numShips == MAX_SHIPS) return;

    if (rand() % 256 < 35)
    {
        if (junkAmt >= 3) return;
        unsigned char junkType = rand() % 2 ? BP_COBRA
                               : rand() % 256 < 10 ? BP_CANISTER
                               : BP_ASTEROID;
        struct ship_t* junk = NewShip(junkType, flt_GetSpawnPos(),
                                      Matrix(256,0,0, 0,256,0, 0,0,256));
        if (junkType == BP_COBRA) return;
        const signed char rollRand = (signed char)((rand() % 128) | 0x6f);
        junk->roll = rand() % 2 ? rollRand : -rollRand;
        if (rand() % 2) junk->speed = (unsigned char)(rand() % 16 + 16);
        else            junk->pitch = rand() % 2 ? 127 : -128;
        junkAmt++;
    }
    else
    {
        bool copNearby = false;
        for (unsigned char i = 0; i < numShips; i++)
            if (ships[i].shipType == BP_VIPER) copNearby = true;
        if (copNearby) mkt_GetScanned();

        if (rand() % 256 < player_outlaw)
        {
            struct ship_t* cop = NewShip(BP_VIPER, flt_GetSpawnPos(),
                                         Matrix(256,0,0, 0,256,0, 0,0,-256));
            cop->aggro = 60;
            cop->isHostile = true;
            return;
        }

        if (extraSpawnDelay > 0) { extraSpawnDelay--; return; }
        if ((rand() & 0x07) >= thisSystemData.government) return;

        if ((rand() % 256) < 100)
        {
            extraSpawnDelay++;
            const unsigned char hunterType = rand() % 2 ? BP_COBRA : BP_PYTHON;
            struct ship_t* enemy = NewShip(hunterType, flt_GetSpawnPos(),
                                           Matrix(256,0,0, 0,256,0, 0,0,-256));
            enemy->isHostile = true;
            enemy->aggro     = 28;
            enemy->hasEcm    = rand() % 256 >= 200;
            enemy->speed     = 12;
            return;
        }

        for (unsigned char np = rand() % 4 + 1; np > 0; np--)
        {
            extraSpawnDelay++;
            const unsigned char pt = rand() % 2 ? BP_MAMBA : BP_SIDEWINDER;
            struct ship_t* enemy = NewShip(pt, flt_GetSpawnPos(),
                                           Matrix(256,0,0, 0,256,0, 0,0,-256));
            enemy->isHostile = true;
            enemy->aggro     = rand() % 64;
            enemy->hasEcm    = rand() % 256 <= 10;
            enemy->speed     = 12;
            if (numShips == MAX_SHIPS) break;
        }
    }
}

void flt_DoFrame(bool dashboardVisible)
{
    /* Black background */
    plat_FillRect(0, 0, screen_w, screen_h, PLT_COLOR_BLACK);

    /* Outer white frame */
    xor_Rectangle(DASH_HOFFSET, 0, DASH_WIDTH - 1, DASH_VOFFSET - 1);

    if (drawCycle == 0) flt_TrySpawnShips();
    if (!stationSoi && drawCycle % 64 == 0) flt_TrySpawnStation();

    switch (drawCycle % 8)
    {
    case 0: flt_UpdatePlayerAltitude();    break;
    case 4: flt_UpdateCabinTemperature();  break;
    default: break;
    }

    if (laserPulseCounter > 0) laserPulseCounter--;
    if (player_laser_temp > 0) player_laser_temp--;
    if (player_energy < 255)   player_energy++;
    if (ecmTimer > 0)          ecmTimer--;

    if (missileStatus == SEARCHING) flt_TryFindMissileTarget();

    const unsigned char thisTimeShip = drawCycle % (MAX_SHIPS + 1);
    if (thisTimeShip < MAX_SHIPS)
        ships[drawCycle % MAX_SHIPS].orientation = orthonormalize(ships[drawCycle % MAX_SHIPS].orientation);
    else
    {
        station.orientation = orthonormalize(station.orientation);
        planet.orientation  = orthonormalize(planet.orientation);
    }

    DoAI(drawCycle % MAX_SHIPS);
    DoAI((drawCycle + MAX_SHIPS / 2) % MAX_SHIPS);

    flt_UpdateShip(&planet);
    if (flt_UpdateShip(&station)) return;
    for (unsigned char i = 0; i < numShips; i++) flt_UpdateShip(&ships[i]);

    drawSpaceView();

    if (dashboardVisible) drawDashboard();

    if (!player_dead && flt_playerToDie) { flt_Death(); return; }

    if (drawLasers)
    {
        const unsigned char laserY = (unsigned char)(VIEW_VCENTER - LASER_LINE_SPREAD / 2 + rand() % LASER_LINE_SPREAD);
        const unsigned char laserX = (unsigned char)(VIEW_HCENTER - LASER_LINE_SPREAD / 2 + rand() % LASER_LINE_SPREAD);

        xor_Line(laserX, laserY, LASER_LINE_X_ONE + DASH_HOFFSET, DASH_VOFFSET);
        xor_Line(laserX, laserY, LASER_LINE_X_TWO + DASH_HOFFSET, DASH_VOFFSET);
        xor_Line(laserX, laserY, DASH_WIDTH - LASER_LINE_X_ONE + DASH_HOFFSET, DASH_VOFFSET);
        xor_Line(laserX, laserY, DASH_WIDTH - LASER_LINE_X_TWO + DASH_HOFFSET, DASH_VOFFSET);

        drawLasers = false;
    }
}

void doFlight(void)
{
    flt_playerToDie = false;
}

void flt_DamagePlayer(unsigned char amount, bool fromBack)
{
    unsigned char* relevantShield = fromBack ? &player_aft_shield : &player_fore_shield;

    if (amount < *relevantShield) { *relevantShield -= amount; return; }

    amount -= *relevantShield;
    *relevantShield = 0;

    if (amount < player_energy) { player_energy -= amount; return; }

    flt_playerToDie = true;
}

void flt_Death(void)
{
    if (player_dead)
    {
        if (death_start_time == 0) death_start_time = plat_GetTicks();
        return;
    }

    death_start_time = plat_GetTicks();

    player_dead = true;
    drawCycle   = 255;

    for (unsigned char i = 0; i < NUM_PLAYER_DEATH_CANS; i++)
    {
        struct ship_t* can = NewShip(BP_CANISTER,
                                     (struct vector_t){ 0, 0, 0 },
                                     Matrix(256,0,0, 0,256,0, 0,0,256));
        can->speed       = player_speed / 4;
        can->pitch       = 127;
        can->roll        = 127;
        can->isExploding = rand() % 2;
    }

    player_speed = 0;
}

bool flt_DeathTick(void)
{
    flt_DoFrame(false);
    xor_SetCursorPos(9, 9);
    xor_Print("GAME OVER");
    plat_FlushScreen();
    return (plat_GetTicks() - death_start_time) >= DEATH_SCREEN_TIME;
}
