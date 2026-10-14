/*
 * DigitalClock - Orologio stile sveglia digitale anni '80
 * Windows nativo (Win32 + GDI + WinHTTP)
 *
 * Compilazione MinGW (una riga):
 *   gcc -mwindows -municode clock.c -o clock.exe -lwinhttp -lws2_32
 *
 * TASTI:
 *   F11 / doppio click  → schermo intero / finestra
 *   H                   → mostra popup comandi
 *   T                   → toggle 12h / 24h
 *   A                   → imposta sveglia
 *   ESC                 → ferma sveglia / esci fullscreen / esci
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

/* ── Palette ───────────────────────────────────────────── */
#define COL_BG        RGB(0,   0,   0)
#define COL_SEG_ON    RGB(220, 220, 220)
#define COL_SEG_OFF   RGB(15,  15,  15)
#define COL_TEXT      RGB(160, 160, 160)
#define COL_COLON     RGB(220, 220, 220)
#define COL_ALARM_ON  RGB(220, 220, 220)  /* icona sveglia attiva  */
#define COL_ALARM_OFF RGB(50,  50,  50)   /* icona sveglia inattiva */

/* ── Segmenti a 7 barre ────────────────────────────────── */
static const BYTE seg7[10] = {
    0x3F,0x06,0x5B,0x4F,0x66,
    0x6D,0x7D,0x07,0x7F,0x6F
};

/* ── Stato globale ─────────────────────────────────────── */
static HWND  g_hwnd        = NULL;
static float g_temp        = -999.f;
static int   g_wcode       = -1;       /* weathercode Open-Meteo   */
static BOOL  g_fullscreen  = FALSE;
static BOOL  g_format24    = TRUE;     /* TRUE=24h  FALSE=12h      */
static WINDOWPLACEMENT g_wpPrev;

/* ── Sveglia ───────────────────────────────────────────── */
static BOOL g_alarm_set    = FALSE;
static int  g_alarm_hour   = 7;
static int  g_alarm_min    = 0;
static BOOL g_alarm_firing = FALSE;   /* sta suonando?            */
static int  g_alarm_ticks  = 0;       /* contatore beep           */
static int  g_alarm_last_min = -1;    /* minuto (hh*60+mm) già suonato */

/* ── Nomi giorni settimana ─────────────────────────────── */
static const wchar_t *GIORNI[] = {
    L"DOMENICA", L"LUNED\u00CC", L"MARTED\u00CC",
    L"MERCOLED\u00CC", L"GIOVED\u00CC", L"VENERD\u00CC", L"SABATO"
};

/* ── Icone meteo (Unicode) ─────────────────────────────── */
static const wchar_t* WeatherIcon(int code)
{
    if (code < 0)   return L"";
    if (code == 0)  return L"\u2600";  /* ☀ sereno           */
    if (code <= 2)  return L"\u26C5";  /* ⛅ parzialmente     */
    if (code == 3)  return L"\u2601";  /* ☁ nuvoloso          */
    if (code <= 49) return L"\u2601";  /* nebbia/foschia      */
    if (code <= 59) return L"\u2602";  /* ☂ pioggerella       */
    if (code <= 69) return L"\u2602";  /* pioggia             */
    if (code <= 79) return L"\u2744";  /* ❄ neve              */
    if (code <= 84) return L"\u2602";  /* rovesci             */
    if (code <= 94) return L"\u26A1";  /* ⚡ temporale        */
    return L"\u26A1";
}

/* ─────────────────────────────────────────────────────────
 *  DrawSegment
 * ───────────────────────────────────────────────────────── */
