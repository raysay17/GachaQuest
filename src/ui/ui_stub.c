/* Scaffold: modul UI belum diimplementasikan.
   Semua fungsi di sini stub supaya proyek tetap bisa dibangun.
   Implementasi nyata milik P1 (feat/scaffold-ui). */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "caps.h"
#include "render.h"
#include "style.h"
#include "term.h"
#include "ui.h"
#include "widgets.h"

/* --- caps ---------------------------------------------------------------- */

void caps_detect(void) {}
ColorMode caps_color(void) { return COLOR_MONO; }
int caps_unicode(void) { return 0; }
int caps_interactive(void) { return 0; }
int caps_cols(void) { return UI_COLS; }
int caps_rows(void) { return UI_ROWS; }
void caps_set_size(int cols, int rows) { (void)cols; (void)rows; }

const Glyphs *caps_glyphs(void)
{
    static const Glyphs ascii = {
        "+", "+", "+", "+", "-", "|",
        "#", ".",
        ">",
        "v"
    };
    return &ascii;
}

/* --- render -------------------------------------------------------------- */

void render_init(void) {}
void render_clear(void) {}

void render_text(int row, int col, Style s, const char *fmt, ...)
{
    (void)row; (void)col; (void)s; (void)fmt;
}

void render_text_clipped(int row, int col, int maxW, Style s, const char *fmt, ...)
{
    (void)row; (void)col; (void)maxW; (void)s; (void)fmt;
}

int render_span(int row, int col, int maxW, Style base, Style alt, const char *text)
{
    (void)row; (void)col; (void)maxW; (void)base; (void)alt; (void)text;
    return 0;
}

void render_char(int row, int col, Style s, unsigned int cp)
{
    (void)row; (void)col; (void)s; (void)cp;
}

void render_flush(void) {}

int ui_visible_len(const char *s) { return s != NULL ? (int)strlen(s) : 0; }
int render_max_row(void) { return 0; }
unsigned int render_cell_cp(int row, int col) { (void)row; (void)col; return 0; }

/* --- style --------------------------------------------------------------- */

void style_init(void) {}
Style style_rarity(int rarity) { (void)rarity; return ST_TEXT; }
Style style_hp(int hp, int maxHp) { (void)hp; (void)maxHp; return ST_TEXT; }
const char *style_fg(Style s) { (void)s; return ""; }
const char *style_bg(Style s) { (void)s; return ""; }

/* --- term ---------------------------------------------------------------- */

void term_init(void) {}
void term_shutdown(void) {}
TermKey term_getkey(char *ch) { (void)ch; return KEY_NONE; }
void term_raw(void) {}
void term_cooked(void) {}
void term_cursor(int on) { (void)on; }
void term_alt(int on) { (void)on; }
int term_size_query(void) { return 0; }
void term_poll_size(void) {}

/* --- ui ------------------------------------------------------------------ */

void ui_init(void) {}
void ui_shutdown(void) {}
int ui_interactive(void) { return 0; }

void set_message(const char *fmt, ...) { (void)fmt; }
const char *last_message(void) { return ""; }

void ui_log(const char *fmt, ...) { (void)fmt; }
void ui_log_style(Style st, const char *fmt, ...) { (void)st; (void)fmt; }
void ui_log_clear(void) {}

void ui_log_event(const char *actor, const char *action, const char *target,
                  const char *value, Style actorSt, Style targetSt, Style valueSt)
{
    (void)actor; (void)action; (void)target; (void)value;
    (void)actorSt; (void)targetSt; (void)valueSt;
}

int ui_log_count(void) { return 0; }
const char *ui_log_text(int i) { (void)i; return ""; }
Style ui_log_style_at(int i) { (void)i; return ST_TEXT; }
int ui_log_part_count(int i) { (void)i; return 0; }
const char *ui_log_part_text(int i, int part) { (void)i; (void)part; return ""; }
Style ui_log_part_style(int i, int part) { (void)i; (void)part; return ST_TEXT; }
void ui_log_draw(int row, int col, int maxW, int i)
{
    (void)row; (void)col; (void)maxW; (void)i;
}

void ui_frame(const char *screen) { (void)screen; }
void ui_flush(void) {}

void ui_panel(int row, int col, int w, int h, const char *title)
{
    (void)row; (void)col; (void)w; (void)h; (void)title;
}

void ui_hp_bar(int row, int col, int w, int hp, int maxHp)
{
    (void)row; (void)col; (void)w; (void)hp; (void)maxHp;
}

