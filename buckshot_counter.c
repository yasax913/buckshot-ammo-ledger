/* ============================================================================
 *  恶魔轮盘 · 弹药记录器   (Buckshot Roulette Ammo Ledger)   v2.0
 * ----------------------------------------------------------------------------
 *  纯 Win32 API + GDI，单文件 C，静态链接，无框架依赖。
 *  定位：极小体积（约 45 KB）、极低内存（私有内存约 1.5 MB）、空闲 CPU 0。
 *
 *  设计参考「恶魔轮盘弹药记录」网页版的记录模型：
 *    · 先设定本轮的实弹 / 空弹总数，然后**记录已发射的弹药**，
 *      剩余数 = 本轮总数 − 已发射数；
 *    · 实时显示下一发实弹概率；
 *    · 支持撤销上一发、开始新一轮、清零；
 *    · 记录本轮射击序列与累计统计。
 *
 *  操作
 *    左键「记录实弹 / 记录空弹」= 记一发（对应剩余数 −1）
 *    左键「修正 +」= 对应剩余数 +1（记错时补回）
 *    右键任意按钮 = 撤销上一发
 *    滚轮悬停在某一行 = 上滚记录该类型一发，下滚补回一发
 *    全局热键：Ctrl+Alt+S 实弹+1、Ctrl+Alt+D 空弹+1、Ctrl+Alt+Z 撤销、Ctrl+Alt+R 清零
 *    「轮」= 新一轮弹药设置面板（预设 2/2、3/2、3/3、4/2、4/3 + 步进器）
 *    键盘：1 实弹、2 空弹、U/Ctrl+Z 撤销、N 新一轮、Esc 关闭面板/退出
 *
 *  编译：build.bat（windres + gcc -O2 -s -mwindows）
 * ========================================================================== */

#include <windows.h>

/* ------------------------- 界面文本（Unicode 转义，避免源码编码问题）---- */
#define APP_TITLE   L"\u6076\u9B54\u8F6E\u76D8 \u00B7 \u5F39\u836F\u8BB0\u5F55"      /* 恶魔轮盘 · 弹药记录 */
#define TXT_ROUND   L"\u7B2C"                       /* 第 */
#define TXT_ROUND2  L"\u8F6E"                        /* 轮 */
#define TXT_LEFT    L"\u5269\u4F59"                  /* 剩余 */
#define TXT_SHELLS  L"\u53D1"                        /* 发 */
#define TXT_CHANCE  L"\u4E0B\u4E00\u53D1\u5B9E\u5F39"         /* 下一发实弹 */
#define TXT_LIVE    L"\u5B9E\u5F39"                  /* 实弹 */
#define TXT_BLANK   L"\u7A7A\u5F39"                  /* 空弹 */
#define TXT_REC     L"\u8BB0\u5F55"                  /* 记录 */
#define TXT_ADJ     L"\u4FEE\u6B63"                  /* 修正 */
#define TXT_NEW     L"\u65B0\u4E00\u8F6E"            /* 新一轮 */
#define TXT_UNDO    L"\u64A4\u9500"                  /* 撤销 */
#define TXT_CLEAR   L"\u6E05\u96F6"                  /* 清零 */
#define TXT_TOP     L"\u7F6E"                        /* 置 */
#define TXT_DONE    L"\u672C\u8F6E\u5DF2\u5B8C\u6210"        /* 本轮已完成 */
#define TXT_HINT    L"1\u5B9E\u5F39  2\u7A7A\u5F39  U\u64A4\u9500  N\u65B0\u4E00\u8F6E"  /* 1实弹 2空弹 U撤销 N新一轮 */
#define TXT_SETUP   L"\u65B0\u4E00\u8F6E\u5F39\u836F"        /* 新一轮弹药 */
#define TXT_START   L"\u5F00\u59CB\u8BB0\u5F55"              /* 开始记录 */
#define TXT_CANCEL  L"\u53D6\u6D88"                  /* 取消 */
#define TXT_TOTAL   L"\u672C\u8F6E\u603B\u91CF"              /* 本轮总量 */
#define TXT_ERR     L"\u81F3\u5C11\u9700\u8981\u4E00\u53D1\u5F39\u836F"   /* 至少需要一发弹药 */
#define TXT_ACTIVE  L"\u8BB0\u5F55\u4E2D"            /* 记录中 */
#define FONT_UI     L"Microsoft YaHei UI"
#define FONT_NUM    L"Consolas"
#define WND_CLASS   L"BuckshotAmmoLedger"

/* ------------------------- 尺寸与配色 ------ */
#define WND_W        320
#define WND_H        246
#define TITLE_H      28
#define MAX_SHOT     40          /* 保存的射击序列长度 */
#define MAX_ROUND    16          /* 每种弹药每轮上限 */
#define DEFAULT_LIVE 3
#define DEFAULT_BLANK 3

#define COL_BG      RGB(14, 17, 18)
#define COL_CARD    RGB(23, 27, 29)
#define COL_CARD2   RGB(29, 34, 36)
#define COL_LINE    RGB(48, 54, 56)
#define COL_LINE2   RGB(68, 75, 77)
#define COL_FG      RGB(241, 239, 232)
#define COL_MUTED   RGB(155, 162, 163)
#define COL_MUTED2  RGB(193, 197, 195)
#define COL_RED     RGB(216, 74, 65)
#define COL_RED_HI  RGB(244, 105, 96)
#define COL_RED_BG  RGB(59, 27, 25)
#define COL_BLANK   RGB(196, 200, 191)
#define COL_BLANK_D RGB(101, 107, 105)
#define COL_BLANK_BG RGB(36, 42, 43)
#define COL_BRASS   RGB(178, 139, 79)
#define COL_BRASS_D RGB(120, 92, 50)
#define COL_OK      RGB(135, 184, 154)
#define COL_BTN     RGB(38, 43, 45)
#define COL_BTN_HOT RGB(58, 65, 67)

/* ------------------------- 状态 ------ */
typedef struct { unsigned char type; } Shot;

static int   g_round      = 1;
static int   g_live_total = DEFAULT_LIVE;
static int   g_blank_total = DEFAULT_BLANK;
static Shot  g_shots[MAX_SHOT];
static int   g_shot_n     = 0;      /* 本轮已记录发数 */
static int   g_fired_live = 0;      /* 本轮已发射实弹（含被挤出历史的）*/
static int   g_fired_blank = 0;
static int   g_all_live   = 0;      /* 累计 */
static int   g_all_blank  = 0;
static int   g_done_rounds = 0;

static BOOL  g_topmost    = TRUE;
static BOOL  g_hotkeys    = TRUE;
static int   g_pos_x      = -100000, g_pos_y = -100000;

