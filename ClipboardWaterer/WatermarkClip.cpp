// ClipboardWaterer.cpp
// Compile with: /DUNICODE /D_UNICODE and link with Comdlg32.lib, Msimg32.lib, Gdi32.lib, User32.lib

#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <commdlg.h>
#include <wingdi.h>
#include <strsafe.h>

#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Msimg32.lib")

//---------------------------------------------------------------------------
// Enumeration for watermark position options.
//---------------------------------------------------------------------------
enum WatermarkPositionEnum {
    WM_POS_TOP_LEFT = 0,
    WM_POS_TOP_CENTER,
    WM_POS_TOP_RIGHT,
    WM_POS_MIDDLE_LEFT,
    WM_POS_CENTER,
    WM_POS_MIDDLE_RIGHT,
    WM_POS_BOTTOM_LEFT,
    WM_POS_BOTTOM_CENTER,
    WM_POS_BOTTOM_RIGHT
};

// Global variable for the currently selected watermark position.
// Default is Bottom Right.
int g_WatermarkPos = WM_POS_BOTTOM_RIGHT;

// Global variable for the watermark bitmap handle.
HBITMAP g_hWatermark = NULL;

// Define control IDs for the radio buttons.
#define ID_RADIO_TOP_LEFT      1001
#define ID_RADIO_TOP_CENTER    1002
#define ID_RADIO_TOP_RIGHT     1003
#define ID_RADIO_MIDDLE_LEFT   1004
#define ID_RADIO_CENTER        1005
#define ID_RADIO_MIDDLE_RIGHT  1006
#define ID_RADIO_BOTTOM_LEFT   1007
#define ID_RADIO_BOTTOM_CENTER 1008
#define ID_RADIO_BOTTOM_RIGHT  1009

//---------------------------------------------------------------------------
// SelectWatermarkImage - Opens a file dialog to let the user choose a BMP file.
//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
// CombineImages - Creates a composite bitmap by drawing the watermark over the source bitmap.
// Returns a new HBITMAP with the result.
//---------------------------------------------------------------------------
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

    // Copy the source image into the composite image.
    BitBlt(hdcComposite, 0, 0, bmSource.bmWidth, bmSource.bmHeight,
        hdcSource, 0, 0, SRCCOPY);

    // Prepare the watermark DC.
    HBITMAP hOldWatermark = (HBITMAP)SelectObject(hdcWatermark, hWatermark);

    // Determine position (x,y) for the watermark based on the selected option.
    int margin = 10;
    int x = 0, y = 0;
    switch (g_WatermarkPos)
    {
    case WM_POS_TOP_LEFT:
        x = margin;
        y = margin;
        break;
    case WM_POS_TOP_CENTER:
        x = (bmSource.bmWidth - bmWatermark.bmWidth) / 2;
        y = margin;
        break;
    case WM_POS_TOP_RIGHT:
        x = bmSource.bmWidth - bmWatermark.bmWidth - margin;
        y = margin;
        break;
    case WM_POS_MIDDLE_LEFT:
        x = margin;
        y = (bmSource.bmHeight - bmWatermark.bmHeight) / 2;
        break;
    case WM_POS_CENTER:
        x = (bmSource.bmWidth - bmWatermark.bmWidth) / 2;
        y = (bmSource.bmHeight - bmWatermark.bmHeight) / 2;
        break;
    case WM_POS_MIDDLE_RIGHT:
        x = bmSource.bmWidth - bmWatermark.bmWidth - margin;
        y = (bmSource.bmHeight - bmWatermark.bmHeight) / 2;
        break;
    case WM_POS_BOTTOM_LEFT:
        x = margin;
        y = bmSource.bmHeight - bmWatermark.bmHeight - margin;
        break;
    case WM_POS_BOTTOM_CENTER:
        x = (bmSource.bmWidth - bmWatermark.bmWidth) / 2;
        y = bmSource.bmHeight - bmWatermark.bmHeight - margin;
        break;
    case WM_POS_BOTTOM_RIGHT:
    default:
        x = bmSource.bmWidth - bmWatermark.bmWidth - margin;
        y = bmSource.bmHeight - bmWatermark.bmHeight - margin;
        break;
    }

    // Use AlphaBlend to mix the watermark over the source image.
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255; // Fully opaque; adjust if transparency is desired.
    blend.AlphaFormat = 0;           // Assume bitmap has no per-pixel alpha.

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

