/*
 * DigitalClock — versione cross-platform (Windows / Linux)
 * SDL2 + libcurl, nessuna dipendenza da font esterni (font bitmap incluso)
 *
 * Compilazione: vedi Makefile
 */

#include <SDL.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define PI_VAL 3.14159265358979323846

/* ── Coordinate città (Torino) ─────────────────────────── */
#define CITY_LAT "45.0703"
#define CITY_LON "7.6869"

/* ── Palette ───────────────────────────────────────────── */
static const SDL_Color COL_BG   = {0,   0,   0,   255};
static const SDL_Color COL_ON   = {220, 220, 220, 255};
static const SDL_Color COL_OFF  = {15,  15,  15,  255};
static const SDL_Color COL_TEXT = {160, 160, 160, 255};
static const SDL_Color COL_DIM  = {60,  60,  60,  255};

/* ── Segmenti 7-bar ────────────────────────────────────── */
static const unsigned char seg7[10] = {
    0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};

/* ── Giorni settimana (italiano, senza accenti — stile retro) ── */
static const char *GIORNI[7] = {
    "DOMENICA","LUNEDI","MARTEDI","MERCOLEDI","GIOVEDI","VENERDI","SABATO"
};

/* ── Font bitmap 5x7 (A-Z, 0-9, spazio, : / - . e '^' = gradi) ── */
typedef struct { char c; unsigned char rows[7]; } Glyph;

