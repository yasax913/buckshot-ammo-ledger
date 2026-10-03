/* make_icon.c —— 生成 app.ico（32bpp BMP 图标，含 16/32/48 三种尺寸）
 * 编译运行： gcc -O2 make_icon.c -o build\make_icon.exe && build\make_icon.exe
 * 图标内容：深蓝圆底 + 实弹（红弹头、黄铜弹壳）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { unsigned char b, g, r, a; } RGBA;

static RGBA pix[48][48];

static void set(int x, int y, RGBA c) { if (x >= 0 && x < 48 && y >= 0 && y < 48) pix[y][x] = c; }

/* 以 (cx,cy) 为中心、半径 r 的实心圆，边缘 1px 抗锯齿 */
static void disc(double cx, double cy, double r, RGBA col)
{
    int x, y;
    for (y = 0; y < 48; y++) {
        for (x = 0; x < 48; x++) {
            double dx = (x + 0.5) - cx, dy = (y + 0.5) - cy;
            double d = dx * dx + dy * dy;
            double outer = (r + 0.5) * (r + 0.5), inner = (r - 0.5) * (r - 0.5);
            if (d <= inner) {
                pix[y][x] = col;
            } else if (d < outer) {
                double t = (outer - d) / (outer - inner); /* 0..1 */
                RGBA p = pix[y][x];
                double na = col.a * t + p.a * (1.0 - t);
                if (na <= 0.0) { pix[y][x].a = 0; continue; }
                pix[y][x].r = (unsigned char)((col.r * t * col.a + p.r * (1 - t) * p.a) / na);
                pix[y][x].g = (unsigned char)((col.g * t * col.a + p.g * (1 - t) * p.a) / na);
                pix[y][x].b = (unsigned char)((col.b * t * col.a + p.b * (1 - t) * p.a) / na);
                pix[y][x].a = (unsigned char)(na + 0.5);
            }
        }
    }
}
static void rect(int x0, int y0, int x1, int y1, RGBA c)
{
    int x, y;
    for (y = y0; y <= y1; y++) for (x = x0; x <= x1; x++) set(x, y, c);
}
/* 竖直渐变的圆角矩形（弹壳） */
static void capsule(int x0, int y0, int x1, int y1, RGBA top, RGBA bot)
{
    int x, y;
    double h = (double)(y1 - y0);
    for (y = y0; y <= y1; y++) {
        double t = h > 0 ? (y - y0) / h : 0.0;
        RGBA c;
        c.r = (unsigned char)(top.r + (bot.r - top.r) * t);
        c.g = (unsigned char)(top.g + (bot.g - top.g) * t);
        c.b = (unsigned char)(top.b + (bot.b - top.b) * t);
        c.a = 255;
        for (x = x0; x <= x1; x++) set(x, y, c);
    }
}
static void tri(double ax, double ay, double bx, double by, double cx, double cy, RGBA col)
{
    int x, y;
    double minx = ax, maxx = ax, miny = ay, maxy = ay;
    if (bx < minx) minx = bx; if (cx < minx) minx = cx;
    if (bx > maxx) maxx = bx; if (cx > maxx) maxx = cx;
    if (by < miny) miny = by; if (cy < miny) miny = cy;
    if (by > maxy) maxy = by; if (cy > maxy) maxy = cy;
    for (y = (int)miny - 1; y <= (int)maxy + 1; y++) {
        for (x = (int)minx - 1; x <= (int)maxx + 1; x++) {
            double px = x + 0.5, py = y + 0.5;
            double d1 = (px - bx) * (ay - by) - (ax - bx) * (py - by);
            double d2 = (px - cx) * (by - cy) - (bx - cx) * (py - cy);
            double d3 = (px - ax) * (cy - ay) - (cx - ax) * (py - ay);
            int neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            int pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            if (!(neg && pos)) set(x, y, col);
        }
    }
}