static int   g_hot = 0, g_down = 0;          /* 悬停 / 按下的控件 id */
static BOOL  g_setup_open = FALSE;           /* 新一轮面板是否展开 */
static int   g_set_live = 3, g_set_blank = 3;/* 面板中的待设定值 */
static BOOL  g_set_err = FALSE;
static int   g_flash = 0;                    /* 动画计数（帧递减）*/
static int   g_flash_id = 0;

static HFONT g_f_tiny = NULL, g_f_small = NULL, g_f_body = NULL,
             g_f_bold = NULL, g_f_num = NULL, g_f_bignum = NULL;

/* 控件 id */
enum {
    HIT_NONE = 0,
    HIT_TOP, HIT_UNDO, HIT_NEW, HIT_CLOSE,
    HIT_REC_LIVE, HIT_ADD_LIVE,
    HIT_REC_BLANK, HIT_ADD_BLANK,
    /* 新一轮面板 */
    HIT_P1, HIT_P2, HIT_P3, HIT_P4, HIT_P5,
    HIT_ML, HIT_PL, HIT_MB, HIT_PB,
    HIT_START, HIT_CANCEL
};

enum { HOT_LIVE = 1, HOT_BLANK, HOT_UNDO, HOT_CLEAR };

#define TIMER_ANIM 1

