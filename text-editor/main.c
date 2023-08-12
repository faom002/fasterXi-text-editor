#include <stdio.h>
#include <windows.h>
#include <commdlg.h>

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

#define IDM_OPEN 101
#define IDM_SAVE 102
#define IDM_NEW 103

#define MAX_ROWS 25
#define MAX_COLS 80

HBRUSH hBrushBackground;
HFONT hFont;
int cursorX, cursorY; // Cursor position

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char *className = "FasterXi";

    WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0,
                       GetModuleHandle(NULL), NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1),
                       NULL, className, NULL};
    RegisterClassEx(&wcex);

    hBrushBackground = CreateSolidBrush(RGB(0, 0, 0)); // Dark background color

    // Create a font for the text editor
    hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                       CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE,
                       "Consolas");

    HWND hwnd = CreateWindow(className, "FasterXi - Windows-like Text Editor", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                             CW_USEDEFAULT, 800, 800, NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        return 1;
    }

    cursorX = cursorY = 0; // Initialize cursor position

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up resources
    DeleteObject(hBrushBackground);
    DeleteObject(hFont);

    return (int)msg.wParam;
}

char buffer[MAX_ROWS][MAX_COLS] = {0};
int numRows = 0;
int currentRow = 0;
int currentCol = 0;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HMENU hMenu;

    switch (msg) {
        case WM_CREATE: {
            hMenu = CreateMenu();
            HMENU hSubMenu = CreatePopupMenu();
            AppendMenu(hSubMenu, MF_STRING, IDM_OPEN, "Open...");
            AppendMenu(hSubMenu, MF_STRING, IDM_SAVE, "Save");
            AppendMenu(hSubMenu, MF_STRING, IDM_NEW, "New");
            AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, "File");
            SetMenu(hwnd, hMenu);
            break;
        }

        case WM_CHAR: {
            if (numRows < MAX_ROWS && currentCol < MAX_COLS - 1) {
                int lastRow = currentRow;
                int lastCol = currentCol;

                if (wParam == VK_RETURN) {  // Handle Enter key
                    if (currentRow < MAX_ROWS - 1) {
                        currentRow++;
                        currentCol = 0;
                        buffer[currentRow][0] = '\0';  // Start a new line
                    }
                } else if (wParam == VK_BACK && currentCol > 0) {  // Handle Backspace key
                    currentCol--;
                    buffer[currentRow][currentCol] = '\0';
                } else if (wParam >= 32 && wParam <= 126) {  // Handle printable characters
                    buffer[currentRow][currentCol] = (char)wParam;
                    currentCol++;
                    buffer[currentRow][currentCol] = '\0';
                }

                InvalidateRect(hwnd, NULL, TRUE);  // Repaint the window

                // Update cursor position
                cursorX = currentCol;
                cursorY = currentRow;

                // Update the caret position
                if (GetFocus() == hwnd) {
                    CreateCaret(hwnd, NULL, 2, 16);
                    SetCaretPos(cursorX * 8, cursorY * 20);
                    ShowCaret(hwnd);
                }
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            SelectObject(hdc, hBrushBackground);
            SelectObject(hdc, hFont);

            // Fill background with dark color
            FillRect(hdc, &rect, hBrushBackground);

            SetTextColor(hdc, RGB(255, 255, 255)); // Text color is white
            SetBkMode(hdc, TRANSPARENT); // Text background is transparent

            for (int i = 0; i <= numRows; i++) {
                TextOut(hdc, rect.left, i * 20, buffer[i], strlen(buffer[i]));
            }

            // Draw the cursor/pointer
            if (GetFocus() == hwnd) {
                RECT cursorRect = {cursorX * 8, cursorY * 20, (cursorX + 1) * 8, (cursorY + 1) * 20};
                DrawFocusRect(hdc, &cursorRect);
            }

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDM_OPEN: {
                    OPENFILENAME ofn;
                    char szFileName[MAX_PATH] = "";
                    char fileContent[MAX_ROWS * MAX_COLS];

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST;

                    if (GetOpenFileName(&ofn)) {
                        FILE *file = fopen(szFileName, "r");
                        if (file != NULL) {
                            // Read content from the file
                            numRows = 0;
                            currentRow = 0;
                            while (fgets(buffer[numRows], MAX_COLS, file) != NULL && numRows < MAX_ROWS) {
                                buffer[numRows][strlen(buffer[numRows]) - 1] = '\0';  // Remove newline
                                numRows++;
                            }
                            fclose(file);
                            InvalidateRect(hwnd, NULL, TRUE);  // Repaint the window
                        }
                    }
                    break;
                }

                case IDM_SAVE: {
                    OPENFILENAME ofn;
                    char szFileName[MAX_PATH] = "";

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = "txt";

                    if (GetSaveFileName(&ofn)) {
                        FILE *file = fopen(szFileName, "w");
                        if (file != NULL) {
                            for (int i = 0; i <= numRows; i++) {
                                fputs(buffer[i], file);
                                fputc('\n', file);
                            }
                            fclose(file);
                        }
                    }
                    break;
                }

                case IDM_NEW: {
                    numRows = 0;
                    currentRow = 0;
                    currentCol = 0;
                    memset(buffer, 0, sizeof(buffer));
                    InvalidateRect(hwnd, NULL, TRUE);
                    break;
                }
            }
            break;
        }

        case WM_SETFOCUS: {
            CreateCaret(hwnd, NULL, 2, 16);
            SetCaretPos(cursorX * 8, cursorY * 20);
            ShowCaret(hwnd);
            break;
        }

        case WM_KILLFOCUS: {
            HideCaret(hwnd);
            DestroyCaret();
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}
