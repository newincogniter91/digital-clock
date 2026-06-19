/*
 * DigitalClock - Orologio stile sveglia digitale anni '80
 * Windows nativo (Win32 + GDI + WinHTTP)
 *
 * Compilazione MinGW (una riga):
 *   gcc clock.c -o clock.exe -mwindows -lwinhttp -lws2_32 -municode
 *
 * Oppure con GCC senza -mwindows per vedere la console di debug:
 *   gcc clock.c -o clock.exe -lwinhttp -lws2_32 -municode
 */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winhttp.h>
#include <wchar.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#pragma comment(lib, "winhttp.lib")

/* ── Palette ───────────────────────────────────────────── */
#define COL_BG        RGB(0,   0,   0)    /* nero assoluto            */
#define COL_SEG_ON    RGB(220, 220, 220)  /* bianco-grigio segmento   */
#define COL_SEG_OFF   RGB(15,  15,  15)   /* segmento spento, appena visibile */
#define COL_TEXT      RGB(160, 160, 160)  /* testo secondario         */
#define COL_COLON     RGB(220, 220, 220)

/* ── Segmenti a 7 barre ────────────────────────────────── */
/*
 *   -a-
 *  f   b
 *   -g-
 *  e   c
 *   -d-
 *
 * Bit mask: a=0x01 b=0x02 c=0x04 d=0x08 e=0x10 f=0x20 g=0x40
 */
static const BYTE seg7[10] = {
    0x3F, /* 0: abcdef    */
    0x06, /* 1: bc        */
    0x5B, /* 2: abdeg     */
    0x4F, /* 3: abcdg     */
    0x66, /* 4: bcfg      */
    0x6D, /* 5: acdfg     */
    0x7D, /* 6: acdefg    */
    0x07, /* 7: abc       */
    0x7F, /* 8: abcdefg   */
    0x6F, /* 9: abcdfg    */
};

/* ── Stato globale ─────────────────────────────────────── */
static HWND  g_hwnd   = NULL;
static float g_temp   = -999.f;   /* gradi Celsius, -999 = non ancora caricati */
static BOOL  g_fullscreen = FALSE;
static WINDOWPLACEMENT g_wpPrev;

/* ── Nomi giorni settimana in italiano ─────────────────── */
static const wchar_t *GIORNI[] = {
    L"DOMENICA", L"LUNED\u00CC", L"MARTED\u00CC",
    L"MERCOLED\u00CC", L"GIOVED\u00CC", L"VENERD\u00CC", L"SABATO"
};

/* ─────────────────────────────────────────────────────────
 *  Disegna un singolo segmento (rettangolo piatto inclinato)
 *  dir: 0=orizzontale  1=verticale-sinistra  2=verticale-destra
 * ───────────────────────────────────────────────────────── */
static void DrawSegment(HDC hdc, int x, int y, int w, int t, int dir, BOOL on)
{
    HBRUSH br = CreateSolidBrush(on ? COL_SEG_ON : COL_SEG_OFF);
    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
    HPEN pen = CreatePen(PS_SOLID, 1, on ? COL_SEG_ON : COL_SEG_OFF);
    HPEN oldpen = (HPEN)SelectObject(hdc, pen);

    int sk = t / 2;  /* skew per effetto corsivo leggero */
    POINT pts[6];

    if (dir == 0) {
        /* orizzontale */
        pts[0].x = x + sk;       pts[0].y = y;
        pts[1].x = x + w - sk;   pts[1].y = y;
        pts[2].x = x + w - sk*2; pts[2].y = y + t;
        pts[3].x = x + sk*2;     pts[3].y = y + t;
        Polygon(hdc, pts, 4);
    } else {
        /* verticale */
        pts[0].x = x;        pts[0].y = y + sk;
        pts[1].x = x + t;    pts[1].y = y;
        pts[2].x = x + t;    pts[2].y = y + w - sk;
        pts[3].x = x;        pts[3].y = y + w;
        Polygon(hdc, pts, 4);
    }

    SelectObject(hdc, old);
    SelectObject(hdc, oldpen);
    DeleteObject(br);
    DeleteObject(pen);
}

/* ─────────────────────────────────────────────────────────
 *  Disegna una cifra 7-segmenti
 *  x,y = angolo top-left   W=larghezza  H=altezza  T=spessore segmento
 * ───────────────────────────────────────────────────────── */