//---------------------------------------------------------------------------
// ProcessClipboardUpdate - Checks if the clipboard contains a bitmap,
// then overlays the watermark.
//---------------------------------------------------------------------------
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

    // Duplicate the clipboard bitmap.
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

    // Cleanup temporary DCs.
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

//---------------------------------------------------------------------------
// CreateRadioButtons - Create nine radio buttons for watermark position
//---------------------------------------------------------------------------
void CreateRadioButtons(HWND hWnd, HINSTANCE hInst)
{
    // Dimensions and layout for radio buttons.
    int radioWidth = 100;
    int radioHeight = 20;
    int startX = 10;
    int startY = 300; // using the bottom region of a 500x400 window
    int spacingX = radioWidth + 10;
    int spacingY = radioHeight + 5;

    // Top row
    CreateWindowW(L"BUTTON", L"Top Left",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        startX, startY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_TOP_LEFT, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Top Center",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + spacingX, startY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_TOP_CENTER, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Top Right",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + 2 * spacingX, startY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_TOP_RIGHT, hInst, NULL);

    // Middle row
    CreateWindowW(L"BUTTON", L"Middle Left",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        startX, startY + spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_MIDDLE_LEFT, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Center",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + spacingX, startY + spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_CENTER, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Middle Right",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + 2 * spacingX, startY + spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_MIDDLE_RIGHT, hInst, NULL);

    // Bottom row
    CreateWindowW(L"BUTTON", L"Bottom Left",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        startX, startY + 2 * spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_BOTTOM_LEFT, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Bottom Center",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + spacingX, startY + 2 * spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_BOTTOM_CENTER, hInst, NULL);
    CreateWindowW(L"BUTTON", L"Bottom Right",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        startX + 2 * spacingX, startY + 2 * spacingY,
        radioWidth, radioHeight, hWnd, (HMENU)ID_RADIO_BOTTOM_RIGHT, hInst, NULL);

    // Set default selection to "Bottom Right"
    HWND hRadioDefault = GetDlgItem(hWnd, ID_RADIO_BOTTOM_RIGHT);
    SendMessage(hRadioDefault, BM_SETCHECK, BST_CHECKED, 0);
}

//---------------------------------------------------------------------------
// Window Procedure
//---------------------------------------------------------------------------
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
        // Create a simple menu with a command to select a watermark image.
        HMENU hMenu = CreateMenu();
        HMENU hFileMenu = CreatePopupMenu();
        AppendMenuW(hFileMenu, MF_STRING, 1, L"Select Watermark Image");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
        SetMenu(hWnd, hMenu);

        // Create radio buttons for position selection.
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        HINSTANCE hInst = pcs->hInstance;
        CreateRadioButtons(hWnd, hInst);
    }
    break;

    case WM_CLIPBOARDUPDATE:
    {
        ProcessClipboardUpdate(hWnd);
    }
    break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case 1:
            // Menu: select watermark image.
            SelectWatermarkImage(hWnd);
            break;

            // Process radio button selection.
        case ID_RADIO_TOP_LEFT:
            g_WatermarkPos = WM_POS_TOP_LEFT;
            break;
        case ID_RADIO_TOP_CENTER:
            g_WatermarkPos = WM_POS_TOP_CENTER;
            break;
        case ID_RADIO_TOP_RIGHT:
            g_WatermarkPos = WM_POS_TOP_RIGHT;
            break;
        case ID_RADIO_MIDDLE_LEFT:
            g_WatermarkPos = WM_POS_MIDDLE_LEFT;
            break;
        case ID_RADIO_CENTER:
            g_WatermarkPos = WM_POS_CENTER;
            break;
        case ID_RADIO_MIDDLE_RIGHT:
            g_WatermarkPos = WM_POS_MIDDLE_RIGHT;
            break;
        case ID_RADIO_BOTTOM_LEFT:
            g_WatermarkPos = WM_POS_BOTTOM_LEFT;
            break;
        case ID_RADIO_BOTTOM_CENTER:
            g_WatermarkPos = WM_POS_BOTTOM_CENTER;
            break;
        case ID_RADIO_BOTTOM_RIGHT:
            g_WatermarkPos = WM_POS_BOTTOM_RIGHT;
            break;
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

//---------------------------------------------------------------------------
// WinMain - Program entry point
//---------------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    const wchar_t szWindowClass[] = L"ClipboardWatererWindow";
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