static void render_scene(void)
{
    RGBA clear = { 0, 0, 0, 0 };
    RGBA bgA   = { 42, 46, 56, 255 };   /* 深灰蓝底 */
    RGBA caseA = { 66, 190, 236, 255 }; /* 弹壳亮 */
    RGBA caseB = { 32, 118, 178, 255 }; /* 弹壳暗 */
    RGBA tipA  = { 255, 128, 116, 255 };
    RGBA tipB  = { 214, 42, 36, 255 };
    RGBA shine = { 255, 255, 255, 120 };
    int i;
    for (i = 0; i < 48 * 48; i++) pix[i / 48][i % 48] = clear;

    /* 圆底 */
    disc(24.0, 24.0, 22.0, bgA);
    /* 弹壳（黄铜/蓝色金属，带明暗） */
    capsule(14, 22, 33, 41, caseA, caseB);
    /* 底缘（弹壳底座略亮） */
    rect(14, 39, 33, 41, caseB);
    /* 弹头（红色，从弹壳顶部收成尖） */
    rect(15, 19, 32, 23, tipA);
    tri(16.0, 23.0, 31.0, 23.0, 23.5, 6.0, tipB);
    /* 弹壳高光 */
    rect(17, 24, 19, 38, shine);
}

static void write_le32(FILE *f, unsigned v)
{
    fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f);
    fputc((v >> 16) & 0xFF, f); fputc((v >> 24) & 0xFF, f);
}
static void write_le16(FILE *f, unsigned v) { fputc(v & 0xFF, f); fputc((v >> 8) & 0xFF, f); }

/* 把当前 pix 下采样到 size×size，写入 ICO 的 BMP 数据段 */
static void write_bmp(FILE *f, int size, int *out_bytes)
{
    int x, y, and_row = ((size + 31) / 32) * 4;
    long start = ftell(f);
    write_le32(f, 40);                     /* BITMAPINFOHEADER */
    write_le32(f, size);
    write_le32(f, size * 2);               /* 高度 = 图像 + 掩码 */
    write_le16(f, 1);
    write_le16(f, 32);
    write_le32(f, 0);                      /* BI_RGB */
    write_le32(f, size * size * 4);
    write_le32(f, 0); write_le32(f, 0); write_le32(f, 0); write_le32(f, 0);
    /* XOR 数据：自下而上，BGRA */
    for (y = size - 1; y >= 0; y--) {
        for (x = 0; x < size; x++) {
            double fx = (x + 0.5) * 48.0 / size - 0.5;
            double fy = (y + 0.5) * 48.0 / size - 0.5;
            int sx = (int)(fx + 0.5), sy = (int)(fy + 0.5);
            RGBA c;
            if (sx < 0) sx = 0; if (sx > 47) sx = 47;
            if (sy < 0) sy = 0; if (sy > 47) sy = 47;
            c = pix[sy][sx];
            fputc(c.b, f); fputc(c.g, f); fputc(c.r, f); fputc(c.a, f);
        }
    }
    /* AND 掩码：全 0（由 alpha 决定透明） */
    for (y = 0; y < size; y++) {
        for (x = 0; x < and_row; x++) fputc(0, f);
    }
    if (out_bytes) *out_bytes = (int)(ftell(f) - start);
}

int main(void)
{
    const int sizes[3] = { 16, 32, 48 };
    FILE *f;
    long after_header;
    int i, bytes[3];
    int total = 6 + 16 * 3;

    render_scene();

    f = fopen("app.ico", "wb");
    if (!f) { fprintf(stderr, "cannot write app.ico\n"); return 1; }

    write_le16(f, 0);        /* reserved */
    write_le16(f, 1);        /* type = icon */
    write_le16(f, 3);        /* count */
    after_header = total;
    (void)after_header;
    /* 目录项先占位，稍后回填偏移 */
    for (i = 0; i < 3; i++) {
        fputc(sizes[i] == 256 ? 0 : sizes[i], f);
        fputc(sizes[i] == 256 ? 0 : sizes[i], f);
        fputc(0, f); fputc(0, f);
        write_le16(f, 1); write_le16(f, 32);
        write_le32(f, 0);   /* size，回填 */
        write_le32(f, 0);   /* offset，回填 */
    }

    for (i = 0; i < 3; i++) bytes[i] = 0;
    for (i = 0; i < 3; i++) write_bmp(f, sizes[i], &bytes[i]);

    /* 回填目录 */
    {
        int offset = total, k;
        for (k = 0; k < 3; k++) {
            long pos = 6 + k * 16;
            fseek(f, pos + 8, SEEK_SET);
            write_le32(f, (unsigned)bytes[k]);
            write_le32(f, (unsigned)offset);
            offset += bytes[k];
        }
    }
    fclose(f);
    printf("app.ico written: %d bytes\n", total + bytes[0] + bytes[1] + bytes[2]);
    return 0;
}
