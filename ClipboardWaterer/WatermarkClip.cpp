// WatermarkClip.cpp
// Compile with: /DUNICODE /D_UNICODE and link with Comdlg32.lib, Msimg32.lib, Gdi32.lib, User32.lib, Advapi32.lib
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <commdlg.h>
#include <wingdi.h>
#include <strsafe.h>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Msimg32.lib")

// Global variable to hold the watermark bitmap handle.
HBITMAP g_hWatermark = NULL;

//--------------------------------------------------------------------------------
// SelectWatermarkImage - Opens a file dialog to let the user choose a BMP file.
//--------------------------------------------------------------------------------
void SelectWatermarkImage(HWND hWnd)
{
    OPENFILENAME ofn;
    wchar_t fileName[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Bitmap Files (*.bmp)\0*.bmp\0All Files\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = L"bmp";

    if (GetOpenFileName(&ofn))
    {
        if (g_hWatermark)
        {
            DeleteObject(g_hWatermark);
            g_hWatermark = NULL;
        }
        // Load the bitmap image from file.
        g_hWatermark = (HBITMAP)LoadImage(NULL, fileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (g_hWatermark)
        {
            MessageBox(hWnd, L"Watermark image loaded successfully.", L"Info", MB_OK);
        }
        else
        {
            MessageBox(hWnd, L"Failed to load watermark image.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

//--------------------------------------------------------------------------------
// CombineImages - Creates a composite bitmap by drawing the watermark over the source bitmap.
// Returns a new HBITMAP with the result.
//--------------------------------------------------------------------------------
HBITMAP CombineImages(HBITMAP hSource, HBITMAP hWatermark)
{
    if (!hSource || !hWatermark)
        return NULL;

    BITMAP bmSource, bmWatermark;
    GetObject(hSource, sizeof(BITMAP), &bmSource);
    GetObject(hWatermark, sizeof(BITMAP), &bmWatermark);

    // Get a screen DC for compatibility
    HDC hdcScreen = GetDC(NULL);
    HDC hdcSource = CreateCompatibleDC(hdcScreen);
    HDC hdcComposite = CreateCompatibleDC(hdcScreen);
    HDC hdcWatermark = CreateCompatibleDC(hdcScreen);

    // Create a new bitmap to host the composite image.
    HBITMAP hComposite = CreateCompatibleBitmap(hdcScreen, bmSource.bmWidth, bmSource.bmHeight);
    HBITMAP hOldComposite = (HBITMAP)SelectObject(hdcComposite, hComposite);

    // Select the source image into its DC.
    HBITMAP hOldSource = (HBITMAP)SelectObject(hdcSource, hSource);

    // Start with a copy of the source image.
    BitBlt(hdcComposite, 0, 0, bmSource.bmWidth, bmSource.bmHeight,
        hdcSource, 0, 0, SRCCOPY);

    // Prepare the watermark DC.
    HBITMAP hOldWatermark = (HBITMAP)SelectObject(hdcWatermark, hWatermark);

    // Determine the position for the watermark (e.g., lower right corner with a margin).
    int margin = 10;
    int x = bmSource.bmWidth - bmWatermark.bmWidth - margin;
    int y = bmSource.bmHeight - bmWatermark.bmHeight - margin;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    // Use AlphaBlend to mix the watermark over the source. Adjust SourceConstantAlpha to change watermark opacity if desired.
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255; // 255 = fully opaque; lower to make watermark translucent.
    blend.AlphaFormat = 0;           // This example assumes no per-pixel alpha.

    AlphaBlend(hdcComposite, x, y, bmWatermark.bmWidth, bmWatermark.bmHeight,
        hdcWatermark, 0, 0, bmWatermark.bmWidth, bmWatermark.bmHeight, blend);

    // Cleanup: restore original objects and delete DCs.
    SelectObject(hdcComposite, hOldComposite);
    SelectObject(hdcSource, hOldSource);
    SelectObject(hdcWatermark, hOldWatermark);
    DeleteDC(hdcSource);
    DeleteDC(hdcComposite);
    DeleteDC(hdcWatermark);
    ReleaseDC(NULL, hdcScreen);

    return hComposite;
}

//--------------------------------------------------------------------------------
// ProcessClipboardUpdate - Checks if the clipboard contains a bitmap, then overlays the watermark.
//--------------------------------------------------------------------------------
void ProcessClipboardUpdate(HWND hWnd)
{
    if (!g_hWatermark)
    {
        // No watermark selected; nothing to do.
        return;
    }
    if (!IsClipboardFormatAvailable(CF_BITMAP))
        return;

    if (!OpenClipboard(hWnd))
        return;

    HBITMAP hClipboardBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
    if (!hClipboardBitmap)
    {
        CloseClipboard();
        return;
    }

    // Duplicate the clipboard bitmap because the handle from the clipboard is managed by the system.
    BITMAP bm;
    GetObject(hClipboardBitmap, sizeof(BITMAP), &bm);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmapCopy = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmapCopy);

    // Create a temporary DC to access the original clipboard bitmap.
    HDC hdcTemp = CreateCompatibleDC(hdcScreen);
    HBITMAP hOldTemp = (HBITMAP)SelectObject(hdcTemp, hClipboardBitmap);

    BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcTemp, 0, 0, SRCCOPY);

    // Clean up temporary DCs.
    SelectObject(hdcMem, hOldBitmap);
    SelectObject(hdcTemp, hOldTemp);
    DeleteDC(hdcTemp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Create composite image by overlaying the watermark.
    HBITMAP hComposite = CombineImages(hBitmapCopy, g_hWatermark);
    if (hComposite)
    {
        EmptyClipboard();
        // SetClipboardData takes ownership of the hComposite handle.
        SetClipboardData(CF_BITMAP, hComposite);
    }
    // Delete our copy of the original clipboard image.
    DeleteObject(hBitmapCopy);

    CloseClipboard();
}

//--------------------------------------------------------------------------------
// Window Procedure
//--------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Register for clipboard update notifications.
        if (!AddClipboardFormatListener(hWnd))
        {
            MessageBox(hWnd, L"Failed to add clipboard format listener.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
    break;

    case WM_CLIPBOARDUPDATE:
    {
        // Process new clipboard content.
        ProcessClipboardUpdate(hWnd);
    }
    break;

    case WM_COMMAND:
    {
        // Use menu command (ID 1) to allow the user to select a watermark image.
        if (LOWORD(wParam) == 1)
        {
            SelectWatermarkImage(hWnd);
        }
    }
    break;

    case WM_DESTROY:
    {
        RemoveClipboardFormatListener(hWnd);
        if (g_hWatermark)
        {
            DeleteObject(g_hWatermark);
            g_hWatermark = NULL;
        }
        PostQuitMessage(0);
    }
    break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//--------------------------------------------------------------------------------
// WinMain - Program entry point
//--------------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    const wchar_t szWindowClass[] = L"WatermarkClipWindow";
    const wchar_t szTitle[] = L"Clipboard Watermark Utility";

    WNDCLASSEXW wcex;
    ZeroMemory(&wcex, sizeof(wcex));
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;

    if (!RegisterClassExW(&wcex))
        return 1;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 500, 400,
        NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return 1;

    // Create a simple menu with a command to select a watermark image.
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, 1, L"Select Watermark Image");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
    SetMenu(hWnd, hMenu);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}