static void DrawDigit(HDC hdc, int digit, int x, int y, int W, int H, int T)
{
    if (digit < 0 || digit > 9) return;
    BYTE mask = seg7[digit];
    int gap = T / 3;
    int segW = W - 2*T - 2*gap;
    int halfH = (H - 3*T - 2*gap) / 2;

    /* a - top */
    DrawSegment(hdc, x + T + gap, y,
                segW, T, 0, (mask >> 0) & 1);
    /* b - top-right */
    DrawSegment(hdc, x + W - T, y + T + gap,
                halfH, T, 1, (mask >> 1) & 1);
    /* c - bot-right */
    DrawSegment(hdc, x + W - T, y + T + gap + halfH + T + gap,
                halfH, T, 1, (mask >> 2) & 1);
    /* d - bottom */
    DrawSegment(hdc, x + T + gap, y + H - T,
                segW, T, 0, (mask >> 3) & 1);
    /* e - bot-left */
    DrawSegment(hdc, x, y + T + gap + halfH + T + gap,
                halfH, T, 1, (mask >> 4) & 1);
    /* f - top-left */
    DrawSegment(hdc, x, y + T + gap,
                halfH, T, 1, (mask >> 5) & 1);
    /* g - middle */
    DrawSegment(hdc, x + T + gap, y + T + gap + halfH + gap,
                segW, T, 0, (mask >> 6) & 1);
}

/* Disegna due puntini : */
static void DrawColon(HDC hdc, int x, int cy, int r)
{
    HBRUSH br = CreateSolidBrush(COL_COLON);
    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
    Ellipse(hdc, x-r, cy - r*4 - r, x+r, cy - r*4 + r);
    Ellipse(hdc, x-r, cy + r*3,     x+r, cy + r*3 + r*2);
    SelectObject(hdc, old);
    DeleteObject(br);
}

/* ─────────────────────────────────────────────────────────
 *  Fetch temperatura Torino via Open-Meteo (thread separato)
 *  lat=45.07  lon=7.69
 * ───────────────────────────────────────────────────────── */
