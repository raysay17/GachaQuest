#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include "shop.h"
#include "ui.h"
#include "util.h"

static const char *const shopItems[] = { "Beli Potion", "Jual Item", "Kembali" };
static const int shopNums[] = { 1, 2, 0 };

/* Sell price follows the rarity table (same values as DUPLICATE_PRICE_*). */
static int sellPrice(int rarity)
{
    switch (rarity) {
        case RARITY_RARE: return DUPLICATE_PRICE_RARE;
        case RARITY_EPIC: return DUPLICATE_PRICE_EPIC;
        case RARITY_LEGENDARY: return DUPLICATE_PRICE_LEGENDARY;
        default: return DUPLICATE_PRICE_COMMON;
    }
}

static const char *rarityLabel(int rarity)
{
    switch (rarity) {
        case RARITY_RARE: return "Rare";
        case RARITY_EPIC: return "Epic";
        case RARITY_LEGENDARY: return "Legendary";
        default: return "Common";
    }
}

/* Result of an action, shown as the system message of the next frame (piped
   runs print it before the next menu prompt). */
static void report(const char *fmt, ...)
{
    char text[128];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(text, sizeof text, fmt, ap);
    va_end(ap);
    set_message("%s", text);
}

static void drawShop(void *ctx, int sel)
{
    const Player *p = ctx;
    char info[96];

    snprintf(info, sizeof info, "Gold: %d   Potion: %d/%d   Harga potion: %d gold",
             p->gold, p->potion, MAX_POTION, POTION_COST);
    ui_screen_begin("SHOP", info);
    ui_menu_v(UI_CONTENT_ROW, 4, shopItems, shopNums, 3, sel);
    ui_screen_end(NULL);
}

static void buyPotion(Player *p)
{
    if (p->potion >= MAX_POTION) {
        report("Stok potion penuh (maksimal %d).", MAX_POTION);
        return;
    }
    if (p->gold < POTION_COST) {
        report("Gold kurang! Potion butuh %d gold.", POTION_COST);
        return;
    }
    p->gold -= POTION_COST;
    p->potion += 1;
    report("Potion dibeli. Stok potion: %d.", p->potion);
}

static void drawSell(void *ctx, int sel)
{
    const Player *p = ctx;
    char rows[MAX_ITEM][72];
    const char *ptrs[MAX_ITEM];
    char info[64];
    int i;

    for (i = 0; i < p->itemCount && i < MAX_ITEM; ++i) {
        const Item *it = &p->inventory[i];
        snprintf(rows[i], sizeof rows[i], "%2d. %-16s %-9s +%-3d %4dg%s",
                 i + 1, it->name, rarityLabel(it->rarity), it->power, sellPrice(it->rarity),
                 (i == p->weaponIndex || i == p->armorIndex) ? " (dipakai)" : "");
        ptrs[i] = rows[i];
    }
    snprintf(info, sizeof info, "Gold: %d   Pilih item yang dijual", p->gold);
    ui_screen_begin("SHOP - JUAL ITEM", info);
    ui_list_panel(UI_CONTENT_ROW, 2, 64, 16, "Inventory", ptrs, p->itemCount, sel);
    ui_screen_end(NULL);
}

/* The boxed confirmation only works on a terminal; piped runs read y/n. */
static int confirmSale(const char *question)
{
    char prompt[160];

    if (ui_interactive()) return ui_confirm_box(question);
    snprintf(prompt, sizeof prompt, "%s (y/n): ", question);
    return confirmPrompt(prompt);
}

static void sellItem(Player *p)
{
    char question[128];
    const Item *it;
    int idx, price, i;

    if (p->itemCount <= 0) {
        report("Inventory kosong, tidak ada item untuk dijual.");
        return;
    }
    idx = ui_pick_row(p->itemCount, drawSell, p);
    if (idx < 0 || idx >= p->itemCount) return;

    it = &p->inventory[idx];
    if (idx == p->weaponIndex || idx == p->armorIndex) {
        report("%s sedang dipakai, lepas dulu sebelum dijual.", it->name);
        return;
    }

    price = sellPrice(it->rarity);
    snprintf(question, sizeof question, "Jual %s seharga %d gold?", it->name, price);
    if (!confirmSale(question)) {
        report("Penjualan dibatalkan.");
        return;
    }

    {
        char name[MAX_NAME];
        snprintf(name, sizeof name, "%s", it->name);
        p->gold = (p->gold > INT_MAX - price) ? INT_MAX : p->gold + price;
        for (i = idx; i < p->itemCount - 1; ++i) p->inventory[i] = p->inventory[i + 1];
        p->itemCount -= 1;
        /* items behind the removed slot moved one place forward */
        if (p->weaponIndex > idx) p->weaponIndex -= 1;
        if (p->armorIndex > idx) p->armorIndex -= 1;
        report("Terjual: %s +%d gold", name, price);
    }
}

void shopMenu(Player *p)
{
    if (p == NULL) return;
    for (;;) {
        int pick = ui_menu_pick(shopItems, shopNums, 3, 0, 2, drawShop, p);
        if (pick == 1) buyPotion(p);
        else if (pick == 2) sellItem(p);
        else return;
    }
}
