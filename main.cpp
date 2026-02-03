#include <windows.h>
#include <vector>
#include <string>

// =====================
// CONFIGURATION
// =====================
const int GRID_ROWS    = 20;
const int GRID_COLS    = 12;
const wchar_t* POP_TEXT = L"POP";

const int CELL_SIZE = 16; // pixels per button
// =====================

HINSTANCE g_hInstance;
std::vector<HWND> g_buttons;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            HWND button = (HWND)lParam;
            SetWindowTextW(button, POP_TEXT);
            EnableWindow(button, FALSE);
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateGrid(HWND hwnd)
{
    for (int row = 0; row < GRID_ROWS; ++row)
    {
        for (int col = 0; col < GRID_COLS; ++col)
        {
            HWND button = CreateWindowW(
                L"BUTTON",
                L"",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                col * CELL_SIZE,
                row * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE,
                hwnd,
                nullptr,
                g_hInstance,
                nullptr
            );

            g_buttons.push_back(button);
        }
    }
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow
)
{
    g_hInstance = hInstance;

    const wchar_t CLASS_NAME[] = L"BubbleWrapWindow";

    WNDCLASS wc{};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    int windowWidth  = GRID_COLS * CELL_SIZE + 16;
    int windowHeight = GRID_ROWS * CELL_SIZE + 39;

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Digital Bubble Wrap",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowWidth,
        windowHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    CreateGrid(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
