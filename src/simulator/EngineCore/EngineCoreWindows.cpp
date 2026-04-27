///   File: EngineCoreWindows.cpp

//This class bases on Code::Blocks OpenGL template (OpenGL + WinAPI)

#ifdef _WIN32

#include "EngineCoreWindows.h"

EngineCore *EngineCore::instance = nullptr;
GLuint EngineCore::fontListBase = 0;

int EngineCore::init()
{
    SetConsoleTitle("City traffic simulation");

    instance = this;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEX wcex;

    HGLRC hRC;

    /* register window class */
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;

    if (!RegisterClassEx(&wcex))
        throw ExceptionClass("Cannot register window class");

    /* create main window */
    hwnd = CreateWindowEx(0,
                          "GLSample",
                          "City traffic simulation",
                          WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          width,
                          height,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    if (hwnd == NULL)
        throw ExceptionClass("Cannot create a window");

    /* enable OpenGL for the window */
    enableOpenGL(hwnd, &hDC, &hRC);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0);
    initLight();

    // Initialize bitmap font for HUD text rendering
    fontListBase = glGenLists(128);
    HFONT hFont = CreateFont(
        -14,            // height (negative = character height)
        0,              // width (auto)
        0, 0,           // escapement, orientation
        FW_NORMAL,      // weight
        FALSE, FALSE, FALSE,  // italic, underline, strikeout
        ANSI_CHARSET,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        FF_DONTCARE | DEFAULT_PITCH,
        "Consolas"      // monospace font for clean alignment
    );
    HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
    wglUseFontBitmaps(hDC, 0, 128, fontListBase);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);

    return 0;
}

void EngineCore::checkKeyboard()
{
    bool isShiftPressed = GetAsyncKeyState(VK_LSHIFT) != 0;
    for (int i=0; i < 128; i++)
    {
        if (GetAsyncKeyState(i) != 0)
        {
            if (i >= 'A' && i <= 'Z' && !isShiftPressed) keyHeld(i + 32);
            else keyHeld(i);
        }
    }
}

void EngineCore::checkMouse()
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    if((GetKeyState(VK_LBUTTON) & 0x100) != 0 || (GetKeyState(VK_RBUTTON) & 0x100) != 0)
        mouseMove((cursorPos.x - prevMouseX), (cursorPos.y - prevMouseY));

    prevMouseX = cursorPos.x;
    prevMouseY = cursorPos.y;
}

float EngineCore::getDeltaTime()
{
    clock_t newTime = clock();
    float realDelta = newTime - prevTime;
    realDelta /= CLOCKS_PER_SEC;
    prevTime = newTime;

    return realDelta;
}

void EngineCore::checkEvents()
{
    if (GetActiveWindow() == hwnd)
    {
        checkKeyboard();
        checkMouse();
    }

    MSG msg;

    /* check for messages */
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        /* handle or dispatch messages */
        if (msg.message != WM_QUIT)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void EngineCore::swapBuffers()
{
    SwapBuffers(hDC);
}

LRESULT CALLBACK EngineCore::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            instance->keyPressed(27);
            break;

        case WM_KEYDOWN:
            instance->keyPressed(wParam);
            break;

        case WM_DESTROY:
            return 0;

        case WM_SIZE:
            instance->width = (int)LOWORD(lParam);
            instance->height = (int)HIWORD(lParam);
            instance->updateRatio();
            break;

        case WM_SETFOCUS:
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            instance->prevMouseX = cursorPos.x;
            instance->prevMouseY = cursorPos.y;
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EngineCore::enableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    /* get the device context (DC) */
    *hDC = GetDC(hwnd);

    /* set the pixel format for the DC */
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    /* create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);
}

void EngineCore::disableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

void EngineCore::showWindow()
{
    ShowWindow(hwnd, SW_SHOWDEFAULT);
}

void EngineCore::hideWindow()
{
    ShowWindow(hwnd, SW_HIDE);
}

#endif // _WIN32
