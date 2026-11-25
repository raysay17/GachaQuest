#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storage.h"
#include "atomicfile.h"
#include "ui.h"

/* True when the rest of the line (from pos) holds only whitespace. */
static int restIsBlank(const char *line, int pos)
{
    while (line[pos] != '\0') {
        if (line[pos] != ' ' && line[pos] != '\t' && line[pos] != '\r' && line[pos] != '\n') return 0;
        ++pos;
    }
    return 1;
}

/* Format per line: id|nama|tipe|rarity|power. Exactly MAX_ITEM valid lines
   are required; any failure returns 0 and the caller reports the error. */
int loadCatalog(const char *file, Item items[])
{
    FILE *f;
    char line[256];
    int count = 0;
    int ok = 1;

    if (file == NULL || items == NULL) return 0;
    f = fopen(file, "r");
    if (f == NULL) return 0;

    while (ok && fgets(line, (int)sizeof line, f) != NULL) {
        Item it;
        char type = 0;
        int used = 0;

        if (restIsBlank(line, 0)) continue;
        if (count >= MAX_ITEM) { ok = 0; break; }
        if (sscanf(line, "%d|%29[^|]|%c|%d|%d%n",
                   &it.id, it.name, &type, &it.rarity, &it.power, &used) != 5
            || !restIsBlank(line, used)
            || it.id < 1 || (type != 'S' && type != 'A')
            || it.rarity < RARITY_COMMON || it.rarity > RARITY_LEGENDARY
            || it.power < 0) {
            ok = 0;
            break;
        }
        it.type = type;
        items[count++] = it;
    }
    if (ferror(f)) ok = 0;
    fclose(f);
    return (ok && count == MAX_ITEM) ? count : 0;
}

/* Format per line: id|nama|hp|atk|def|exp|gold|minLevel. Exactly
   MAX_MONSTER valid lines are required; any failure returns 0. */
int loadMonsters(const char *file, Monster monsters[])
{
    FILE *f;
    char line[256];
    int count = 0;
    int ok = 1;

    if (file == NULL || monsters == NULL) return 0;
    f = fopen(file, "r");
    if (f == NULL) return 0;

    while (ok && fgets(line, (int)sizeof line, f) != NULL) {
        Monster m;
        int used = 0;

        if (restIsBlank(line, 0)) continue;
        if (count >= MAX_MONSTER) { ok = 0; break; }
        if (sscanf(line, "%d|%29[^|]|%d|%d|%d|%d|%d|%d%n",
                   &m.id, m.name, &m.hp, &m.atk, &m.def,
                   &m.exp, &m.gold, &m.minLevel, &used) != 8
            || !restIsBlank(line, used)
            || m.id < 1 || m.hp <= 0 || m.atk < 0 || m.def < 0
            || m.exp < 0 || m.gold < 0 || m.minLevel < 1) {
            ok = 0;
            break;
        }
        monsters[count++] = m;
    }
    if (ferror(f)) ok = 0;
    fclose(f);
    return (ok && count == MAX_MONSTER) ? count : 0;
}

/* ------------------------------------------------------------------ */
/* Small shared helpers                                               */
/* ------------------------------------------------------------------ */

#define LINE_LEN 512
#define SAVE_BUF 2048
#define SAVE_FIELDS 15
#define LEGACY_SAVE "data/save.txt"
#define LB_FILE "data/leaderboard.txt"
#define LB_READ_CAP 64

static int slotValid(int slot)
{
    return slot >= 1 && slot <= SAVE_SLOTS;
}

/* Appends formatted text to buf; 0 when it would not fit. */
static int appendf(char *buf, int cap, int *len, const char *fmt, ...)
{
    va_list ap;
    int n;

    if (*len >= cap) return 0;
    va_start(ap, fmt);
    n = vsnprintf(buf + *len, (size_t)(cap - *len), fmt, ap);
    va_end(ap);
    if (n < 0 || n >= cap - *len) return 0;
    *len += n;
    return 1;
}

/* '|' and line breaks would break the file format, so they never reach disk. */
static void cleanName(const char *src, char *dst)
{
    int i;

    for (i = 0; src[i] != '\0' && i < MAX_NAME - 1; ++i) {
        char c = src[i];
        dst[i] = (c == '|' || c == '\n' || c == '\r') ? '_' : c;
    }
    dst[i] = '\0';
}

