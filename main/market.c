#include <stdlib.h>
#include <stdbool.h>

#include "platform.h"
#include "market.h"
#include "generation.h"
#include "upgrades.h"
#include "xorgfx.h"
#include "variables.h"
#include "input.h"

const struct mkt_tradeGood_t {
	unsigned char basePrice;
	signed char econFactor;
	char unitSymbol;
	unsigned char baseQuantity;
	unsigned char mask;
} tradeGoods[NUM_TRADE_GOODS] = {
	{  19, -2, 't',   6, 0x01 },
	{  20, -1, 't',  10, 0x03 },
	{  65, -3, 't',   2, 0x07 },
	{  40, 11, 't', 226, 0x07 },
	{  83, -5, 't', 251, 0x0f },
	{ 196,  8, 't',  54, 0x03 },
	{ 235, 29, 't',   8, 0x78 },
	{ 154, 14, 't',  56, 0x03 },
	{ 117,  6, 't',  40, 0x07 },
	{  78,  1, 't',  17, 0x1f },
	{ 124, 13, 't',  29, 0x07 },
	{ 176, -9, 't', 220, 0x3f },
	{  32, -1, 't',  53, 0x03 },
	{  97, -1, 'k',  66, 0x07 },
	{ 171, -2, 'k',  55, 0x1f },
	{  45, -1, 'g', 250, 0x0f },
	{  53, 15, 'g', 192, 0x07 }
};

const char productNames[NUM_TRADE_GOODS][13] = {
	"Food",
	"Textiles",
	"Radioactives",
	"Chemicals",
	"Liquor/Wines",
	"Luxuries",
	"Narcotics",
	"Computers",
	"Machinery",
	"Alloys",
	"Firearms",
	"Furs",
	"Minerals",
	"Gold",
	"Platinum",
	"Gemstones",
	"Alien Items"
};

unsigned char mkt_localPrices[NUM_TRADE_GOODS];
unsigned char mkt_localQuantities[NUM_TRADE_GOODS];
unsigned char inventory[NUM_TRADE_GOODS];

unsigned char marketSeed;

void mkt_ResetLocalMarket()
{
	for (unsigned char i = 0; i < NUM_TRADE_GOODS; i++)
	{
		mkt_localPrices[i] = 
			tradeGoods[i].basePrice 
			+ (marketSeed & tradeGoods[i].mask) 
			+ thisSystemData.economy * tradeGoods[i].econFactor;

		const signed int tempQuantity =
			tradeGoods[i].baseQuantity
			+ (marketSeed & tradeGoods[i].mask)
			- thisSystemData.economy * tradeGoods[i].econFactor;
		mkt_localQuantities[i] = tempQuantity < 0 ? 0 : tempQuantity % 64;
	}
}

void mkt_PrintMarketTable()
{

        unsigned char oldHeight = xor_rowHeight;
        unsigned char oldxor_textYOffset = xor_textYOffset;
        xor_textYOffset = -5;
        xor_rowHeight = 9;

	// headings
//	xor_SetCursorPos(1, 3); //3
//	xor_Print("PRODUCT");
//	xor_SetCursorPos(16, 3); //3
//	xor_Print("PRICE");
//	xor_SetCursorPos(23, 3); //3
//	xor_Print("AMOUNT");

//        xor_rowHeight = 9;    /* 8px font + 1px gap */

	// table body
	for (unsigned char i = 0; i < NUM_TRADE_GOODS; i++)
	{
		const unsigned char y = i + 3; //4

		xor_SetCursorPos(0, y);
		xor_Print(productNames[i]);

		xor_SetCursorPos(14, y);
		xor_PrintUInt24(mkt_localPrices[i] * 4 / 10, 3);
		xor_PrintChar('.');
		xor_PrintUInt8(mkt_localPrices[i] * 4 % 10, 1);
		xor_Print(" Cr");

		if (mkt_localQuantities[i] > 0)
		{
			xor_SetCursorPos(24, y);
			xor_PrintUInt8(mkt_localQuantities[i], 2);
			xor_PrintChar(tradeGoods[i].unitSymbol);
			if (tradeGoods[i].unitSymbol == 'k') xor_PrintChar('g');
		}
		else
		{
			xor_SetCursorPos(25, y);
			xor_PrintChar('-');
		}
	}

        xor_rowHeight = oldHeight;
        xor_textYOffset = oldxor_textYOffset;
}

void mkt_PrintInventoryTable()
{
	const unsigned char cargoCap = player_upgrades.largeCargoBay ? 35 : 25;

	// capacity
	xor_SetCursorPos(24, 1);
	xor_PrintUInt8(cargoCap - player_cargo_space, 2);
	xor_PrintChar('/');
	xor_PrintUInt8(cargoCap, 2);
	xor_PrintChar('t');

	// headings
	xor_SetCursorPos(1, 3);
	xor_Print("PRODUCT");
	xor_SetCursorPos(23, 3);
	xor_Print("AMOUNT");

	unsigned char y = 4;
	for (unsigned char i = 0; i < NUM_TRADE_GOODS; i++)
	{
		if (inventory[i] == 0) continue;

		xor_SetCursorPos(0, y);
		xor_Print(productNames[i]);
		xor_SetCursorPos(24, y);
		xor_PrintUInt8(inventory[i], 2);
		xor_PrintChar(tradeGoods[i].unitSymbol);
		if (tradeGoods[i].unitSymbol == 'k') xor_PrintChar('g');

		y++;
	}
}