static void DrawSegment(HDC hdc, int x, int y, int w, int t, int dir, BOOL on)
{
    HBRUSH br  = CreateSolidBrush(on ? COL_SEG_ON : COL_SEG_OFF);
    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
    HPEN pen   = CreatePen(PS_SOLID, 1, on ? COL_SEG_ON : COL_SEG_OFF);
    HPEN oldp  = (HPEN)SelectObject(hdc, pen);
    int sk = t / 2;
    POINT pts[4];
    if (dir == 0) {
        pts[0].x=x+sk;     pts[0].y=y;
        pts[1].x=x+w-sk;   pts[1].y=y;
        pts[2].x=x+w-sk*2; pts[2].y=y+t;
        pts[3].x=x+sk*2;   pts[3].y=y+t;
    } else {
        pts[0].x=x;    pts[0].y=y+sk;
        pts[1].x=x+t;  pts[1].y=y;
        pts[2].x=x+t;  pts[2].y=y+w-sk;
        pts[3].x=x;    pts[3].y=y+w;
    }
    Polygon(hdc, pts, 4);
    SelectObject(hdc, old);  DeleteObject(br);
    SelectObject(hdc, oldp); DeleteObject(pen);
}

/* ─────────────────────────────────────────────────────────
 *  DrawDigit
 * ───────────────────────────────────────────────────────── */
static void DrawDigit(HDC hdc, int digit, int x, int y, int W, int H, int T)
{
    if (digit < 0 || digit > 9) return;
    BYTE mask = seg7[digit];
    int gap  = T / 3;
    int segW = W - 2*T - 2*gap;
    int half = (H - 3*T - 2*gap) / 2;

    DrawSegment(hdc, x+T+gap, y,                          segW, T, 0, (mask>>0)&1);
    DrawSegment(hdc, x+W-T,   y+T+gap,                    half, T, 1, (mask>>1)&1);
    DrawSegment(hdc, x+W-T,   y+T+gap+half+T+gap,         half, T, 1, (mask>>2)&1);
    DrawSegment(hdc, x+T+gap, y+H-T,                      segW, T, 0, (mask>>3)&1);
    DrawSegment(hdc, x,       y+T+gap+half+T+gap,         half, T, 1, (mask>>4)&1);
    DrawSegment(hdc, x,       y+T+gap,                    half, T, 1, (mask>>5)&1);
    DrawSegment(hdc, x+T+gap, y+T+gap+half+gap,           segW, T, 0, (mask>>6)&1);
}

/* ─────────────────────────────────────────────────────────
 *  DrawColon  (lampeggia ogni secondo)
 * ───────────────────────────────────────────────────────── */
static void DrawColon(HDC hdc, int x, int cy, int r, BOOL visible)
{
    COLORREF c = visible ? COL_COLON : COL_BG;
    HBRUSH br  = CreateSolidBrush(c);
    HBRUSH old = (HBRUSH)SelectObject(hdc, br);
    HPEN pen   = CreatePen(PS_SOLID,1,c);
    HPEN oldp  = (HPEN)SelectObject(hdc, pen);
    Ellipse(hdc, x-r, cy-r*4-r, x+r, cy-r*4+r);
    Ellipse(hdc, x-r, cy+r*3,   x+r, cy+r*3+r*2);
    SelectObject(hdc, old);  DeleteObject(br);
    SelectObject(hdc, oldp); DeleteObject(pen);
}

/* ─────────────────────────────────────────────────────────
 *  Fetch meteo (thread)
 * ───────────────────────────────────────────────────────── */