static DWORD WINAPI FetchWeatherThread(LPVOID param)
{
    HINTERNET hSession = WinHttpOpen(
        L"DigitalClock/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return 0;

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"api.open-meteo.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return 0; }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        L"/v1/forecast?latitude=45.0703&longitude=7.6869"
        L"&current=temperature_2m&forecast_days=1",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) goto cleanup;

    if (!WinHttpSendRequest(hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) goto cleanup;

    if (!WinHttpReceiveResponse(hRequest, NULL)) goto cleanup;

    char buf[2048] = {0};
    DWORD read = 0, total = 0;
    while (WinHttpReadData(hRequest, buf + total,
           sizeof(buf) - 1 - total, &read) && read > 0)
        total += read;
    buf[total] = '\0';

    /* Parse: cerca "temperature_2m":<valore> */
    char *p = strstr(buf, "\"temperature_2m\":");
    if (p) {
        p += strlen("\"temperature_2m\":");
        g_temp = (float)atof(p);
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    }

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return 0;
}

/* ─────────────────────────────────────────────────────────
 *  Toggle fullscreen
 * ───────────────────────────────────────────────────────── */
static void ToggleFullscreen(HWND hwnd)
{
    if (!g_fullscreen) {
        g_wpPrev.length = sizeof(g_wpPrev);
        GetWindowPlacement(hwnd, &g_wpPrev);
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
        SetWindowLong(hwnd, GWL_STYLE,
            GetWindowLong(hwnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right  - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen = TRUE;
    } else {
        SetWindowLong(hwnd, GWL_STYLE,
            GetWindowLong(hwnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd, &g_wpPrev);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_fullscreen = FALSE;
    }
}

/* ─────────────────────────────────────────────────────────
 *  WM_PAINT  –  disegna tutto su buffer off-screen
 * ───────────────────────────────────────────────────────── */
static void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right;
    int H = rc.bottom;

    /* Off-screen buffer */
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, W, H);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

    /* Sfondo nero */
    HBRUSH bgBr = CreateSolidBrush(COL_BG);
    FillRect(mem, &rc, bgBr);
    DeleteObject(bgBr);

    /* ── Layout adattivo ── */
    int digitH = (int)(H * 0.55);
    int digitW = (int)(digitH * 0.6);
    int T      = max(4, digitH / 12);

    /* Larghezza totale: 4 cifre + 1 colon + spazi */
    int gap    = digitW / 5;
    int colonW = T * 3;
    int totalW = 4 * digitW + colonW + 5 * gap;
    int startX = (W - totalW) / 2;
    int startY = (H - digitH) / 2 + (int)(H * 0.04);

    /* Ora corrente */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int hh = tm->tm_hour;
    int mm = tm->tm_min;
    int ss = tm->tm_sec;

    /* ── Cifre ORA ── */
    DrawDigit(mem, hh / 10, startX,           startY, digitW, digitH, T);
    DrawDigit(mem, hh % 10, startX + digitW + gap, startY, digitW, digitH, T);

    /* ── Due punti ── */
    int colonX = startX + 2*digitW + 2*gap + colonW/2;
    DrawColon(mem, colonX, startY + digitH/2, T);

    /* ── Cifre MINUTI ── */
    int mx = startX + 2*digitW + 2*gap + colonW + gap;
    DrawDigit(mem, mm / 10, mx,           startY, digitW, digitH, T);
    DrawDigit(mem, mm % 10, mx + digitW + gap, startY, digitW, digitH, T);

    /* ── SECONDI (piccoli, in basso a destra dei minuti) ── */
    int sH = (int)(digitH * 0.28);
    int sW = (int)(sH * 0.6);
    int sT = max(2, sH / 12);
    int sx = mx + 2*digitW + 2*gap + gap/2;
    int sy = startY + digitH - sH;
    DrawDigit(mem, ss / 10, sx,      sy, sW, sH, sT);
    DrawDigit(mem, ss % 10, sx + sW + sT, sy, sW, sH, sT);

    /* ── Testo superiore ── */
    int fontH = max(12, (int)(H * 0.038));
    HFONT hfont = CreateFont(fontH, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
    HFONT oldfont = (HFONT)SelectObject(mem, hfont);
    SetBkMode(mem, TRANSPARENT);
    SetTextColor(mem, COL_TEXT);

    /* Data: GG/MM/AA centrata */
    wchar_t dateBuf[32];
    swprintf(dateBuf, 32, L"%02d/%02d/%02d",
        tm->tm_mday, tm->tm_mon + 1, tm->tm_year % 100);
    RECT dateRc = { W/4, (int)(H*0.10), W*3/4, (int)(H*0.20) };
    DrawTextW(mem, dateBuf, -1, &dateRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    /* Giorno settimana: destra */
    const wchar_t *giorno = GIORNI[tm->tm_wday];
    RECT dayRc = { W/2, (int)(H*0.10), W - fontH, (int)(H*0.20) };
    DrawTextW(mem, giorno, -1, &dayRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    /* Temperatura: alto sinistra */
    wchar_t tempBuf[32];
    if (g_temp > -900.f)
        swprintf(tempBuf, 32, L"%.0f\u00B0C", g_temp);
    else
        wcscpy(tempBuf, L"--°C");
    RECT tempRc = { fontH, (int)(H*0.10), W/3, (int)(H*0.20) };
    DrawTextW(mem, tempBuf, -1, &tempRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* ── Istruzioni in basso ── */
    int hintH = max(9, (int)(H * 0.025));
    HFONT hfontHint = CreateFont(hintH, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FIXED_PITCH | FF_MODERN, L"Courier New");
    SelectObject(mem, hfontHint);
    SetTextColor(mem, RGB(60,60,60));
    RECT hintRc = { 0, H - hintH*2, W, H };
    DrawTextW(mem, L"F11 Schermo intero  |  ESC Esci",
              -1, &hintRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(mem, oldfont);
    DeleteObject(hfont);
    DeleteObject(hfontHint);

    /* Blit */
    BitBlt(hdc, 0, 0, W, H, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);

    EndPaint(hwnd, &ps);
}

/* ─────────────────────────────────────────────────────────
 *  WndProc
 * ───────────────────────────────────────────────────────── */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        /* Impedisce standby e spegnimento schermo finché l'app è aperta */
        SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
        /* Timer 1: ridisegna ogni secondo */
        SetTimer(hwnd, 1, 1000, NULL);
        /* Timer 2: aggiorna meteo ogni 10 minuti */
        SetTimer(hwnd, 2, 600000, NULL);
        /* Fetch immediato al lancio */
        CloseHandle(CreateThread(NULL, 0, FetchWeatherThread, NULL, 0, NULL));
        return 0;

    case WM_TIMER:
        if (wp == 1) InvalidateRect(hwnd, NULL, FALSE);
        if (wp == 2) CloseHandle(CreateThread(NULL, 0, FetchWeatherThread, NULL, 0, NULL));
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_F11) ToggleFullscreen(hwnd);
        if (wp == VK_ESCAPE) {
            if (g_fullscreen) ToggleFullscreen(hwnd);
            else PostQuitMessage(0);
        }
        return 0;

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;   /* evita flicker */

    case WM_DESTROY:
        SetThreadExecutionState(ES_CONTINUOUS); /* ripristina standby normale */
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ─────────────────────────────────────────────────────────
 *  WinMain
 * ───────────────────────────────────────────────────────── */
int WINAPI WinMainW(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.hInstance     = hInst;
    wc.lpfnWndProc   = WndProc;
    wc.lpszClassName = L"DigitalClockClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        L"DigitalClockClass",
        L"Orologio Digitale",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        900, 450,
        NULL, NULL, hInst, NULL);

    g_hwnd = hwnd;
    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

/* Entry point unicode per MinGW */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    return WinMainW(hInst, hPrev, lpCmd, nShow);
}