/* ============================ 小工具函数 ============================ */
static void mem_zero(void *d, unsigned n)
{
    unsigned char *p = (unsigned char *)d;
    while (n--) *p++ = 0;
}
static int str_starts(const char *s, const char *prefix)
{
    while (*prefix) { if (*s++ != *prefix++) return 0; }
    return 1;
}
static char *str_chr(const char *s, int c)
{
    for (; *s; s++) if (*s == (char)c) return (char *)s;
    return NULL;
}
static unsigned str_len(const char *s) { const char *p = s; while (*p) p++; return (unsigned)(p - s); }
static void str_copy(char *d, const char *s) { while ((*d++ = *s++) != 0) {} }
static int to_int(const char *s)
{
    int v = 0, neg = 0;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return neg ? -v : v;
}
static char *put_num(char *p, int v, int min_digits)
{
    char tmp[12];
    int n = 0, i;
    if (v < 0) v = 0;
    if (v > 999999) v = 999999;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v > 0);
    while (n < min_digits) tmp[n++] = '0';
    for (i = n - 1; i >= 0; i--) *p++ = tmp[i];
    return p;
}
static int clamp_i(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* 十进制字符串（WCHAR，定长缓冲） */
static void wstr_num(WCHAR *out, int v, int width)
{
    WCHAR tmp[12];
    int n = 0, i;
    if (v < 0) v = 0;
    if (v > 99999) v = 99999;
    do { tmp[n++] = (WCHAR)(L'0' + v % 10); v /= 10; } while (v > 0);
    while (n < width) tmp[n++] = L'0';
    for (i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    out[n] = 0;
}
static void wstr_cat(WCHAR *dst, const WCHAR *src)
{
    WCHAR *p = dst;
    while (*p) p++;
    while ((*p++ = *src++) != 0) {}
}
static void wstr_set(WCHAR *dst, const WCHAR *src)
{
    while ((*dst++ = *src++) != 0) {}
}

/* ============================ GDI 绘图工具 ============================ */
static void fill_rect(HDC dc, int l, int t, int r, int b, COLORREF c)
{
    RECT rc; HBRUSH br;
    rc.left = l; rc.top = t; rc.right = r; rc.bottom = b;
    br = CreateSolidBrush(c);
    FillRect(dc, &rc, br);
    DeleteObject(br);
}
static void draw_line(HDC dc, int x1, int y1, int x2, int y2, COLORREF c)
{
    HPEN pen = CreatePen(PS_SOLID, 1, c);
    HGDIOBJ old = SelectObject(dc, pen);
    MoveToEx(dc, x1, y1, NULL);
    LineTo(dc, x2, y2);
    SelectObject(dc, old);
    DeleteObject(pen);
}
static void fill_round(HDC dc, int l, int t, int r, int b, int rad, COLORREF fill, COLORREF border)
{
    HBRUSH br = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ ob = SelectObject(dc, br);
    HGDIOBJ op = SelectObject(dc, pen);
    RoundRect(dc, l, t, r, b, rad * 2, rad * 2);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(br);
    DeleteObject(pen);
}
static void text_left(HDC dc, HFONT f, const WCHAR *s, int x, int cy, COLORREF c)
{
    RECT r;
    HGDIOBJ old = SelectObject(dc, f);
    SetTextColor(dc, c);
    r.left = x; r.right = x + 260; r.top = cy - 12; r.bottom = cy + 12;
    DrawTextW(dc, s, lstrlenW(s), &r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}
static void text_right(HDC dc, HFONT f, const WCHAR *s, int right, int cy, COLORREF c)
{
    RECT r;
    HGDIOBJ old = SelectObject(dc, f);
    SetTextColor(dc, c);
    r.left = right - 200; r.right = right; r.top = cy - 12; r.bottom = cy + 12;
    DrawTextW(dc, s, lstrlenW(s), &r, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}
static void text_center(HDC dc, HFONT f, const WCHAR *s, int cx, int cy, COLORREF c)
{
    RECT r;
    HGDIOBJ old = SelectObject(dc, f);
    SetTextColor(dc, c);
    r.left = cx - 120; r.right = cx + 120; r.top = cy - 14; r.bottom = cy + 14;
    DrawTextW(dc, s, lstrlenW(s), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}
static void text_in(HDC dc, HFONT f, const WCHAR *s, const RECT *box, COLORREF c)
{
    HGDIOBJ old = SelectObject(dc, f);
    SetTextColor(dc, c);
    DrawTextW(dc, s, lstrlenW(s), (RECT *)box, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, old);
}

/* 画一发子弹：实弹 = 红弹头 + 黄铜弹壳；空弹 = 灰白弹壳 + 灰弹头 */
static void draw_shell(HDC dc, int x, int y, BOOL live, int scale, BOOL dim)
{
    int w    = (scale == 0) ? 9 : 11;
    int h    = (scale == 0) ? 20 : 26;
    int capH = h * 34 / 100;          /* 弹头（顶部收窄部分）高度 */
    int rimY = y + h - 4;
    int i;
    COLORREF tipLo, casTop, casBot, edge, hi, rim;

    if (live) {
        tipLo = COL_RED;  casTop = RGB(212, 172, 104); casBot = RGB(140, 104, 54);
        edge = RGB(72, 46, 30); hi = RGB(238, 208, 150); rim = RGB(96, 70, 34);
    } else {
        tipLo = RGB(112, 118, 115); casTop = RGB(214, 218, 208); casBot = RGB(150, 156, 150);
        edge = RGB(74, 80, 78);  hi = RGB(246, 248, 242); rim = RGB(104, 110, 107);
    }
    if (dim) {
        tipLo = live ? RGB(92, 46, 42) : RGB(80, 84, 82);
        casTop = live ? RGB(104, 86, 56) : RGB(96, 100, 98);
        casBot = live ? RGB(74, 62, 42)  : RGB(74, 78, 76);
        edge = RGB(44, 48, 47); hi = casTop; rim = casBot;
    }

    /* 弹壳主体（竖直渐变） */
    for (i = capH; i < h - 4; i++) {
        int t = (i - capH) * 255 / (h - 4 - capH > 0 ? (h - 4 - capH) : 1);
        COLORREF c = RGB((GetRValue(casTop) * (255 - t) + GetRValue(casBot) * t) / 255,
                         (GetGValue(casTop) * (255 - t) + GetGValue(casBot) * t) / 255,
                         (GetBValue(casTop) * (255 - t) + GetBValue(casBot) * t) / 255);
        fill_rect(dc, x, y + i, x + w, y + i + 1, c);
    }
    /* 弹头：顶部收窄 */
    for (i = 0; i < capH; i++) {
        int inset = (capH - 1 - i) * (w / 2 - 1) / (capH > 1 ? capH - 1 : 1);
        COLORREF c = (i < capH / 2) ? (live ? COL_RED_HI : RGB(152, 158, 155)) : tipLo;
        int l = x + inset, r = x + w - inset;
        if (r > l) fill_rect(dc, l, y + i, r, y + i + 1, c);
    }
    /* 弹壳底缘 */
    fill_rect(dc, x, rimY, x + w, y + h, rim);
    /* 高光 */
    if (!dim) fill_rect(dc, x + 1, y + capH + 1, x + 2, rimY - 1, hi);
    /* 描边 */
    draw_line(dc, x, y + capH, x, y + h - 1, edge);
    draw_line(dc, x + w - 1, y + capH, x + w - 1, y + h - 1, edge);
    draw_line(dc, x, y + h - 1, x + w - 1, y + h - 1, edge);
    draw_line(dc, x, rimY, x + w - 1, rimY, edge);
    if (!dim) draw_line(dc, x, y + capH, x + w - 1, y + capH, edge);
}

/* 小圆点（射击序列用） */
static void draw_pip(HDC dc, int cx, int cy, BOOL live, BOOL newest)
{
    HBRUSH br = CreateSolidBrush(live ? (newest ? COL_RED_HI : COL_RED)
                                      : (newest ? COL_BLANK : COL_BLANK_D));
    HPEN pen = CreatePen(PS_SOLID, 1, live ? COL_RED : COL_BLANK_D);
    HGDIOBJ ob = SelectObject(dc, br);
    HGDIOBJ op = SelectObject(dc, pen);
    int r = newest ? 4 : 3;
    Ellipse(dc, cx - r, cy - r, cx + r + 1, cy + r + 1);
    SelectObject(dc, ob);
    SelectObject(dc, op);
    DeleteObject(br);
    DeleteObject(pen);
}

/* ============================ 计数逻辑 ============================ */
static int remaining_live(void)
{
    int v = g_live_total - g_fired_live;
    return v < 0 ? 0 : v;
}
static int remaining_blank(void)
{
    int v = g_blank_total - g_fired_blank;
    return v < 0 ? 0 : v;
}
static BOOL round_complete(void)
{
    return remaining_live() + remaining_blank() == 0;
}
static int next_live_percent(void)
{
    int tot = remaining_live() + remaining_blank();
    if (tot <= 0) return -1;
    return (remaining_live() * 100 + tot / 2) / tot;
}
static void push_shot(int type)
{
    int i;
    if (g_shot_n >= MAX_SHOT) {
        /* 历史满：丢弃最旧一条（已发射计数不受影响） */
        for (i = 1; i < g_shot_n; i++) g_shots[i - 1] = g_shots[i];
        g_shot_n--;
    }
    g_shots[g_shot_n++].type = (unsigned char)type;
}
static void pop_shot(int *out_type)
{
    if (g_shot_n <= 0) { *out_type = -1; return; }
    g_shot_n--;
    *out_type = g_shots[g_shot_n].type;
}

static void record_shot(HWND hwnd, int type)
{
    BOOL was_done = round_complete();

    if (type == 0) {
        if (remaining_live() <= 0) { MessageBeep(MB_ICONASTERISK); return; }
        g_fired_live++;
        g_all_live++;
    } else {
        if (remaining_blank() <= 0) { MessageBeep(MB_ICONASTERISK); return; }
        g_fired_blank++;
        g_all_blank++;
    }
    push_shot(type);

    if (!was_done && round_complete()) {
        g_done_rounds++;
        MessageBeep(MB_ICONASTERISK);
        /* 本轮打完，自动弹出新一轮设置面板 */
        g_setup_open = TRUE;
        g_set_live = g_live_total;
        g_set_blank = g_blank_total;
        g_set_err = FALSE;
    }
    g_flash = 5;
    g_flash_id = (type == 0) ? HIT_REC_LIVE : HIT_REC_BLANK;
    SetTimer(hwnd, TIMER_ANIM, 60, NULL);
}

static void adjust_back(HWND hwnd, int type)
{
    if (type == 0) {
        if (g_fired_live <= 0) return;
        g_fired_live--;
        if (g_all_live > 0) g_all_live--;
    } else {
        if (g_fired_blank <= 0) return;
        g_fired_blank--;
        if (g_all_blank > 0) g_all_blank--;
    }
    /* 从序列尾部移除最近一条同类型记录 */
    {
        int i;
        for (i = g_shot_n - 1; i >= 0; i--) {
            if (g_shots[i].type == type) {
                int j;
                for (j = i + 1; j < g_shot_n; j++) g_shots[j - 1] = g_shots[j];
                g_shot_n--;
                break;
            }
        }
    }
    (void)hwnd;
}

static void undo_shot(HWND hwnd)
{
    int type;
    BOOL was_done = round_complete();
    pop_shot(&type);
    if (type < 0) { MessageBeep(MB_ICONASTERISK); return; }

    if (type == 0) {
        if (g_fired_live > 0) g_fired_live--;
        if (g_all_live > 0) g_all_live--;
    } else {
        if (g_fired_blank > 0) g_fired_blank--;
        if (g_all_blank > 0) g_all_blank--;
    }
    if (was_done) {
        if (g_done_rounds > 0) g_done_rounds--;
        g_setup_open = FALSE;
    }
    g_flash = 5;
    g_flash_id = HIT_UNDO;
    SetTimer(hwnd, TIMER_ANIM, 60, NULL);
}

static void clear_all(HWND hwnd)
{
    g_fired_live = 0;
    g_fired_blank = 0;
    g_all_live = 0;
    g_all_blank = 0;
    g_shot_n = 0;
    g_done_rounds = 0;
    g_round = 1;
    g_setup_open = FALSE;
    g_flash = 5;
    g_flash_id = HIT_UNDO;
    SetTimer(hwnd, TIMER_ANIM, 60, NULL);
}

static void start_new_round(HWND hwnd, int live, int blank)
{
    BOOL has_progress = (g_shot_n > 0);
    if (live + blank <= 0) { g_set_err = TRUE; return; }
    if (has_progress) g_round++;
    g_live_total = clamp_i(live, 0, MAX_ROUND);
    g_blank_total = clamp_i(blank, 0, MAX_ROUND);
    g_fired_live = 0;
    g_fired_blank = 0;
    g_shot_n = 0;
    g_setup_open = FALSE;
    g_set_err = FALSE;
    g_flash = 5;
    g_flash_id = HIT_NEW;
    SetTimer(hwnd, TIMER_ANIM, 60, NULL);
}

/* ============================ 设置文件 ============================ */
static void settings_path(char *out)
{
    char path[MAX_PATH];
    char *slash = path, *p;
    DWORD n = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) { str_copy(out, "settings.ini"); return; }
    for (p = path; *p; p++) if (*p == '\\' || *p == '/') slash = p + 1;
    *slash = 0;
    str_copy(out, path);
    str_copy(out + str_len(out), "settings.ini");
}
static void load_settings(void)
{
    char path[MAX_PATH];
    char buf[512];
    DWORD got = 0;
    HANDLE h;
    char *line;
    int x = -100000, y = -100000, top = 1, hk = 1, li = DEFAULT_LIVE, bl = DEFAULT_BLANK;

    settings_path(path);
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    if (!ReadFile(h, buf, 511, &got, NULL)) got = 0;
    CloseHandle(h);
    buf[got] = 0;

    line = buf;
    while (*line) {
        char *eol = str_chr(line, '\n');
        if (eol) *eol = 0;
        if      (str_starts(line, "round="))      g_round       = to_int(line + 6);
        else if (str_starts(line, "live_total=")) g_live_total  = to_int(line + 11);
        else if (str_starts(line, "blank_total="))g_blank_total = to_int(line + 12);
        else if (str_starts(line, "fired_live=")) g_fired_live  = to_int(line + 11);
        else if (str_starts(line, "fired_blank="))g_fired_blank = to_int(line + 12);
        else if (str_starts(line, "all_live="))   g_all_live    = to_int(line + 9);
        else if (str_starts(line, "all_blank="))  g_all_blank   = to_int(line + 10);
        else if (str_starts(line, "done="))       g_done_rounds = to_int(line + 5);
        else if (str_starts(line, "shots=")) {
            const char *s = line + 6;
            g_shot_n = 0;
            while (*s && g_shot_n < MAX_SHOT) {
                if (*s == 'L')      g_shots[g_shot_n++].type = 0;
                else if (*s == 'B') g_shots[g_shot_n++].type = 1;
                s++;
            }
        }
        else if (str_starts(line, "x="))          x = to_int(line + 2);
        else if (str_starts(line, "y="))          y = to_int(line + 2);
        else if (str_starts(line, "topmost="))    top = to_int(line + 8);
        else if (str_starts(line, "hotkeys="))    hk  = to_int(line + 8);
        if (!eol) break;
        line = eol + 1;
    }
    (void)li; (void)bl;
    g_round       = clamp_i(g_round, 1, 9999);
    g_live_total  = clamp_i(g_live_total, 0, MAX_ROUND);
    g_blank_total = clamp_i(g_blank_total, 0, MAX_ROUND);
    if (g_live_total + g_blank_total == 0) { g_live_total = DEFAULT_LIVE; g_blank_total = DEFAULT_BLANK; }
    g_fired_live  = clamp_i(g_fired_live, 0, g_live_total);
    g_fired_blank = clamp_i(g_fired_blank, 0, g_blank_total);
    g_all_live    = clamp_i(g_all_live, 0, 999999);
    g_all_blank   = clamp_i(g_all_blank, 0, 999999);
    g_done_rounds = clamp_i(g_done_rounds, 0, 9999);
    g_topmost = top ? TRUE : FALSE;
    g_hotkeys = hk ? TRUE : FALSE;
    if (x > -100000 && y > -100000) { g_pos_x = x; g_pos_y = y; }
}
static void save_settings(HWND hwnd)
{
    RECT r;
    char buf[512];
    char path[MAX_PATH];
    char *p = buf;
    HANDLE f;
    DWORD wrote = 0;
    int i;

    GetWindowRect(hwnd, &r);
    str_copy(p, "round=");       p += 6;  p = put_num(p, g_round, 1);        *p++ = '\n';
    str_copy(p, "live_total=");  p += 11; p = put_num(p, g_live_total, 1);   *p++ = '\n';
    str_copy(p, "blank_total="); p += 12; p = put_num(p, g_blank_total, 1);  *p++ = '\n';
    str_copy(p, "fired_live=");  p += 11; p = put_num(p, g_fired_live, 1);   *p++ = '\n';
    str_copy(p, "fired_blank="); p += 12; p = put_num(p, g_fired_blank, 1);  *p++ = '\n';
    str_copy(p, "all_live=");    p += 9;  p = put_num(p, g_all_live, 1);     *p++ = '\n';
    str_copy(p, "all_blank=");   p += 10; p = put_num(p, g_all_blank, 1);    *p++ = '\n';
    str_copy(p, "done=");        p += 5;  p = put_num(p, g_done_rounds, 1);  *p++ = '\n';
    str_copy(p, "shots=");       p += 6;
    for (i = 0; i < g_shot_n && (p - buf) < 500; i++) *p++ = g_shots[i].type ? 'B' : 'L';
    *p++ = '\n';
    str_copy(p, "x=");           p += 2;  p = put_num(p, r.left, 1);         *p++ = '\n';
    str_copy(p, "y=");           p += 2;  p = put_num(p, r.top, 1);          *p++ = '\n';
    str_copy(p, "topmost=");     p += 8;  *p++ = g_topmost ? '1' : '0';      *p++ = '\n';
    str_copy(p, "hotkeys=");     p += 8;  *p++ = g_hotkeys ? '1' : '0';      *p++ = '\n';
    *p = 0;

    settings_path(path);
    f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return;
    WriteFile(f, buf, str_len(buf), &wrote, NULL);
    CloseHandle(f);
}

/* ============================ 布局 ============================ */
static void rect_win_buttons(RECT *top, RECT *undo, RECT *newr, RECT *close)
{
    close->right = WND_W - 8;      close->left = close->right - 24;   /* 288-312 */
    newr->right  = close->left - 2; newr->left = newr->right - 50;     /* 236-286 */
    undo->right  = newr->left - 2;  undo->left = undo->right - 46;     /* 188-234 */
    top->right   = undo->left - 2;  top->left  = top->right - 24;      /* 162-186 */
    top->top = undo->top = newr->top = close->top = 4;
    top->bottom = undo->bottom = newr->bottom = close->bottom = 24;
}
static void rect_record(int row, RECT *rec, RECT *adj)
{
    int cy = (row == 0) ? 158 : 206;
    rec->left = 12;  rec->right = 210;
    adj->left = 216; adj->right = WND_W - 12;
    rec->top = cy - 16; rec->bottom = cy + 16;
    adj->top = cy - 16; adj->bottom = cy + 16;
}
static void rect_presets(RECT *p)
{
    int i;
    for (i = 0; i < 5; i++) {
        p[i].left  = 20 + i * 57;
        p[i].right = p[i].left + 51;
        p[i].top = 74; p[i].bottom = 98;
    }
}
static void rect_steppers(RECT *ml, RECT *pl, RECT *mb, RECT *pb)
{
    ml->left = 60; ml->right = 84;  ml->top = 116; ml->bottom = 142;
    pl->left = 96; pl->right = 120; pl->top = 116; pl->bottom = 142;
    mb->left = 232; mb->right = 256; mb->top = 116; mb->bottom = 142;
    pb->left = 268; pb->right = 292; pb->top = 116; pb->bottom = 142;
    (void)0;
}
static void rect_dialog_buttons(RECT *start, RECT *cancel)
{
    start->left = 20;  start->right = 204;
    cancel->left = 212; cancel->right = 300;
    start->top = cancel->top = 200;
    start->bottom = cancel->bottom = 232;
}

static int hit_test(int x, int y)
{
    POINT pt; RECT a, b, c, d, p[5], ml, pl, mb, pb;
    pt.x = x; pt.y = y;

    if (g_setup_open) {
        rect_presets(p);
        for (int i = 0; i < 5; i++) if (PtInRect(&p[i], pt)) return HIT_P1 + i;
        rect_steppers(&ml, &pl, &mb, &pb);
        if (PtInRect(&ml, pt)) return HIT_ML;
        if (PtInRect(&pl, pt)) return HIT_PL;
        if (PtInRect(&mb, pt)) return HIT_MB;
        if (PtInRect(&pb, pt)) return HIT_PB;
        rect_dialog_buttons(&a, &b);
        if (PtInRect(&a, pt)) return HIT_START;
        if (PtInRect(&b, pt)) return HIT_CANCEL;
        return HIT_NONE;
    }

    rect_win_buttons(&a, &b, &c, &d);
    if (PtInRect(&b, pt)) return HIT_UNDO;
    if (PtInRect(&c, pt)) return HIT_NEW;
    if (PtInRect(&d, pt)) return HIT_CLOSE;
    if (PtInRect(&a, pt)) return HIT_TOP;

    rect_record(0, &a, &b);
    if (PtInRect(&a, pt)) return HIT_REC_LIVE;
    if (PtInRect(&b, pt)) return HIT_ADD_LIVE;
    rect_record(1, &a, &b);
    if (PtInRect(&a, pt)) return HIT_REC_BLANK;
    if (PtInRect(&b, pt)) return HIT_ADD_BLANK;
    return HIT_NONE;
}

/* ============================ 绘制 ============================ */
static void draw_button(HDC dc, const RECT *r, int id, const WCHAR *label, HFONT font,
                        COLORREF fill, COLORREF border, COLORREF text)
{
    BOOL hot = (g_hot == id), down = (g_down == id);
    COLORREF f = fill, bd = border, tx = text;
    if (down) { f = COL_BTN_HOT; }
    else if (hot) { f = RGB(GetRValue(fill) + 14, GetGValue(fill) + 14, GetBValue(fill) + 14); }
    if (g_flash > 0 && g_flash_id == id) { bd = COL_RED_HI; }
    fill_round(dc, r->left, r->top, r->right, r->bottom, 6, f, bd);
    text_in(dc, font, label, r, tx);
}

static void draw_header(HDC dc)
{
    RECT t, u, n, c;
    WCHAR buf[64];
    int pct = next_live_percent();

    rect_win_buttons(&t, &u, &n, &c);

    /* 标题 */
    text_left(dc, g_f_body, APP_TITLE, 12, 14, COL_FG);

    /* 置顶 */
    fill_round(dc, t.left, t.top, t.right, t.bottom, 5,
               g_topmost ? (g_hot == HIT_TOP ? RGB(96, 146, 226) : RGB(72, 114, 186)) : COL_BTN,
               g_topmost ? RGB(110, 160, 236) : COL_LINE);
    text_in(dc, g_f_small, TXT_TOP, &t, g_topmost ? RGB(240, 246, 255) : COL_MUTED);

    /* 撤销 */
    {
        BOOL has = (g_shot_n > 0);
        draw_button(dc, &u, HIT_UNDO, TXT_UNDO, g_f_small,
                    has ? COL_CARD2 : COL_CARD, COL_LINE, has ? COL_FG : RGB(90, 96, 97));
    }
    /* 新一轮 */
    draw_button(dc, &n, HIT_NEW, TXT_NEW, g_f_small, COL_CARD2, COL_LINE2, COL_FG);
    /* 关闭 */
    draw_button(dc, &c, HIT_CLOSE, L"\u00D7", g_f_body, COL_CARD, COL_LINE, COL_MUTED2);

    /* 本轮状态行 */
    wstr_set(buf, TXT_ROUND);
    wstr_num(buf + lstrlenW(buf), g_round, 1);
    wstr_cat(buf, TXT_ROUND2);
    wstr_cat(buf, L"  \u00B7  ");
    wstr_cat(buf, round_complete() ? TXT_DONE : TXT_ACTIVE);
    wstr_cat(buf, L"  \u00B7  \u5DF2\u8BB0\u5F55 ");
    {
        WCHAR num[8];
        wstr_num(num, g_shot_n, 1);
        wstr_cat(buf, num);
        wstr_cat(buf, TXT_SHELLS);
    }
    text_left(dc, g_f_small, buf, 12, 44, round_complete() ? COL_OK : COL_MUTED);

    /* 剩余总数（右） */
    {
        WCHAR num[8];
        wstr_set(buf, TXT_LEFT);
        wstr_cat(buf, L" ");
        wstr_num(num, remaining_live() + remaining_blank(), 1);
        wstr_cat(buf, num);
        wstr_cat(buf, L" ");
        wstr_cat(buf, TXT_SHELLS);
        text_right(dc, g_f_body, buf, WND_W - 12, 44, COL_FG);
    }

    /* 概率条 */
    {
        int live = remaining_live(), blank = remaining_blank();
        int tot = live + blank;
        int barL = 12, barR = WND_W - 12;
        int barY = 62;
        int w = barR - barL;
        int liveW = tot > 0 ? w * live / tot : 0;

        fill_rect(dc, barL, barY, barR, barY + 6, COL_BLANK_BG);
        if (liveW > 0) fill_rect(dc, barL, barY, barL + liveW, barY + 6, COL_RED);
        if (liveW < w) fill_rect(dc, barL + liveW, barY, barR, barY + 6, RGB(90, 96, 94));

        text_left(dc, g_f_tiny, TXT_CHANCE, 12, 76, COL_MUTED);
        if (pct >= 0) {
            wstr_num(buf, pct, 1);
            wstr_cat(buf, L"%");
            text_left(dc, g_f_body, buf, 78, 76, COL_RED_HI);
        } else {
            text_left(dc, g_f_body, L"--", 78, 76, COL_MUTED);
        }
    }

    /* 射击序列（右对齐，新的在右边） */
    {
        int shown = g_shot_n > 20 ? 20 : g_shot_n;
        int x = WND_W - 14;
        for (int i = 0; i < shown; i++) {
            int idx = g_shot_n - 1 - i;
            draw_pip(dc, x - 5, 76, g_shots[idx].type == 0, i == 0);
            x -= 11;
        }
    }
}

static void draw_belt(HDC dc)
{
    /* 弹仓：本轮尚未发射的子弹（实弹在前、空弹在后）；右侧显示已发射格 */
    int live = remaining_live(), blank = remaining_blank();
    int fired = g_fired_live + g_fired_blank;
    int x = 44, i;
    int y = 97;

    text_left(dc, g_f_tiny, L"\u5F39\u4ED3", 12, 100, COL_MUTED);

    for (i = 0; i < live && i < 12; i++) {
        draw_shell(dc, x, y, TRUE, 0, FALSE);
        x += 13;
    }
    for (i = 0; i < blank && i < 12; i++) {
        draw_shell(dc, x, y, FALSE, 0, FALSE);
        x += 13;
    }
    if (live + blank == 0) {
        text_left(dc, g_f_tiny, L"\u672C\u8F6E\u5F39\u836F\u5DF2\u5168\u90E8\u8BB0\u5F55", 44, 100, COL_OK);
    }
    /* 已发射格（右侧，暗色小格） */
    if (fired > 0) {
        int rx = WND_W - 14;
        int n = fired > 8 ? 8 : fired;
        for (i = 0; i < n; i++) {
            fill_rect(dc, rx - 7, y - 6, rx, y + 6, RGB(38, 42, 44));
            draw_line(dc, rx - 7, y - 6, rx - 1, y - 6, COL_LINE);
            rx -= 10;
        }
    }
}

static void draw_row(HDC dc, int row)
{
    BOOL live = (row == 0);
    int cy = live ? 158 : 206;
    int rem = live ? remaining_live() : remaining_blank();
    int total = live ? g_live_total : g_blank_total;
    int fired = live ? g_fired_live : g_fired_blank;
    RECT rec, adj;
    WCHAR buf[96], num[8];
    COLORREF accent = live ? COL_RED : COL_BLANK;
    COLORREF accentBg = live ? COL_RED_BG : COL_BLANK_BG;
    COLORREF border = live ? RGB(120, 52, 46) : RGB(78, 84, 82);

    rect_record(row, &rec, &adj);

    /* 行卡片 */
    fill_round(dc, 8, cy - 21, WND_W - 8, cy + 21, 8, COL_CARD, COL_LINE);

    /* 类型标签 + 本轮已记录 */
    text_left(dc, g_f_bold, live ? TXT_LIVE : TXT_BLANK, 20, cy - 8, accent);
    wstr_set(buf, L"\u5DF2\u8BB0\u5F55 ");
    wstr_num(num, fired, 1);
    wstr_cat(buf, num);
    wstr_cat(buf, L" / ");
    wstr_num(num, total, 1);
    wstr_cat(buf, num);
    text_left(dc, g_f_tiny, buf, 58, cy - 9, COL_MUTED);

    /* 剩余大数字 */
    {
        WCHAR big[8];
        wstr_num(big, rem, 1);
        text_right(dc, g_f_bignum, big, 178, cy, rem > 0 ? accent : RGB(96, 100, 100));
    }

    /* 记录按钮 */
    {
        BOOL enabled = (rem > 0) && !round_complete();
        WCHAR label[32];
        wstr_set(label, TXT_REC);
        wstr_cat(label, live ? TXT_LIVE : TXT_BLANK);
        if (rem <= 0) {
            fill_round(dc, rec.left, rec.top, rec.right, rec.bottom, 6, COL_CARD2, COL_LINE);
            text_in(dc, g_f_small, label, &rec, RGB(96, 100, 100));
        } else if (!enabled) {
            fill_round(dc, rec.left, rec.top, rec.right, rec.bottom, 6, COL_CARD2, COL_LINE);
            text_in(dc, g_f_small, label, &rec, COL_MUTED);
        } else {
            COLORREF f = accentBg;
            if (g_down == (live ? HIT_REC_LIVE : HIT_REC_BLANK)) f = accent;
            else if (g_hot == (live ? HIT_REC_LIVE : HIT_REC_BLANK))
                f = live ? RGB(84, 38, 34) : RGB(52, 60, 60);
            fill_round(dc, rec.left, rec.top, rec.right, rec.bottom, 6, f, border);
            text_in(dc, g_f_small, label, &rec, COL_FG);
        }
    }

    /* 修正 + */
    draw_button(dc, &adj, live ? HIT_ADD_LIVE : HIT_ADD_BLANK, L"+\u4FEE\u6B63", g_f_small,
                COL_CARD2, COL_LINE, COL_MUTED2);
}

static void draw_setup(HDC dc)
{
    RECT p[5], ml, pl, mb, pb, start, cancel;
    WCHAR buf[64], num[8];
    int i, total = g_set_live + g_set_blank;
    static const int PL[5] = { 2, 3, 3, 4, 4 };
    static const int PB[5] = { 2, 2, 3, 2, 3 };
    static const WCHAR *LT[5] = { L"2/2", L"3/2", L"3/3", L"4/2", L"4/3" };

    /* 遮罩 */
    fill_rect(dc, 0, TITLE_H, WND_W, WND_H, RGB(11, 13, 14));
    fill_round(dc, 8, TITLE_H + 6, WND_W - 8, WND_H - 8, 10, COL_CARD, COL_LINE2);

    text_left(dc, g_f_bold, TXT_SETUP, 20, 50, COL_FG);
    /* 本轮总量（整串右对齐，避免与标题重叠） */
    {
        WCHAR t[32];
        wstr_set(t, TXT_TOTAL);
        wstr_cat(t, L" ");
        wstr_num(num, total, 1);
        wstr_cat(t, num);
        wstr_cat(t, L" ");
        wstr_cat(t, TXT_SHELLS);
        text_right(dc, g_f_small, t, WND_W - 20, 50, COL_MUTED2);
    }

    /* 预设 */
    rect_presets(p);
    for (i = 0; i < 5; i++) {
        BOOL active = (g_set_live == PL[i] && g_set_blank == PB[i]);
        BOOL hot = (g_hot == HIT_P1 + i);
        fill_round(dc, p[i].left, p[i].top, p[i].right, p[i].bottom, 6,
                   active ? COL_RED_BG : (hot ? COL_BTN_HOT : COL_CARD2),
                   active ? COL_RED : COL_LINE);
        text_in(dc, g_f_small, LT[i], &p[i], active ? COL_FG : COL_MUTED2);
    }

    /* 步进器 */
    rect_steppers(&ml, &pl, &mb, &pb);
    text_left(dc, g_f_tiny, TXT_LIVE, 20, 129, COL_RED);
    text_in(dc, g_f_num, L"-", &ml, COL_FG);
    {
        WCHAR v[4];
        wstr_num(v, g_set_live, 1);
        text_center(dc, g_f_bold, v, 90, 129, COL_FG);
    }
    text_in(dc, g_f_num, L"+", &pl, COL_FG);

    text_left(dc, g_f_tiny, TXT_BLANK, 176, 129, COL_BLANK);
    text_in(dc, g_f_num, L"-", &mb, COL_FG);
    {
        WCHAR v[4];
        wstr_num(v, g_set_blank, 1);
        text_center(dc, g_f_bold, v, 262, 129, COL_FG);
    }
    text_in(dc, g_f_num, L"+", &pb, COL_FG);

    /* 预览弹仓 */
    {
        int x = 20, y = 168;
        for (i = 0; i < g_set_live && i < 8; i++) { draw_shell(dc, x, y, TRUE, 0, FALSE); x += 13; }
        for (i = 0; i < g_set_blank && i < 8; i++) { draw_shell(dc, x, y, FALSE, 0, FALSE); x += 13; }
        if (g_set_live + g_set_blank > 16)
            text_left(dc, g_f_tiny, L"...", x, y + 10, COL_MUTED);
    }

    if (g_set_err) text_right(dc, g_f_tiny, TXT_ERR, WND_W - 20, 129, COL_RED_HI);

    /* 按钮 */
    rect_dialog_buttons(&start, &cancel);
    {
        BOOL hot = (g_hot == HIT_START), down = (g_down == HIT_START);
        fill_round(dc, start.left, start.top, start.right, start.bottom, 7,
                   down ? COL_RED_HI : (hot ? RGB(232, 88, 78) : COL_RED), COL_RED_HI);
        text_in(dc, g_f_bold, TXT_START, &start, RGB(255, 246, 244));
    }
    draw_button(dc, &cancel, HIT_CANCEL, TXT_CANCEL, g_f_small, COL_CARD2, COL_LINE, COL_MUTED2);
    (void)buf; (void)num;
}

static void paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc0 = BeginPaint(hwnd, &ps);
    RECT cr;
    HDC dc;
    HBITMAP bmp;
    HGDIOBJ obmp;
    int w, h;

    GetClientRect(hwnd, &cr);
    w = cr.right; h = cr.bottom;

    dc = CreateCompatibleDC(dc0);
    bmp = CreateCompatibleBitmap(dc0, w, h);
    obmp = SelectObject(dc, bmp);
    SetBkMode(dc, TRANSPARENT);

    fill_rect(dc, 0, 0, w, h, COL_BG);

    if (g_setup_open) {
        draw_setup(dc);
    } else {
        fill_rect(dc, 0, 0, w, TITLE_H, COL_CARD);
        draw_line(dc, 0, TITLE_H - 1, w, TITLE_H - 1, COL_LINE);
        draw_header(dc);
        draw_line(dc, 8, 88, w - 8, 88, COL_LINE);
        draw_belt(dc);
        draw_row(dc, 0);
        draw_row(dc, 1);
        text_center(dc, g_f_tiny, TXT_HINT, w / 2, h - 10, RGB(96, 102, 103));
    }

    BitBlt(dc0, 0, 0, w, h, dc, 0, 0, SRCCOPY);

    SelectObject(dc, obmp);
    DeleteObject(bmp);
    DeleteDC(dc);
    EndPaint(hwnd, &ps);
}

/* ============================ 窗口过程 ============================ */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        load_settings();
        g_f_tiny   = CreateFontW(-11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_UI);
        g_f_small  = CreateFontW(-13, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_UI);
        g_f_body   = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_UI);
        g_f_bold   = CreateFontW(-15, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_UI);
        g_f_num    = CreateFontW(-18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_NUM);
        g_f_bignum = CreateFontW(-26, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, FONT_NUM);
        if (g_topmost)
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        if (g_pos_x > -100000)
            SetWindowPos(hwnd, NULL, g_pos_x, g_pos_y, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        if (g_hotkeys) {
            RegisterHotKey(hwnd, HOT_LIVE,  MOD_CONTROL | MOD_ALT, 'S');
            RegisterHotKey(hwnd, HOT_BLANK, MOD_CONTROL | MOD_ALT, 'D');
            RegisterHotKey(hwnd, HOT_UNDO,  MOD_CONTROL | MOD_ALT, 'Z');
            RegisterHotKey(hwnd, HOT_CLEAR, MOD_CONTROL | MOD_ALT, 'R');
        }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        paint(hwnd);
        return 0;

    case WM_TIMER:
        if (wp == TIMER_ANIM) {
            if (g_flash > 0) g_flash--;
            if (g_flash == 0) { KillTimer(hwnd, TIMER_ANIM); g_flash_id = 0; }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_MOUSEMOVE:
    {
        int h = hit_test((int)(short)LOWORD(lp), (int)(short)HIWORD(lp));
        if (h != g_hot) {
            TRACKMOUSEEVENT tme;
            g_hot = h;
            InvalidateRect(hwnd, NULL, FALSE);
            if (h == HIT_NONE) {
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = 0;
                TrackMouseEvent(&tme);
            }
        }
        return 0;
    }
    case WM_MOUSELEAVE:
        if (g_hot != HIT_NONE) { g_hot = HIT_NONE; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;

    case WM_LBUTTONDOWN:
    {
        int h = hit_test((int)(short)LOWORD(lp), (int)(short)HIWORD(lp));
        g_down = h;
        switch (h) {
        case HIT_TOP:
            g_topmost = !g_topmost;
            SetWindowPos(hwnd, g_topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            break;
        case HIT_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case HIT_UNDO:
            undo_shot(hwnd);
            break;
        case HIT_NEW:
            g_setup_open = TRUE;
            g_set_live = g_live_total;
            g_set_blank = g_blank_total;
            g_set_err = FALSE;
            break;
        case HIT_REC_LIVE:  record_shot(hwnd, 0); break;
        case HIT_REC_BLANK: record_shot(hwnd, 1); break;
        case HIT_ADD_LIVE:  adjust_back(hwnd, 0); break;
        case HIT_ADD_BLANK: adjust_back(hwnd, 1); break;
        case HIT_P1: g_set_live = 2; g_set_blank = 2; g_set_err = FALSE; break;
        case HIT_P2: g_set_live = 3; g_set_blank = 2; g_set_err = FALSE; break;
        case HIT_P3: g_set_live = 3; g_set_blank = 3; g_set_err = FALSE; break;
        case HIT_P4: g_set_live = 4; g_set_blank = 2; g_set_err = FALSE; break;
        case HIT_P5: g_set_live = 4; g_set_blank = 3; g_set_err = FALSE; break;
        case HIT_ML: g_set_live = clamp_i(g_set_live - 1, 0, MAX_ROUND); g_set_err = FALSE; break;
        case HIT_PL: g_set_live = clamp_i(g_set_live + 1, 0, MAX_ROUND); g_set_err = FALSE; break;
        case HIT_MB: g_set_blank = clamp_i(g_set_blank - 1, 0, MAX_ROUND); g_set_err = FALSE; break;
        case HIT_PB: g_set_blank = clamp_i(g_set_blank + 1, 0, MAX_ROUND); g_set_err = FALSE; break;
        case HIT_START:
            start_new_round(hwnd, g_set_live, g_set_blank);
            if (g_set_err) MessageBeep(MB_ICONHAND);
            break;
        case HIT_CANCEL:
            g_setup_open = FALSE;
            g_set_err = FALSE;
            break;
        default:
            break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_RBUTTONDOWN:
        /* 右键任意处 = 撤销上一发 */
        undo_shot(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_LBUTTONUP:
        if (g_down != HIT_NONE) { g_down = HIT_NONE; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;

    case WM_MOUSEWHEEL:
    {
        POINT pt;
        int d = ((short)HIWORD(wp) > 0) ? 1 : -1;
        pt.x = (int)(short)LOWORD(lp);
        pt.y = (int)(short)HIWORD(lp);
        ScreenToClient(hwnd, &pt);
        if (!g_setup_open) {
            if (pt.y >= 137 && pt.y <= 179) {
                if (d > 0) record_shot(hwnd, 0); else adjust_back(hwnd, 0);
            } else if (pt.y >= 185 && pt.y <= 227) {
                if (d > 0) record_shot(hwnd, 1); else adjust_back(hwnd, 1);
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_HOTKEY:
        if (wp == HOT_LIVE)       record_shot(hwnd, 0);
        else if (wp == HOT_BLANK) record_shot(hwnd, 1);
        else if (wp == HOT_UNDO)  undo_shot(hwnd);
        else if (wp == HOT_CLEAR) clear_all(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            if (g_setup_open) { g_setup_open = FALSE; g_set_err = FALSE; InvalidateRect(hwnd, NULL, FALSE); }
            else DestroyWindow(hwnd);
            return 0;
        }
        if (wp == '1') { record_shot(hwnd, 0); InvalidateRect(hwnd, NULL, FALSE); return 0; }
        if (wp == '2') { record_shot(hwnd, 1); InvalidateRect(hwnd, NULL, FALSE); return 0; }
        if (wp == 'U' || (wp == 'Z' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            undo_shot(hwnd); InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        if (wp == 'N') {
            g_setup_open = TRUE; g_set_live = g_live_total; g_set_blank = g_blank_total;
            g_set_err = FALSE; InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        if (g_setup_open && wp == VK_RETURN) {
            start_new_round(hwnd, g_set_live, g_set_blank);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_NCHITTEST:
    {
        POINT pt;
        RECT t, u, n, c;
        pt.x = (int)(short)LOWORD(lp);
        pt.y = (int)(short)HIWORD(lp);
        ScreenToClient(hwnd, &pt);
        if (!g_setup_open) {
            rect_win_buttons(&t, &u, &n, &c);
            if (PtInRect(&t, pt) || PtInRect(&u, pt) || PtInRect(&n, pt) || PtInRect(&c, pt))
                return HTCLIENT;
            if (pt.y >= 0 && pt.y < TITLE_H) return HTCAPTION;
        }
        return HTCLIENT;
    }

    case WM_SYSCOMMAND:
        if ((wp & 0xFFF0) == SC_KEYMENU) return 0;
        break;

    case WM_DESTROY:
        if (g_hotkeys) {
            UnregisterHotKey(hwnd, HOT_LIVE);
            UnregisterHotKey(hwnd, HOT_BLANK);
            UnregisterHotKey(hwnd, HOT_UNDO);
            UnregisterHotKey(hwnd, HOT_CLEAR);
        }
        save_settings(hwnd);
        if (g_f_tiny)   DeleteObject(g_f_tiny);
        if (g_f_small)  DeleteObject(g_f_small);
        if (g_f_body)   DeleteObject(g_f_body);
        if (g_f_bold)   DeleteObject(g_f_bold);
        if (g_f_num)    DeleteObject(g_f_num);
        if (g_f_bignum) DeleteObject(g_f_bignum);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ============================ 入口 ============================ */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEXW wc;
    RECT r;
    HWND hwnd;
    MSG msg;
    int x, y, ww, wh;

    (void)hPrev; (void)lpCmdLine; (void)nCmdShow;

    mem_zero(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hIcon         = LoadIconW(hInst, (LPCWSTR)1);
    wc.hIconSm       = LoadIconW(hInst, (LPCWSTR)1);
    wc.hbrBackground = NULL;
    wc.lpszClassName = WND_CLASS;
    if (!RegisterClassExW(&wc)) return 1;

    r.left = 0; r.top = 0; r.right = WND_W; r.bottom = WND_H;
    AdjustWindowRectEx(&r, WS_POPUP, FALSE, WS_EX_TOOLWINDOW);
    ww = r.right - r.left;
    wh = r.bottom - r.top;
    x = (GetSystemMetrics(SM_CXSCREEN) - ww) / 2;
    y = 80;

    hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, WND_CLASS, APP_TITLE, WS_POPUP,
                           x, y, ww, wh, NULL, NULL, hInst, NULL);
    if (!hwnd) return 2;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