static DWORD WINAPI FetchWeatherThread(LPVOID param)
{
    (void)param;
    HINTERNET hSes = WinHttpOpen(L"DigitalClock/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSes) return 0;

    HINTERNET hCon = WinHttpConnect(hSes, L"api.open-meteo.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hCon) { WinHttpCloseHandle(hSes); return 0; }

    HINTERNET hReq = WinHttpOpenRequest(hCon, L"GET",
        L"/v1/forecast?latitude=45.0703&longitude=7.6869"
        L"&current_weather=true&timezone=auto",
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq) goto cleanup;

    if (!WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS,
        0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) goto cleanup;
    if (!WinHttpReceiveResponse(hReq, NULL)) goto cleanup;

    char buf[2048] = {0};
    DWORD rd = 0, tot = 0;
    while (WinHttpReadData(hReq, buf+tot, sizeof(buf)-1-tot, &rd) && rd > 0)
        tot += rd;
    buf[tot] = '\0';

    /* parse temperature + weathercode da current_weather */
    char *p = strstr(buf, "\"current_weather\"");
    if (p) {
        char *q = strstr(p, "\"temperature\":");
        if (q) {
            q += strlen("\"temperature\":");
            g_temp = (float)atof(q);
        }
        char *w = strstr(p, "\"weathercode\":");
        if (w) {
            w += strlen("\"weathercode\":");
            g_wcode = atoi(w);
        }
        if (g_hwnd) InvalidateRect(g_hwnd, NULL, FALSE);
    }

cleanup:
    if (hReq) WinHttpCloseHandle(hReq);
    WinHttpCloseHandle(hCon);
    WinHttpCloseHandle(hSes);
    return 0;
}

/* Stato del dialog sveglia */
static HWND g_dlgEH = NULL, g_dlgEM = NULL, g_dlgChk = NULL;
static BOOL g_dlgRunning = FALSE;

/* WndProc del dialog: i WM_COMMAND dei bottoni arrivano qui (vengono
 * inviati con SendMessage, quindi NON passano mai dal GetMessage loop) */
static LRESULT CALLBACK AlarmDlgProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) {
            wchar_t b[8];
            GetWindowTextW(g_dlgEH,b,8); g_alarm_hour=_wtoi(b);
            GetWindowTextW(g_dlgEM,b,8); g_alarm_min =_wtoi(b);
            g_alarm_set=(SendMessage(g_dlgChk,BM_GETCHECK,0,0)==BST_CHECKED);
            if (g_alarm_hour < 0)  g_alarm_hour = 0;
            if (g_alarm_hour > 23) g_alarm_hour = 23;
            if (g_alarm_min  < 0)  g_alarm_min  = 0;
            if (g_alarm_min  > 59) g_alarm_min  = 59;
            g_alarm_firing = FALSE;
            g_alarm_ticks  = 0;
            {   /* se imposto l'ora corrente, non suonare subito */
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                g_alarm_last_min = tm->tm_hour*60 + tm->tm_min;
            }
            g_dlgRunning = FALSE;
            return 0;
        }
        if (LOWORD(w) == IDCANCEL) { g_dlgRunning = FALSE; return 0; }
        break;
    case WM_CLOSE:
        g_dlgRunning = FALSE;
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

/* Crea il dialog sveglia a runtime (no .rc file) */
static void ShowAlarmDialog(HWND parent)
{
    static BOOL classReg = FALSE;
    HINSTANCE hInstDlg = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    if (!classReg) {
        WNDCLASSEXW dc = { sizeof(dc) };
        dc.hInstance     = hInstDlg;
        dc.lpfnWndProc   = AlarmDlgProc;
        dc.lpszClassName = L"DigitalClockAlarmDlg";
        dc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        dc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&dc);
        classReg = TRUE;
    }

    /* Dimensione dell'area client 230x150: la finestra esterna include titolo e bordi */
    RECT rc = { 0, 0, 230, 150 };
    AdjustWindowRectEx(&rc, WS_POPUP | WS_CAPTION | WS_SYSMENU, FALSE,
                       WS_EX_DLGMODALFRAME | WS_EX_TOPMOST);

    /* Dialog manuale con CreateWindowEx */
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"DigitalClockAlarmDlg", L"Sveglia",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_CENTER,
        0,0, rc.right - rc.left, rc.bottom - rc.top, parent, NULL,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), NULL);
    if (!hDlg) return;

    /* Centra rispetto al parent */
    RECT rp, rd;
    GetWindowRect(parent, &rp);
    GetWindowRect(hDlg,   &rd);
    int dw = rd.right-rd.left, dh = rd.bottom-rd.top;
    SetWindowPos(hDlg, NULL,
        rp.left+(rp.right-rp.left-dw)/2,
        rp.top +(rp.bottom-rp.top-dh)/2,
        dw, dh, SWP_NOZORDER);

    HFONT hf = CreateFont(16,0,0,0,FW_NORMAL,0,0,0,
        DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");

    HWND hLbl = CreateWindowW(L"STATIC", L"Ora (HH):", WS_CHILD|WS_VISIBLE,
        20,20,80,20, hDlg,(HMENU)101,NULL,NULL);
    HWND hEH  = CreateWindowW(L"EDIT",   L"", WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_CENTER,
        100,18,50,22, hDlg,(HMENU)102,NULL,NULL);
    HWND hLm  = CreateWindowW(L"STATIC", L"Min (MM):", WS_CHILD|WS_VISIBLE,
        20,50,80,20, hDlg,(HMENU)103,NULL,NULL);
    HWND hEM  = CreateWindowW(L"EDIT",   L"", WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_CENTER,
        100,48,50,22, hDlg,(HMENU)104,NULL,NULL);
    HWND hChk = CreateWindowW(L"BUTTON", L"Sveglia attiva",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,
        20,80,160,22, hDlg,(HMENU)105,NULL,NULL);
    HWND hOK  = CreateWindowW(L"BUTTON", L"OK",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
        50,110,70,26, hDlg,(HMENU)IDOK,NULL,NULL);
    HWND hCan = CreateWindowW(L"BUTTON", L"Annulla",
        WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
        130,110,70,26, hDlg,(HMENU)IDCANCEL,NULL,NULL);

    /* Applica font a tutti */
    HWND kids[7];
    kids[0]=hLbl; kids[1]=hEH; kids[2]=hLm; kids[3]=hEM;
    kids[4]=hChk; kids[5]=hOK; kids[6]=hCan;
    for (int i=0;i<7;i++) SendMessage(kids[i],WM_SETFONT,(WPARAM)hf,TRUE);

    /* Valori iniziali */
    wchar_t tmp[8];
    swprintf(tmp,8,L"%02d",g_alarm_hour); SetWindowTextW(hEH,tmp);
    swprintf(tmp,8,L"%02d",g_alarm_min);  SetWindowTextW(hEM,tmp);
    SendMessage(hChk,BM_SETCHECK,BST_CHECKED,0);   /* di default la sveglia viene attivata */
    g_dlgEH = hEH; g_dlgEM = hEM; g_dlgChk = hChk;

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    /* Mini message loop modale */
    MSG msg;
    g_dlgRunning = TRUE;
    EnableWindow(parent, FALSE);
    SetFocus(hEH);
    while (g_dlgRunning) {
        BOOL r = GetMessageW(&msg, NULL, 0, 0);
        if (r <= 0) {                       /* WM_QUIT: rilancialo al loop principale */
            if (r == 0) PostQuitMessage((int)msg.wParam);
            break;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            g_dlgRunning = FALSE;
            break;
        }
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!IsWindow(hDlg)) break;
    }
    EnableWindow(parent, TRUE);
    DestroyWindow(hDlg);
    DeleteObject(hf);
    SetForegroundWindow(parent);
    SetFocus(parent);
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
        GetMonitorInfo(MonitorFromWindow(hwnd,MONITOR_DEFAULTTOPRIMARY),&mi);
        SetWindowLong(hwnd,GWL_STYLE,
            GetWindowLong(hwnd,GWL_STYLE)&~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd,HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right-mi.rcMonitor.left,
            mi.rcMonitor.bottom-mi.rcMonitor.top,
            SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
        g_fullscreen=TRUE;
    } else {
        SetWindowLong(hwnd,GWL_STYLE,
            GetWindowLong(hwnd,GWL_STYLE)|WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hwnd,&g_wpPrev);
        SetWindowPos(hwnd,NULL,0,0,0,0,
            SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|
            SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
        g_fullscreen=FALSE;
    }
}