void ui_hp_bar_n(int row, int col, int w, int hp, int maxHp)
{
    (void)row; (void)col; (void)w; (void)hp; (void)maxHp;
}

void ui_bar(int row, int col, int w, int cur, int max, Style fill)
{
    (void)row; (void)col; (void)w; (void)cur; (void)max; (void)fill;
}

void ui_menu_v(int row, int col, const char *const *items, const int *nums,
               int n, int sel)
{
    (void)row; (void)col; (void)items; (void)nums; (void)n; (void)sel;
}

void ui_menu_h(int row, int col, const char *const *items, const int *nums,
               int n, int sel)
{
    (void)row; (void)col; (void)items; (void)nums; (void)n; (void)sel;
}

void ui_footer(const char *extra) { (void)extra; }
void ui_footer_line(const char *text) { (void)text; }
void ui_message(int row) { (void)row; }
void ui_status_panel_at(const Player *p, int row, int col, int w)
{
    (void)p; (void)row; (void)col; (void)w;
}

int ui_menu_pick(const char *const *items, const int *nums, int n,
                 int minNum, int maxNum, UiDrawFn draw, void *ctx)
{
    (void)items; (void)nums; (void)n; (void)maxNum; (void)draw; (void)ctx;
    return minNum;
}

int ui_confirm_box(const char *question) { (void)question; return 0; }

void ui_input_screen(const char *name) { (void)name; }
void ui_line_begin(const char *fmt, ...) { (void)fmt; }
void ui_line_end(void) {}
void ui_line_prompt(void) {}
void ui_line_error(const char *fmt, ...) { (void)fmt; }
void ui_line_note(const char *fmt, ...) { (void)fmt; }

int ui_line_read(char *buf, int max)
{
    (void)buf; (void)max;
    return -1;
}

int lebar_tampil(const char *s) { return s != NULL ? (int)strlen(s) : 0; }

int wrap_teks(const char *text, int width, char rows[][192], int maxRows)
{
    (void)text; (void)width; (void)rows; (void)maxRows;
    return 0;
}

void render_input_box(const char *question, const char *buffer)
{
    (void)question; (void)buffer;
}

int ui_pick_row(int count, UiDrawFn draw, void *ctx)
{
    (void)count; (void)draw; (void)ctx;
    return -1;
}

int ui_utf8_enabled(void) { return 0; }
void clear_screen(void) {}
void draw_box_top(const char *title) { (void)title; }
void draw_box_bottom(void) {}
void draw_box_line(const char *fmt, ...) { (void)fmt; }

void format_hp_bar(char *buf, int bufSize, int hp, int maxHp)
{
    if (buf != NULL && bufSize > 0) {
        snprintf(buf, (size_t)bufSize, "%d/%d", hp, maxHp);
    }
}

void draw_header(const char *title, const Player *p)
{
    (void)title; (void)p;
}

void draw_hp_bar(int hp, int maxHp) { (void)hp; (void)maxHp; }
void draw_menu(int number, const char *label) { (void)number; (void)label; }
const char *rarity_color(int rarity) { (void)rarity; return ""; }
const char *hp_color(int hp, int maxHp) { (void)hp; (void)maxHp; return ""; }

/* --- widgets ------------------------------------------------------------- */

void widget_panel(int row, int col, int w, int h, const char *title)
{
    (void)row; (void)col; (void)w; (void)h; (void)title;
}

void widget_hp_bar(int row, int col, int w, int hp, int maxHp)
{
    (void)row; (void)col; (void)w; (void)hp; (void)maxHp;
}

void widget_hp_bar_n(int row, int col, int w, int hp, int maxHp)
{
    (void)row; (void)col; (void)w; (void)hp; (void)maxHp;
}

void widget_bar(int row, int col, int w, int cur, int max, Style fill)
{
    (void)row; (void)col; (void)w; (void)cur; (void)max; (void)fill;
}

void widget_menu_v(int row, int col, const char *const *items, const int *nums,
                   int n, int sel)
{
    (void)row; (void)col; (void)items; (void)nums; (void)n; (void)sel;
}

void widget_menu_h(int row, int col, const char *const *items, const int *nums,
                   int n, int sel)
{
    (void)row; (void)col; (void)items; (void)nums; (void)n; (void)sel;
}

void widget_footer(const char *extra) { (void)extra; }
void widget_footer_line(const char *text) { (void)text; }
void widget_footer_reset(void) {}
void widget_msg(int row, int col, int w, const char *msg)
{
    (void)row; (void)col; (void)w; (void)msg;
}