static const Glyph FONT[] = {
    {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
    {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
    {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
    {'3', {0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}},
    {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
    {'5', {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}},
    {'6', {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}},
    {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
    {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
    {'9', {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}},
    {'A', {0x04,0x0A,0x11,0x11,0x1F,0x11,0x11}},
    {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
    {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
    {'D', {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
    {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
    {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
    {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}},
    {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
    {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
    {'J', {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}},
    {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
    {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
    {'M', {0x11,0x1B,0x15,0x11,0x11,0x11,0x11}},
    {'N', {0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
    {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
    {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
    {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
    {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
    {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
    {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
    {'V', {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}},
    {'W', {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
    {'X', {0x11,0x0A,0x04,0x04,0x04,0x0A,0x11}},
    {'Y', {0x11,0x0A,0x04,0x04,0x04,0x04,0x04}},
    {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},
    {' ', {0,0,0,0,0,0,0}},
    {':', {0,4,0,0,4,0,0}},
    {'/', {1,2,4,4,8,16,0}},
    {'-', {0,0,0,0x1F,0,0,0}},
    {'.', {0,0,0,0,0,4,0}},
    {'^', {0x0C,0x12,0x0C,0,0,0,0}},   /* simbolo gradi °  */
};
#define FONT_COUNT (int)(sizeof(FONT)/sizeof(FONT[0]))

static const Glyph* FindGlyph(char c)
{
    c = (char)toupper((unsigned char)c);
    for (int i = 0; i < FONT_COUNT; i++)
        if (FONT[i].c == c) return &FONT[i];
    return NULL;
}

/* ── Stato applicazione ────────────────────────────────── */
typedef struct {
    SDL_Window   *win;
    SDL_Renderer *ren;
    int running;
    int fullscreen;
    int format24;

    float temp;
    int   wcode;

    int alarm_set, alarm_hour, alarm_min;
    int alarm_firing;
    Uint32 alarm_fire_start;

    int show_help;
    int show_alarm_edit;
    int edit_field, edit_hour, edit_min, edit_enabled;
} AppState;

static AppState app;
static SDL_mutex *g_weatherMutex = NULL;
static SDL_AudioDeviceID g_audioDev = 0;
static Uint8  *g_beepBuf = NULL;
static Uint32  g_beepLen = 0;

/* ─────────────────────────────────────────────────────────
 *  Disegno segmenti / cifre / testo
 * ───────────────────────────────────────────────────────── */
static void DrawSegment(SDL_Renderer *r, int x, int y, int w, int t, int dir, int on)
{
    SDL_Color c = on ? COL_ON : COL_OFF;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rect;
    if (dir == 0) { rect.x = x; rect.y = y; rect.w = w; rect.h = t; }
    else          { rect.x = x; rect.y = y; rect.w = t; rect.h = w; }
    SDL_RenderFillRect(r, &rect);
}

static void DrawDigit(SDL_Renderer *r, int digit, int x, int y, int W, int H, int T)
{
    if (digit < 0 || digit > 9) return;
    unsigned char mask = seg7[digit];
    int gap  = T / 3;
    int segW = W - 2*T - 2*gap;
    int half = (H - 3*T - 2*gap) / 2;

    DrawSegment(r, x+T+gap, y,                          segW, T, 0, (mask>>0)&1);
    DrawSegment(r, x+W-T,   y+T+gap,                    half, T, 1, (mask>>1)&1);
    DrawSegment(r, x+W-T,   y+T+gap+half+T+gap,         half, T, 1, (mask>>2)&1);
    DrawSegment(r, x+T+gap, y+H-T,                      segW, T, 0, (mask>>3)&1);
    DrawSegment(r, x,       y+T+gap+half+T+gap,         half, T, 1, (mask>>4)&1);
    DrawSegment(r, x,       y+T+gap,                    half, T, 1, (mask>>5)&1);
    DrawSegment(r, x+T+gap, y+T+gap+half+gap,           segW, T, 0, (mask>>6)&1);
}

static void DrawColon(SDL_Renderer *r, int x, int cy, int rad, int visible)
{
    SDL_Color c = visible ? COL_ON : COL_BG;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect top = { x-rad, cy-rad*5, rad*2, rad*2 };
    SDL_Rect bot = { x-rad, cy+rad*3, rad*2, rad*2 };
    SDL_RenderFillRect(r, &top);
    SDL_RenderFillRect(r, &bot);
}

static void DrawChar(SDL_Renderer *r, char c, int x, int y, int scale, SDL_Color col)
{
    const Glyph *g = FindGlyph(c);
    if (!g) return;
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    for (int row = 0; row < 7; row++) {
        unsigned char bits = g->rows[row];
        for (int cb = 0; cb < 5; cb++) {
            if (bits & (1 << (4-cb))) {
                SDL_Rect px = { x+cb*scale, y+row*scale, scale, scale };
                SDL_RenderFillRect(r, &px);
            }
        }
    }
}

static int TextWidth(const char *s, int scale) { return (int)strlen(s) * 6 * scale; }

static void DrawText(SDL_Renderer *r, const char *s, int x, int y, int scale, SDL_Color col)
{
    int cx = x;
    for (const char *p = s; *p; p++) {
        DrawChar(r, *p, cx, y, scale, col);
        cx += 6 * scale;
    }
}

/* ─────────────────────────────────────────────────────────
 *  Icona finestra (generata in memoria, no file .ico)
 * ───────────────────────────────────────────────────────── */
static void SurfFillRect(SDL_Surface *s, int x, int y, int w, int h, Uint32 color)
{
    SDL_Rect r = { x, y, w, h };
    SDL_FillRect(s, &r, color);
}

static void SurfDrawDigit(SDL_Surface *s, int digit, int x, int y, int W, int H, int T, Uint32 color)
{
    unsigned char mask = seg7[digit];
    int gap  = T / 3;
    int segW = W - 2*T - 2*gap;
    int half = (H - 3*T - 2*gap) / 2;
    if (mask&1)  SurfFillRect(s, x+T+gap, y,                          segW, T, color);
    if (mask&2)  SurfFillRect(s, x+W-T,   y+T+gap,                    T, half, color);
    if (mask&4)  SurfFillRect(s, x+W-T,   y+T+gap+half+T+gap,         T, half, color);
    if (mask&8)  SurfFillRect(s, x+T+gap, y+H-T,                      segW, T, color);
    if (mask&16) SurfFillRect(s, x,       y+T+gap+half+T+gap,         T, half, color);
    if (mask&32) SurfFillRect(s, x,       y+T+gap,                    T, half, color);
    if (mask&64) SurfFillRect(s, x+T+gap, y+T+gap+half+gap,           segW, T, color);
}

static SDL_Surface* CreateClockIconSurface(void)
{
    int sz = 32;
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, sz, sz, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surf) return NULL;
    Uint32 black = SDL_MapRGBA(surf->format, 0, 0, 0, 255);
    Uint32 white = SDL_MapRGBA(surf->format, 220, 220, 220, 255);
    SDL_FillRect(surf, NULL, black);
    SurfDrawDigit(surf, 1, 2,  4, 12, 24, 2, white);
    SurfDrawDigit(surf, 2, 17, 4, 12, 24, 2, white);
    return surf;
}

/* ─────────────────────────────────────────────────────────
 *  Audio: beep sveglia generato al volo (nessun file .wav)
 * ───────────────────────────────────────────────────────── */
static void InitAudio(void)
{
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 1024;
    g_audioDev = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);
}

static void GenerateBeep(void)
{
    int freq = 1000, rate = 44100, ms = 200;
    int samples = rate * ms / 1000;
    g_beepLen = (Uint32)(samples * sizeof(Sint16));
    g_beepBuf = (Uint8*)malloc(g_beepLen);
    Sint16 *p = (Sint16*)g_beepBuf;
    for (int i = 0; i < samples; i++) {
        double t = (double)i / rate;
        p[i] = (Sint16)(3000.0 * sin(2.0 * PI_VAL * freq * t));
    }
}

static void PlayBeep(void)
{
    if (!g_audioDev || !g_beepBuf) return;
    SDL_ClearQueuedAudio(g_audioDev);
    SDL_QueueAudio(g_audioDev, g_beepBuf, g_beepLen);
    SDL_PauseAudioDevice(g_audioDev, 0);
}

/* ─────────────────────────────────────────────────────────
 *  Meteo: fetch via libcurl in thread separato
 * ───────────────────────────────────────────────────────── */
struct MemBuf { char *data; size_t len; };

static size_t WriteCB(void *ptr, size_t size, size_t nmemb, void *ud)
{
    struct MemBuf *mb = (struct MemBuf*)ud;
    size_t total = size * nmemb;
    char *nd = (char*)realloc(mb->data, mb->len + total + 1);
    if (!nd) return 0;
    mb->data = nd;
    memcpy(mb->data + mb->len, ptr, total);
    mb->len += total;
    mb->data[mb->len] = '\0';
    return total;
}

static int FetchWeatherThread(void *arg)
{
    (void)arg;
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    struct MemBuf mb; mb.data = (char*)malloc(1); mb.data[0]='\0'; mb.len = 0;

    char url[256];
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
        "&current_weather=true&timezone=auto", CITY_LAT, CITY_LON);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCB);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &mb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DigitalClock/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        char *p = strstr(mb.data, "\"current_weather\"");
        if (p) {
            char *q = strstr(p, "\"temperature\":");
            char *w = strstr(p, "\"weathercode\":");
            SDL_LockMutex(g_weatherMutex);
            if (q) app.temp  = (float)atof(q + strlen("\"temperature\":"));
            if (w) app.wcode = atoi(w + strlen("\"weathercode\":"));
            SDL_UnlockMutex(g_weatherMutex);
        }
    }
    free(mb.data);
    curl_easy_cleanup(curl);
    return 0;
}

static const char* WeatherLabel(int code)
{
    if (code < 0)   return "";
    if (code == 0)  return "SUN";
    if (code <= 3)  return "CLOUD";
    if (code <= 49) return "FOG";
    if (code <= 67) return "RAIN";
    if (code <= 79) return "SNOW";
    if (code <= 84) return "RAIN";
    if (code <= 86) return "SNOW";
    return "STORM";
}

/* ─────────────────────────────────────────────────────────
 *  Fullscreen toggle
 * ───────────────────────────────────────────────────────── */
static void ToggleFullscreen(void)
{
    app.fullscreen = !app.fullscreen;
    SDL_SetWindowFullscreen(app.win, app.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

/* ─────────────────────────────────────────────────────────
 *  Overlay: aiuto comandi
 * ───────────────────────────────────────────────────────── */
static void DrawHelpOverlay(SDL_Renderer *r, int W, int H)
{
    int bw = 440, bh = 240;
    if (bw > W-20) bw = W-20;
    int bx = (W-bw)/2, by = (H-bh)/2;

    SDL_SetRenderDrawColor(r, 0,0,0,255);
    SDL_Rect box = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 80,80,80,255);
    SDL_RenderDrawRect(r, &box);

    DrawText(r, "COMANDI", bx+16, by+14, 3, COL_ON);

    struct { const char *k; const char *d; } rows[] = {
        {"F11",      "SCHERMO INTERO"},
        {"2 CLICK",  "SCHERMO INTERO"},
        {"T",        "FORMATO 12/24H"},
        {"A",        "IMPOSTA SVEGLIA"},
        {"H",        "MOSTRA/CHIUDI AIUTO"},
        {"ESC",      "FERMA SVEGLIA / ESCI"},
    };
    int n = (int)(sizeof(rows)/sizeof(rows[0]));
    for (int i = 0; i < n; i++) {
        int y = by + 52 + i*26;
        DrawText(r, rows[i].k, bx+16,  y, 2, COL_ON);
        DrawText(r, rows[i].d, bx+150, y, 2, COL_TEXT);
    }
    DrawText(r, "H O ESC PER CHIUDERE", bx+16, by+bh-24, 2, COL_DIM);
}

/* ─────────────────────────────────────────────────────────
 *  Overlay: imposta sveglia
 * ───────────────────────────────────────────────────────── */
static void DrawAlarmOverlay(SDL_Renderer *r, int W, int H)
{
    int bw = 380, bh = 220;
    if (bw > W-20) bw = W-20;
    int bx = (W-bw)/2, by = (H-bh)/2;

    SDL_SetRenderDrawColor(r, 0,0,0,255);
    SDL_Rect box = { bx, by, bw, bh };
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 80,80,80,255);
    SDL_RenderDrawRect(r, &box);

    DrawText(r, "IMPOSTA SVEGLIA", bx+16, by+14, 3, COL_ON);

    char hbuf[3], mbuf[3];
    snprintf(hbuf, 3, "%02d", app.edit_hour);
    snprintf(mbuf, 3, "%02d", app.edit_min);
    SDL_Color hcol = (app.edit_field==0) ? COL_ON : COL_TEXT;
    SDL_Color mcol = (app.edit_field==1) ? COL_ON : COL_TEXT;

    int bs = 5;
    int tx = bx + 60;
    int ty = by + 55;
    DrawText(r, hbuf, tx,               ty, bs, hcol);
    DrawText(r, ":",  tx + 2*6*bs,      ty, bs, COL_TEXT);
    DrawText(r, mbuf, tx + 3*6*bs,      ty, bs, mcol);

    DrawText(r, app.edit_enabled ? "SVEGLIA: ATTIVA" : "SVEGLIA: SPENTA",
              bx+16, by+120, 2, app.edit_enabled ? COL_ON : COL_DIM);

    DrawText(r, "TAB CAMBIA CAMPO", bx+16, by+152, 2, COL_DIM);
    DrawText(r, "SU/GIU REGOLA  SPAZIO ON/OFF", bx+16, by+172, 2, COL_DIM);
    DrawText(r, "INVIO CONFERMA  ESC ANNULLA", bx+16, by+192, 2, COL_DIM);
}

/* ─────────────────────────────────────────────────────────
 *  Controllo sveglia
 * ───────────────────────────────────────────────────────── */
static void CheckAlarm(void)
{
    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);

    if (app.alarm_set && !app.alarm_firing &&
        tmv->tm_hour == app.alarm_hour &&
        tmv->tm_min  == app.alarm_min  &&
        tmv->tm_sec  == 0) {
        app.alarm_firing = 1;
        app.alarm_fire_start = SDL_GetTicks();
    }

    if (app.alarm_firing) {
        static Uint32 lastBeep = 0;
        Uint32 now = SDL_GetTicks();
        if (now - lastBeep > 700) { PlayBeep(); lastBeep = now; }
        if (now - app.alarm_fire_start > 30000) app.alarm_firing = 0;
    }
}

/* ─────────────────────────────────────────────────────────
 *  Rendering principale
 * ───────────────────────────────────────────────────────── */
static void RenderClock(void)
{
    int W, H;
    SDL_GetWindowSize(app.win, &W, &H);

    SDL_SetRenderDrawColor(app.ren, COL_BG.r, COL_BG.g, COL_BG.b, 255);
    SDL_RenderClear(app.ren);

    time_t t = time(NULL);
    struct tm *tmv = localtime(&t);
    int hh = tmv->tm_hour, mm = tmv->tm_min, ss = tmv->tm_sec;
    int isPM = 0;
    if (!app.format24) { isPM = (hh >= 12); hh %= 12; if (hh == 0) hh = 12; }

    int digitH = (int)(H * 0.55);
    int digitW = (int)(digitH * 0.6);
    int T = digitH / 12; if (T < 4) T = 4;
    int gap = digitW / 5;
    int colonW = T * 3;
    int totalW = 4*digitW + colonW + 5*gap;
    int startX = (W - totalW) / 2;
    int startY = (H - digitH) / 2 + (int)(H * 0.04);

    DrawDigit(app.ren, hh/10, startX,               startY, digitW, digitH, T);
    DrawDigit(app.ren, hh%10, startX+digitW+gap,     startY, digitW, digitH, T);

    int colonX = startX + 2*digitW + 2*gap + colonW/2;
    DrawColon(app.ren, colonX, startY+digitH/2, T, (ss % 2 == 0));

    int mx = startX + 2*digitW + 2*gap + colonW + gap;
    DrawDigit(app.ren, mm/10, mx,           startY, digitW, digitH, T);
    DrawDigit(app.ren, mm%10, mx+digitW+gap,startY, digitW, digitH, T);

    /* secondi piccoli, ancorati in basso a destra dell'ultima cifra minuti */
    int sH = (int)(digitH * 0.28);
    int sW = (int)(sH * 0.6);
    int sT = sH/12; if (sT < 2) sT = 2;
    int sx = mx + digitW + gap + digitW - 2*sW - sT;
    int sy = startY + digitH - sH;
    DrawDigit(app.ren, ss/10, sx,        sy, sW, sH, sT);
    DrawDigit(app.ren, ss%10, sx+sW+sT,  sy, sW, sH, sT);

    if (!app.format24)
        DrawText(app.ren, isPM ? "PM" : "AM", sx, sy - (int)(H*0.035), 2, COL_TEXT);

    int fs = (int)(H * 0.022); if (fs < 2) fs = 2;

    char dateBuf[20];
    snprintf(dateBuf, 20, "%02d/%02d/%02d", tmv->tm_mday, tmv->tm_mon+1, tmv->tm_year % 100);
    int dw = TextWidth(dateBuf, fs);
    DrawText(app.ren, dateBuf, (W-dw)/2, (int)(H*0.08), fs, COL_TEXT);

    const char *giorno = GIORNI[tmv->tm_wday];
    int gw = TextWidth(giorno, fs);
    DrawText(app.ren, giorno, W - gw - 20, (int)(H*0.08), fs, COL_TEXT);

    SDL_LockMutex(g_weatherMutex);
    float temp = app.temp; int wcode = app.wcode;
    SDL_UnlockMutex(g_weatherMutex);

    char tempBuf[24];
    if (temp > -900.f)
        snprintf(tempBuf, 24, "%s %d^C", WeatherLabel(wcode), (int)lroundf(temp));
    else
        snprintf(tempBuf, 24, "-- ^C");
    DrawText(app.ren, tempBuf, 20, (int)(H*0.08), fs, COL_TEXT);

    char alarmBuf[24];
    if (app.alarm_set)
        snprintf(alarmBuf, 24, "SVEGLIA %02d:%02d", app.alarm_hour, app.alarm_min);
    else
        snprintf(alarmBuf, 24, "SVEGLIA --:--");
    DrawText(app.ren, alarmBuf, 20, H - (int)(H*0.08), fs, app.alarm_set ? COL_ON : COL_DIM);

    const char *modeStr = app.format24 ? "24H" : "12H";
    int mw = TextWidth(modeStr, fs);
    DrawText(app.ren, modeStr, W - mw - 20, H - (int)(H*0.08), fs, COL_DIM);

    if (app.show_help)       DrawHelpOverlay(app.ren, W, H);
    if (app.show_alarm_edit) DrawAlarmOverlay(app.ren, W, H);

    SDL_RenderPresent(app.ren);
}

/* ─────────────────────────────────────────────────────────
 *  Input
 * ───────────────────────────────────────────────────────── */
static void HandleKey(SDL_Keycode key)
{
    if (app.show_alarm_edit) {
        switch (key) {
            case SDLK_TAB: app.edit_field = !app.edit_field; break;
            case SDLK_UP:
                if (app.edit_field == 0) app.edit_hour = (app.edit_hour + 1) % 24;
                else app.edit_min = (app.edit_min + 1) % 60;
                break;
            case SDLK_DOWN:
                if (app.edit_field == 0) app.edit_hour = (app.edit_hour + 23) % 24;
                else app.edit_min = (app.edit_min + 59) % 60;
                break;
            case SDLK_SPACE: app.edit_enabled = !app.edit_enabled; break;
            case SDLK_RETURN: case SDLK_KP_ENTER:
                app.alarm_hour = app.edit_hour;
                app.alarm_min  = app.edit_min;
                app.alarm_set  = app.edit_enabled;
                app.alarm_firing = 0;
                app.show_alarm_edit = 0;
                break;
            case SDLK_ESCAPE:
                app.show_alarm_edit = 0;
                break;
        }
        return;
    }

    if (app.show_help) {
        if (key == SDLK_h || key == SDLK_ESCAPE) app.show_help = 0;
        return;
    }

    switch (key) {
        case SDLK_F11: ToggleFullscreen(); break;
        case SDLK_h:   app.show_help = 1; break;
        case SDLK_t:   app.format24 = !app.format24; break;
        case SDLK_a:
            app.edit_hour = app.alarm_hour;
            app.edit_min  = app.alarm_min;
            app.edit_enabled = app.alarm_set;
            app.edit_field = 0;
            app.show_alarm_edit = 1;
            break;
        case SDLK_ESCAPE:
            if (app.alarm_firing)      app.alarm_firing = 0;
            else if (app.fullscreen)   ToggleFullscreen();
            else                       app.running = 0;
            break;
    }
}

/* ─────────────────────────────────────────────────────────
 *  main
 * ───────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    g_weatherMutex = SDL_CreateMutex();

    memset(&app, 0, sizeof(app));
    app.format24 = 1;
    app.temp = -999.f;
    app.wcode = -1;
    app.running = 1;
    app.alarm_hour = 7;

    app.win = SDL_CreateWindow("Orologio Digitale",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        900, 450, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
    if (!app.win) { fprintf(stderr, "CreateWindow: %s\n", SDL_GetError()); return 1; }

    app.ren = SDL_CreateRenderer(app.win, -1, SDL_RENDERER_ACCELERATED);
    if (!app.ren) app.ren = SDL_CreateRenderer(app.win, -1, 0);
    if (!app.ren) { fprintf(stderr, "CreateRenderer: %s\n", SDL_GetError()); return 1; }

    SDL_Surface *icon = CreateClockIconSurface();
    if (icon) { SDL_SetWindowIcon(app.win, icon); SDL_FreeSurface(icon); }

    SDL_DisableScreenSaver();
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#endif

    InitAudio();
    GenerateBeep();

    SDL_Thread *wt0 = SDL_CreateThread(FetchWeatherThread, "weather", NULL);
    if (wt0) SDL_DetachThread(wt0);

    Uint32 lastWeather = SDL_GetTicks();

    while (app.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) app.running = 0;
            else if (e.type == SDL_KEYDOWN) HandleKey(e.key.keysym.sym);
            else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.clicks == 2) ToggleFullscreen();
        }

        Uint32 now = SDL_GetTicks();
        if (now - lastWeather > 600000) {
            SDL_Thread *wt = SDL_CreateThread(FetchWeatherThread, "weather", NULL);
            if (wt) SDL_DetachThread(wt);
            lastWeather = now;
        }

        CheckAlarm();
        RenderClock();
        SDL_Delay(50);
    }

#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
    if (g_audioDev) SDL_CloseAudioDevice(g_audioDev);
    free(g_beepBuf);
    SDL_DestroyMutex(g_weatherMutex);
    SDL_DestroyRenderer(app.ren);
    SDL_DestroyWindow(app.win);
    SDL_Quit();
    curl_global_cleanup();
    return 0;
}