/* ─────────────────────────────────────────────────────────
 *  OnPaint
 * ───────────────────────────────────────────────────────── */
static void OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc;
    GetClientRect(hwnd, &rc);
    int W=rc.right, H=rc.bottom;

    HDC mem    = CreateCompatibleDC(hdc);
    HBITMAP bmp= CreateCompatibleBitmap(hdc,W,H);
    HBITMAP old= (HBITMAP)SelectObject(mem,bmp);

    HBRUSH bgBr = CreateSolidBrush(COL_BG);
    FillRect(mem,&rc,bgBr);
    DeleteObject(bgBr);

    /* Layout */
    int digitH = (int)(H*0.55);
    int digitW = (int)(digitH*0.6);
    int T      = max(4, digitH/12);
    int gap    = digitW/5;
    int colonW = T*3;

    /* Stima dimensione secondi (solo per riferimento, non influenza il centramento) */
    int sH_est = (int)(digitH*0.28);
    int sW_est = (int)(sH_est*0.6);
    int sT_est = max(2, sH_est/12);
    (void)sW_est; (void)sT_est; (void)sH_est;

    int totalW = 4*digitW + colonW + 5*gap;
    int startX = (W-totalW)/2;
    int startY = (H-digitH)/2 + (int)(H*0.04);

    /* Ora corrente */
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int hh = tm->tm_hour;
    int mm = tm->tm_min;
    int ss = tm->tm_sec;

    /* Blink: i due punti sono visibili nei secondi pari */
    BOOL colonVisible = (ss % 2 == 0);

    /* Formato 12h */
    BOOL isPM = FALSE;
    if (!g_format24) {
        isPM = (hh >= 12);
        hh = hh % 12;
        if (hh == 0) hh = 12;
    }

    /* Cifre ora */
    DrawDigit(mem, hh/10, startX,               startY, digitW, digitH, T);
    DrawDigit(mem, hh%10, startX+digitW+gap,     startY, digitW, digitH, T);

    /* Colon lampeggiante */
    int colonX = startX+2*digitW+2*gap+colonW/2;
    DrawColon(mem, colonX, startY+digitH/2, T, colonVisible);

    /* Cifre minuti */
    int mx = startX+2*digitW+2*gap+colonW+gap;
    DrawDigit(mem, mm/10, mx,           startY, digitW, digitH, T);
    DrawDigit(mem, mm%10, mx+digitW+gap,startY, digitW, digitH, T);

    /* Secondi piccoli — ancorati in basso a destra delle cifre minuti */
    int sH = (int)(digitH*0.28);
    int sW = (int)(sH*0.6);
    int sT = max(2, sH/12);
    int sx = mx + 2*digitW + 2*gap;   /* subito a destra dell'ultima cifra minuti */
    int sy = startY + digitH - sH;
    DrawDigit(mem, ss/10, sx,       sy, sW, sH, sT);
    DrawDigit(mem, ss%10, sx+sW+sT, sy, sW, sH, sT);

    /* AM/PM in 12h mode */
    if (!g_format24) {
        int fontAP = max(10,(int)(H*0.03));
        HFONT hfAP = CreateFont(fontAP,0,0,0,FW_BOLD,0,0,0,
            DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Courier New");
        HFONT oldAP=(HFONT)SelectObject(mem,hfAP);
        SetBkMode(mem,TRANSPARENT);
        SetTextColor(mem,COL_TEXT);
        RECT rAP={sx, sy-fontAP-2, sx+sW*3, sy};
        DrawTextW(mem, isPM?L"PM":L"AM",-1,&rAP,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        SelectObject(mem,oldAP); DeleteObject(hfAP);
    }

    /* ── Testo superiore ── */
    int fontH2 = max(12,(int)(H*0.038));
    HFONT hfont = CreateFont(fontH2,0,0,0,FW_BOLD,0,0,0,
        DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,
        FIXED_PITCH|FF_MODERN,L"Courier New");
    HFONT oldfont=(HFONT)SelectObject(mem,hfont);
    SetBkMode(mem,TRANSPARENT);
    SetTextColor(mem,COL_TEXT);

    /* Data centrata */
    wchar_t dateBuf[32];
    swprintf(dateBuf,32,L"%02d/%02d/%02d",
        tm->tm_mday, tm->tm_mon+1, tm->tm_year%100);
    RECT dateRc={W/4,(int)(H*0.10),W*3/4,(int)(H*0.20)};
    DrawTextW(mem,dateBuf,-1,&dateRc,DT_CENTER|DT_VCENTER|DT_SINGLELINE);

    /* Giorno settimana destra */
    RECT dayRc={W/2,(int)(H*0.10),W-fontH2,(int)(H*0.20)};
    DrawTextW(mem,GIORNI[tm->tm_wday],-1,&dayRc,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);

    /* Temperatura + icona meteo sinistra */
    wchar_t tempBuf[32];
    if (g_temp > -900.f)
        swprintf(tempBuf,32,L"%ls %.0f\u00B0C", WeatherIcon(g_wcode), g_temp);
    else
        wcscpy(tempBuf, L"-- \u00B0C");

    /* Font Segoe UI per icone Unicode */
    HFONT hfSeg = CreateFont(fontH2,0,0,0,FW_NORMAL,0,0,0,
        DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI Symbol");
    SelectObject(mem,hfSeg);
    RECT tempRc={fontH2,(int)(H*0.10),W/3,(int)(H*0.20)};
    DrawTextW(mem,tempBuf,-1,&tempRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    SelectObject(mem,oldfont);
    DeleteObject(hfSeg);
    DeleteObject(hfont);

    /* ── Icona sveglia in basso a sinistra ── */
    {
        int iconSz = max(14,(int)(H*0.04));
        HFONT hfIco = CreateFont(iconSz,0,0,0,FW_NORMAL,0,0,0,
            DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI Symbol");
        HFONT oldIco=(HFONT)SelectObject(mem,hfIco);
        SetTextColor(mem, g_alarm_set ? COL_ALARM_ON : COL_ALARM_OFF);
        wchar_t alarmBuf[32];
        if (g_alarm_set)
            swprintf(alarmBuf,32,L"\u23F0 %02d:%02d",g_alarm_hour,g_alarm_min);
        else
            wcscpy(alarmBuf,L"\u23F0 --:--");
        RECT icoRc={fontH2, H-(int)(H*0.12), W/2, H-(int)(H*0.02)};
        DrawTextW(mem,alarmBuf,-1,&icoRc,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        SelectObject(mem,oldIco); DeleteObject(hfIco);
    }

    BitBlt(hdc,0,0,W,H,mem,0,0,SRCCOPY);
    SelectObject(mem,old);
    DeleteObject(bmp);
    DeleteDC(mem);
    EndPaint(hwnd,&ps);
}

/* ─────────────────────────────────────────────────────────
 *  Crea icona .ico 32x32 in memoria e la assegna alla finestra
 * ───────────────────────────────────────────────────────── */
static HICON CreateClockIcon(void)
{
    int sz = 32;
    HDC hdc  = GetDC(NULL);
    HDC mem  = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, sz, sz);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);
    ReleaseDC(NULL, hdc);

    RECT r = {0,0,sz,sz};
    HBRUSH bg = CreateSolidBrush(RGB(0,0,0));
    FillRect(mem, &r, bg);
    DeleteObject(bg);

    /* Disegna due cifre "0 0" stilizzate come icona */
    DrawDigit(mem, 1, 2,  4, 12, 24, 2);
    DrawDigit(mem, 2, 17, 4, 12, 24, 2);

    /* Crea maschera monocromatica */
    HDC msk_dc = CreateCompatibleDC(NULL);
    HBITMAP msk = CreateBitmap(sz, sz, 1, 1, NULL);
    HBITMAP omsk = (HBITMAP)SelectObject(msk_dc, msk);
    HBRUSH wb = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH bb2 = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(msk_dc, &r, wb);
    (void)bb2;
    SelectObject(msk_dc, omsk);
    DeleteDC(msk_dc);

    ICONINFO ii = {TRUE, 0, 0, msk, bmp};
    HICON ico = CreateIconIndirect(&ii);

    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteObject(msk);
    DeleteDC(mem);
    return ico;
}

/* ─────────────────────────────────────────────────────────
 *  Popup aiuto  (tasto H)
 *  Finestra non modale, sfondo nero, testo bianco, X per chiudere
 * ───────────────────────────────────────────────────────── */
static HWND g_helpWnd = NULL;

static LRESULT CALLBACK HelpWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        /* sfondo nero */
        HBRUSH bg = CreateSolidBrush(RGB(0,0,0));
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        /* bordo grigio sottile */
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(80,80,80));
        HPEN oldp = (HPEN)SelectObject(hdc, pen);
        HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, nb);
        Rectangle(hdc, 0, 0, rc.right, rc.bottom);
        SelectObject(hdc, oldp); DeleteObject(pen);
        SelectObject(hdc, oldb);

        /* testo comandi */
        HFONT hf = CreateFont(16,0,0,0,FW_NORMAL,0,0,0,
            DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
        HFONT oldf = (HFONT)SelectObject(hdc, hf);
        SetBkMode(hdc, TRANSPARENT);

        /* Titolo */
        SetTextColor(hdc, RGB(220,220,220));
        HFONT hfB = CreateFont(17,0,0,0,FW_BOLD,0,0,0,
            DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
        SelectObject(hdc, hfB);
        RECT rT = {20,16,rc.right-40,36};
        DrawTextW(hdc, L"COMANDI", -1, &rT, DT_LEFT|DT_SINGLELINE);
        SelectObject(hdc, hf);

        /* Separatore */
        HPEN sep = CreatePen(PS_SOLID,1,RGB(50,50,50));
        HPEN olds = (HPEN)SelectObject(hdc,sep);
        MoveToEx(hdc,20,38,NULL); LineTo(hdc,rc.right-20,38);
        SelectObject(hdc,olds); DeleteObject(sep);

        /* Righe comandi */
        static const wchar_t *rows[][2] = {
            {L"F11",    L"Schermo intero / finestra"},
            {L"Doppio click", L"Schermo intero / finestra"},
            {L"T",      L"Toggle formato 12h / 24h"},
            {L"A",      L"Imposta sveglia"},
            {L"H",      L"Mostra questi comandi"},
            {L"ESC",    L"Ferma sveglia / esci da fullscreen / esci"},
        };
        int n = sizeof(rows)/sizeof(rows[0]);
        for (int i = 0; i < n; i++) {
            int y = 50 + i*26;
            /* tasto — bianco */
            SetTextColor(hdc, RGB(220,220,220));
            RECT rK = {20, y, 140, y+22};
            DrawTextW(hdc, rows[i][0], -1, &rK, DT_LEFT|DT_SINGLELINE);
            /* descrizione — grigio */
            SetTextColor(hdc, RGB(130,130,130));
            RECT rD = {150, y, rc.right-20, y+22};
            DrawTextW(hdc, rows[i][1], -1, &rD, DT_LEFT|DT_SINGLELINE);
        }

        SelectObject(hdc, oldf);
        DeleteObject(hf);
        DeleteObject(hfB);

        /* Pulsante X in alto a destra */
        SetTextColor(hdc, RGB(160,160,160));
        HFONT hfX = CreateFont(18,0,0,0,FW_BOLD,0,0,0,
            DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Segoe UI");
        HFONT oldX = (HFONT)SelectObject(hdc, hfX);
        RECT rX = {rc.right-30, 8, rc.right-8, 30};
        DrawTextW(hdc, L"\u2715", -1, &rX, DT_CENTER|DT_SINGLELINE);
        SelectObject(hdc, oldX); DeleteObject(hfX);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONUP: {
        /* click sulla X in alto a destra */
        RECT rc; GetClientRect(hwnd, &rc);
        int x = LOWORD(lp), y = HIWORD(lp);
        if (x >= rc.right-32 && y <= 32) {
            DestroyWindow(hwnd);
            g_helpWnd = NULL;
        }
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE || wp == 'H') {
            DestroyWindow(hwnd);
            g_helpWnd = NULL;
        }
        return 0;
    case WM_DESTROY:
        g_helpWnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void ShowHelpPopup(HWND parent)
{
    /* Se già aperto, chiudi */
    if (g_helpWnd && IsWindow(g_helpWnd)) {
        DestroyWindow(g_helpWnd);
        g_helpWnd = NULL;
        return;
    }

    /* Registra classe una volta sola */
    static BOOL registered = FALSE;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc   = HelpWndProc;
        wc.hInstance     = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
        wc.lpszClassName = L"HelpPopupClass";
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassExW(&wc);
        registered = TRUE;
    }

    int W = 360, H = 230;

    /* Centra sulla finestra parent */
    RECT rp; GetWindowRect(parent, &rp);
    int x = rp.left + (rp.right-rp.left-W)/2;
    int y = rp.top  + (rp.bottom-rp.top-H)/2;

    g_helpWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"HelpPopupClass", L"Comandi",
        WS_POPUP | WS_VISIBLE,
        x, y, W, H,
        parent, NULL,
        (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE),
        NULL);
}

/* ─────────────────────────────────────────────────────────
 *  WndProc
 * ───────────────────────────────────────────────────────── */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        SetThreadExecutionState(ES_CONTINUOUS|ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED);
        SetTimer(hwnd, 1, 1000, NULL);
        SetTimer(hwnd, 2, 600000, NULL);
        CloseHandle(CreateThread(NULL,0,FetchWeatherThread,NULL,0,NULL));
        {
            HICON ico = CreateClockIcon();
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)ico);
            SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)ico);
        }
        return 0;

    case WM_TIMER:
        if (wp == 1) {
            /* Controlla sveglia */
            {
                time_t t = time(NULL);
                struct tm *tm = localtime(&t);
                int key = tm->tm_hour*60 + tm->tm_min;
                if (tm->tm_hour != g_alarm_hour || tm->tm_min != g_alarm_min) {
                    g_alarm_last_min = -1;       /* fuori dal minuto della sveglia: riarma */
                } else if (g_alarm_set && !g_alarm_firing && g_alarm_last_min != key) {
                    g_alarm_firing   = TRUE;
                    g_alarm_ticks    = 0;
                    g_alarm_last_min = key;      /* una volta sola per minuto */
                }
            }
            if (g_alarm_firing) {
                Beep(1000, 200);
                g_alarm_ticks++;
                if (g_alarm_ticks >= 30) {  /* 30 secondi poi si spegne */
                    g_alarm_firing = FALSE;
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
        }
        if (wp == 2)
            CloseHandle(CreateThread(NULL,0,FetchWeatherThread,NULL,0,NULL));
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_F11)  ToggleFullscreen(hwnd);
        if (wp == 'H')     ShowHelpPopup(hwnd);
        if (wp == 'T')     { g_format24 = !g_format24; InvalidateRect(hwnd,NULL,FALSE); }
        if (wp == 'A')     { ShowAlarmDialog(hwnd); InvalidateRect(hwnd,NULL,FALSE); }
        if (wp == VK_ESCAPE) {
            if (g_alarm_firing) { g_alarm_firing=FALSE; g_alarm_ticks=0; }
            else if (g_helpWnd && IsWindow(g_helpWnd)) { DestroyWindow(g_helpWnd); g_helpWnd=NULL; }
            else if (g_fullscreen) ToggleFullscreen(hwnd);
            else PostQuitMessage(0);
        }
        return 0;

    case WM_LBUTTONDBLCLK:
        ToggleFullscreen(hwnd);
        return 0;

    case WM_PAINT:
        OnPaint(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        SetThreadExecutionState(ES_CONTINUOUS);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ─────────────────────────────────────────────────────────
 *  WinMain
 * ───────────────────────────────────────────────────────── */
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmd, int nShow)
{
    (void)hPrev; (void)lpCmd;

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.hInstance     = hInst;
    wc.lpfnWndProc   = WndProc;
    wc.lpszClassName = L"DigitalClockClass";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style         = CS_DBLCLKS;   /* abilita doppio click */
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0,
        L"DigitalClockClass", L"Orologio Digitale",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 900, 450,
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
