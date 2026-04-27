///   File: EngineCoreWindows.h

#ifndef ENGINECOREWINDOWS_H
#define ENGINECOREWINDOWS_H

#ifdef _WIN32

#include <GL/gl.h>

#include <ctime>
#include <windows.h>

#include "EngineCoreBase.h"

class EngineCore : protected EngineCoreBase
{
protected:

    int init();
    virtual ~EngineCore(){};

    float getDeltaTime();
    void checkEvents();
    void swapBuffers();

private:
    void showWindow();
    void hideWindow();

    clock_t prevTime;

    int prevMouseX;
    int prevMouseY;

    HDC hDC;
    HWND hwnd;
    static GLuint fontListBase;  // bitmap font display list base

    void enableOpenGL(HWND hwnd, HDC*, HGLRC*);
    void disableOpenGL(HWND, HDC, HGLRC);

    static LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);

    void checkKeyboard();
    void checkMouse();

    static EngineCore *instance;

public:
    static void SetCmdArgs(int argC, char **argV) {};
    static GLuint getFontListBase() { return fontListBase; }
};

#endif // _WIN32
#endif // ENGINECOREWINDOWS_H