static void mkt_PrintQtyQuery(unsigned char const goodIndex, bool const toBuy)
{
//	xor_SetCursorPos(0, NUM_TRADE_GOODS + 4);
        xor_SetCursorPos(0, NUM_TRADE_GOODS + 5);
	xor_Print("Qty ");
	xor_Print(productNames[goodIndex]);
	xor_Print(" to ");
	xor_Print(toBuy ? "buy" : "sell");
	xor_Print("? ");
}

static void mkt_PrintQueryQty(unsigned char const x, unsigned char const quantity)
{
//	xor_SetCursorPos(x, NUM_TRADE_GOODS + 4);
        xor_SetCursorPos(x, NUM_TRADE_GOODS + 5);
	xor_PrintUInt24Adaptive(quantity);
}

unsigned char mkt_AskForQuantity(unsigned char goodIndex, bool toBuy)
{
	mkt_PrintQtyQuery(goodIndex, toBuy);
	const unsigned char resetX = xor_cursorX;
	mkt_PrintQueryQty(resetX, 0);
	plat_FlushScreen();

	unsigned int quantity = 0;
	bool prevEnter = (enter > 0);

	while (true)
	{
		unsigned int frameTimer = plat_GetTicks();

		updateKeys();

		if (clear > 0)
		{
			mkt_PrintQtyQuery(goodIndex, toBuy);
			mkt_PrintQueryQty(resetX, quantity);
			return 0;
		}

		if (enter > 0 && !prevEnter) break;
		prevEnter = (enter > 0);

		unsigned char prevQty = (unsigned char)quantity;
		unsigned char digit = 0xff;

		/* Map num keys to digits */
		if (prgm  == 1) digit = 2;
		if (vars  == 1) digit = 3;
		if (apps  == 1) digit = 4;
		if (graphVar == 1) digit = 5;
		if (math  == 1) digit = 6;
		if (del   == 1) digit = 7;
		if (stat  == 1) digit = 8;
		if (mode  == 1) digit = 0;
		if (alpha == 1) digit = 1;

		if (digit != 0xff)
		{
			quantity = quantity * 10 + digit;
			if (quantity > 255) quantity = prevQty;
		}

		mkt_PrintQueryQty(resetX, prevQty);
		mkt_PrintQueryQty(resetX, (unsigned char)quantity);

		while (plat_GetTicks() - frameTimer < FRAME_TIME);

		plat_FlushScreen();
	}

	return (unsigned char)quantity;
}

bool mkt_Buy(unsigned char goodIndex)
{
	unsigned char const quantity = mkt_AskForQuantity(goodIndex, true);

	if (quantity > mkt_localQuantities[goodIndex]) return false; // not enough to buy
	
	// check if sufficient cargo hold space available
	if (tradeGoods[goodIndex].unitSymbol == 't')
	{
		if (quantity > player_cargo_space) return false;
	}
	else
	{
		if (inventory[goodIndex] + quantity > 200) return false;
	}

	unsigned int price = (unsigned int)mkt_localPrices[goodIndex] * quantity * 4;
	if (price <= player_money) 
	{
		inventory[goodIndex] += quantity;
		mkt_localQuantities[goodIndex] -= quantity;
		player_money -= price;
		if (tradeGoods[goodIndex].unitSymbol == 't') player_cargo_space -= quantity;
		return true;
	}
	else return false; // can't afford
}

bool mkt_Sell(unsigned char crsPos)
{
	unsigned char goodIndex;
	unsigned char toGo = crsPos + 1;
	for (unsigned char i = 0; true; i++)
	{
		if (inventory[i] > 0) toGo--;
		if (toGo == 0)
		{
			goodIndex = i;
			break;
		}
	}

	unsigned char const quantity = mkt_AskForQuantity(goodIndex, false);

	if (quantity > inventory[goodIndex]) return false; // not enough to sell

	player_money += (unsigned int)mkt_localPrices[goodIndex] * quantity * 4;
	if (tradeGoods[goodIndex].unitSymbol == 't') player_cargo_space += quantity;
	inventory[goodIndex] -= quantity;
	return true;
}

bool mkt_InventoryEmpty()
{
	for (unsigned char i = 0; i < NUM_TRADE_GOODS; i++)
	{
		if (inventory[i] > 0) return false;
	}

	return true;
}

void mkt_AdjustLegalStatus()
{
			  // 2 *   narcotics  +   firearms
	player_outlaw |= 2 * inventory[6] + inventory[10];
}

void mkt_GetScanned()
{
	player_outlaw |= 2 * (2 * inventory[6] + inventory[10]);
}
