#ifndef market_include_file
#define market_include_file

#include "variables.h"

extern unsigned char marketSeed;
extern unsigned char mkt_localQuantities[NUM_TRADE_GOODS];
extern unsigned char inventory[NUM_TRADE_GOODS];

extern unsigned char mkt_qtyValue;
extern unsigned char mkt_qtyGood;
extern bool mkt_qtyBuy;

void mkt_PrintQtyQuery(unsigned char goodIndex, bool toBuy);
bool mkt_BuyAmount(unsigned char goodIndex, unsigned char quantity);

void mkt_ResetLocalMarket();
void mkt_PrintMarketTable();
void mkt_PrintInventoryTable();

// queries the amount then makes the transaction, updating inventory & money
//bool mkt_Buy(unsigned char goodIndex);
//bool mkt_Sell(unsigned char crsPos);

bool mkt_InventoryEmpty();

void mkt_AdjustLegalStatus();
void mkt_GetScanned();
bool mkt_SellAmount(unsigned char goodIndex, unsigned char quantity);
unsigned char mkt_GetInventoryGood(unsigned char row);
unsigned char mkt_GetInventoryCount(void);
unsigned char mkt_GetInventoryGood(unsigned char row);
unsigned char mkt_GetInventoryRowY(unsigned char row);
unsigned char mkt_GetInventoryRow(unsigned char goodIndex);
#endif