/* Reads one line without its line ending; 0 on EOF or a line that is too long. */
static int readFileLine(FILE *f, char *buf, int size)
{
    size_t n;

    if (fgets(buf, size, f) == NULL) return 0;
    n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[--n] = '\0';
    } else if (!feof(f)) {
        return 0;
    }
    if (n > 0 && buf[n - 1] == '\r') buf[n - 1] = '\0';
    return 1;
}

static int parseInt(const char *s, int *out)
{
    char *end;
    long v;

    if (s == NULL || (*s != '-' && (*s < '0' || *s > '9'))) return 0;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v < INT_MIN || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int parseUInt(const char *s, unsigned int *out)
{
    char *end;
    unsigned long v;

    if (s == NULL || *s < '0' || *s > '9') return 0;
    errno = 0;
    v = strtoul(s, &end, 10);
    if (errno != 0 || *end != '\0' || v > UINT_MAX) return 0;
    *out = (unsigned int)v;
    return 1;
}

/* Splits line in place at '|'. Returns the field count, or max + 1 when the
   line has more than max fields. */
static int splitFields(char *line, char *fields[], int max)
{
    int n = 0;
    char *s = line;

    for (;;) {
        if (n >= max) return max + 1;
        fields[n++] = s;
        s = strchr(s, '|');
        if (s == NULL) break;
        *s++ = '\0';
    }
    return n;
}

/* ------------------------------------------------------------------ */
/* Save slots                                                          */
/* ------------------------------------------------------------------ */

void saveSlotPath(int slot, char *out, int size)
{
    if (out == NULL || size <= 0) return;
    snprintf(out, (size_t)size, "data/save_%d.txt", slot);
}

/* Line 1: nama|hp|maxHp|level|exp|gold|atk|def|potion|pity|totalPull|score|seed|weaponId|armorId
   Line 2: itemCount, then one item id per line. weaponId/armorId are item ids
   (not array indexes); -1 means nothing is equipped. */
int saveGameSlot(const Player *p, int slot)
{
    char path[64];
    char name[MAX_NAME];
    char buf[SAVE_BUF];
    int len = 0;
    int weaponId = -1;
    int armorId = -1;
    int i;

    if (p == NULL || !slotValid(slot)) return 0;
    if (p->itemCount < 0 || p->itemCount > MAX_ITEM) return 0;
    if (p->weaponIndex >= 0 && p->weaponIndex < p->itemCount) weaponId = p->inventory[p->weaponIndex].id;
    if (p->armorIndex >= 0 && p->armorIndex < p->itemCount) armorId = p->inventory[p->armorIndex].id;

    cleanName(p->name, name);
    if (!appendf(buf, SAVE_BUF, &len, "%s|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%u|%d|%d\n",
                 name, p->hp, p->maxHp, p->level, p->exp, p->gold, p->atk, p->def,
                 p->potion, p->pity, p->totalPull, p->score, p->seed, weaponId, armorId))
        return 0;
    if (!appendf(buf, SAVE_BUF, &len, "%d\n", p->itemCount)) return 0;
    for (i = 0; i < p->itemCount; ++i) {
        if (!appendf(buf, SAVE_BUF, &len, "%d\n", p->inventory[i].id)) return 0;
    }

    saveSlotPath(slot, path, (int)sizeof path);
    return file_write_atomic(path, buf, (size_t)len);
}

int saveGame(const Player *p)
{
    return saveGameSlot(p, 1);
}

/* Parses line 1 into t and returns the equipped item ids. 0 on any damage. */
static int parseSaveHeader(char *line, Player *t, int *weaponId, int *armorId)
{
    char *f[SAVE_FIELDS];
    int v[11];
    unsigned int seed;
    int i;

    if (splitFields(line, f, SAVE_FIELDS) != SAVE_FIELDS) return 0;
    if (f[0][0] == '\0' || strlen(f[0]) >= MAX_NAME) return 0;
    for (i = 0; i < 11; ++i) {
        if (!parseInt(f[1 + i], &v[i])) return 0;
    }
    if (!parseUInt(f[12], &seed)) return 0;
    if (!parseInt(f[13], weaponId) || !parseInt(f[14], armorId)) return 0;

    /* hp maxHp level exp gold atk def potion pity totalPull score */
    if (v[0] < 0 || v[1] < 0 || v[2] < 1 || v[3] < 0 || v[4] < 0 || v[5] < 0 ||
        v[6] < 0 || v[7] < 0 || v[7] > MAX_POTION || v[8] < 0 || v[9] < 0 || v[10] < 0)
        return 0;

    strcpy(t->name, f[0]);
    t->hp = v[0];
    t->maxHp = v[1];
    t->level = v[2];
    t->exp = v[3];
    t->gold = v[4];
    t->atk = v[5];
    t->def = v[6];
    t->potion = v[7];
    t->pity = v[8];
    t->totalPull = v[9];
    t->score = v[10];
    t->seed = seed;
    return 1;
}

static int parseSaveFile(FILE *f, Player *t, const Item items[], int count)
{
    char line[LINE_LEN];
    int weaponId, armorId, n, i, j;

    if (!readFileLine(f, line, (int)sizeof line)) return 0;
    if (!parseSaveHeader(line, t, &weaponId, &armorId)) return 0;

    if (!readFileLine(f, line, (int)sizeof line)) return 0;
    if (!parseInt(line, &n) || n < 0 || n > MAX_ITEM) return 0;

    for (i = 0; i < n; ++i) {
        int id;
        if (!readFileLine(f, line, (int)sizeof line)) return 0;
        if (!parseInt(line, &id)) return 0;
        for (j = 0; j < count && items[j].id != id; ++j) { }
        if (j >= count) return 0;
        t->inventory[i] = items[j];
    }
    t->itemCount = n;

    t->weaponIndex = -1;
    t->armorIndex = -1;
    for (i = 0; i < n; ++i) {
        if (t->weaponIndex < 0 && t->inventory[i].id == weaponId) t->weaponIndex = i;
        if (t->armorIndex < 0 && t->inventory[i].id == armorId) t->armorIndex = i;
    }
    return 1;
}

/* Slot 1 falls back to the legacy data/save.txt while save_1.txt is missing.
   *p is only touched when the whole file loaded, so a damaged save never
   leaves a half-written player behind. */
int loadGame(Player *p, const Item items[], int count)
{
    return loadGameSlot(p, items, count, 1);
}

int loadGameSlot(Player *p, const Item items[], int count, int slot)
{
    char path[64];
    FILE *f;
    Player t;
    int ok;

    if (p == NULL || items == NULL || count < 0 || !slotValid(slot)) return 0;

    saveSlotPath(slot, path, (int)sizeof path);
    f = fopen(path, "r");
    if (f == NULL && slot == 1) f = fopen(LEGACY_SAVE, "r");

    ok = 0;
    if (f != NULL) {
        memset(&t, 0, sizeof t);
        t.weaponIndex = -1;
        t.armorIndex = -1;
        ok = parseSaveFile(f, &t, items, count);
        fclose(f);
    }
    if (!ok) {
        set_message("Gagal memuat slot %d", slot);
        return 0;
    }
    *p = t;
    return 1;
}

int saveSlotExists(int slot)
{
    char path[64];
    FILE *f;

    if (!slotValid(slot)) return 0;
    saveSlotPath(slot, path, (int)sizeof path);
    f = fopen(path, "r");
    if (f == NULL) return 0;
    fclose(f);
    return 1;
}

/* Fills out with the player name stored in the slot (first field of line 1).
   Returns 1 when a name was read; otherwise out holds a placeholder label. */
int saveSlotName(int slot, char *out, int size)
{
    char path[64];
    char line[LINE_LEN];
    FILE *f;
    char *bar;
    int opened = 0;
    int ok = 0;

    if (out == NULL || size <= 0) return 0;
    out[0] = '\0';
    if (slotValid(slot)) {
        saveSlotPath(slot, path, (int)sizeof path);
        f = fopen(path, "r");
        if (f != NULL) {
            opened = 1;
            if (readFileLine(f, line, (int)sizeof line)) {
                bar = strchr(line, '|');
                if (bar != NULL) *bar = '\0';
                if (line[0] != '\0') {
                    snprintf(out, (size_t)size, "%s", line);
                    ok = 1;
                }
            }
            fclose(f);
        }
    }
    if (!ok) snprintf(out, (size_t)size, "Slot %d - %s", slot, opened ? "rusak" : "kosong");
    return ok;
}

/* First slot without a file, or 0 when all SAVE_SLOTS are taken (0 is not a
   valid slot, so saveGameSlot refuses it instead of overwriting). */
int saveSlotFirstFree(void)
{
    int i;

    for (i = 1; i <= SAVE_SLOTS; ++i) {
        if (!saveSlotExists(i)) return i;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* Leaderboard                                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[MAX_NAME];
    int level;
    int score;
} LbEntry;

/* Reads nama|level|skor lines; malformed lines are skipped. */
static int lbRead(LbEntry e[], int cap)
{
    FILE *f;
    char line[LINE_LEN];
    int n = 0;

    f = fopen(LB_FILE, "r");
    if (f == NULL) return 0;
    while (n < cap && readFileLine(f, line, (int)sizeof line)) {
        char *fld[3];
        int level, score;

        if (splitFields(line, fld, 3) != 3) continue;
        if (fld[0][0] == '\0' || strlen(fld[0]) >= MAX_NAME) continue;
        if (!parseInt(fld[1], &level) || !parseInt(fld[2], &score)) continue;
        strcpy(e[n].name, fld[0]);
        e[n].level = level;
        e[n].score = score;
        ++n;
    }
    fclose(f);
    return n;
}

/* Bubble sort, highest score first; equal scores keep their file order. */
static void lbSort(LbEntry e[], int n)
{
    int i, j;

    for (i = 0; i < n - 1; ++i) {
        for (j = 0; j < n - 1 - i; ++j) {
            if (e[j].score < e[j + 1].score) {
                LbEntry tmp = e[j];
                e[j] = e[j + 1];
                e[j + 1] = tmp;
            }
        }
    }
}

void recordScore(const Player *p)
{
    LbEntry e[LB_READ_CAP + 1];
    char buf[1024];
    int n, len = 0, i;

    if (p == NULL || p->name[0] == '\0') return;

    n = lbRead(e, LB_READ_CAP);
    cleanName(p->name, e[n].name);
    e[n].level = p->level;
    e[n].score = p->score;
    ++n;
    lbSort(e, n);
    if (n > MAX_LEADERBOARD) n = MAX_LEADERBOARD;

    for (i = 0; i < n; ++i) {
        if (!appendf(buf, (int)sizeof buf, &len, "%s|%d|%d\n", e[i].name, e[i].level, e[i].score)) {
            set_message("Gagal menyimpan skor.");
            return;
        }
    }
    if (!file_write_atomic(LB_FILE, buf, (size_t)len)) {
        set_message("Gagal menyimpan skor.");
        return;
    }
    ui_log("Skor %s tercatat: %d", p->name, p->score);
}

typedef struct {
    LbEntry e[TOP_DISPLAY];
    char row[TOP_DISPLAY][64];
    const char *rowPtr[TOP_DISPLAY];
    int n;
} LbView;

static void drawLeaderboard(void *ctx, int sel)
{
    const LbView *v = ctx;

    (void)sel;
    ui_screen_begin("LEADERBOARD", "Skor tertinggi");
    if (v->n > 0)
        ui_list_panel(UI_CONTENT_ROW, 4, 50, v->n + 2, "Top Skor", v->rowPtr, v->n, -1);
    else
        ui_panel(UI_CONTENT_ROW, 4, 50, 3, "Top Skor (belum ada)");
    ui_screen_end(NULL);
}

void showLeaderboard(void)
{
    static const char *const backItem[] = { "Kembali" };
    static const int backNum[] = { 0 };
    LbEntry all[LB_READ_CAP];
    LbView view;
    int total, i;

    total = lbRead(all, LB_READ_CAP);
    lbSort(all, total);
    view.n = total < TOP_DISPLAY ? total : TOP_DISPLAY;
    for (i = 0; i < view.n; ++i) {
        view.e[i] = all[i];
        snprintf(view.row[i], sizeof view.row[i], "%d. %-18s Lv %-3d %6d",
                 i + 1, view.e[i].name, view.e[i].level, view.e[i].score);
        view.rowPtr[i] = view.row[i];
    }

    /* Piped input has nothing to wait for; interactive mode waits for a key. */
    if (ui_interactive()) {
        ui_menu_pick(backItem, backNum, 1, 0, 0, drawLeaderboard, &view);
    } else {
        drawLeaderboard(&view, -1);
    }
}
