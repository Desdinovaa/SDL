/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_SWITCH

#include "../SDL_sysvideo.h"
#include "../../render/SDL_sysrender.h"
#include "../../events/SDL_keyboard_c.h"
#include "../../events/SDL_mouse_c.h"
#include "../../events/SDL_windowevents_c.h"

#include "SDL_switchvideo.h"
#include "SDL_switchopengles.h"
#include "SDL_switchtouch.h"
#include "SDL_switchkeyboard.h"
#include "SDL_switchmouse_c.h"
#include "SDL_switchswkb.h"

// Currently only one window
static SDL_Window *switch_window = NULL;
static AppletOperationMode operationMode;

static void
SWITCH_Destroy(SDL_VideoDevice *device)
{
    if (device) {
        SDL_free(device->internal);
        SDL_free(device);
    }
}

static SDL_VideoDevice *
SWITCH_CreateDevice(void)
{
    SDL_VideoDevice *device;

    // Initialize SDL_VideoDevice structure
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        return NULL;
    }

    // Setup amount of available displays
    device->num_displays = 0;

    // Set device free function
    device->free = SWITCH_Destroy;

    // Setup all functions which we can handle
    device->VideoInit = SWITCH_VideoInit;
    device->VideoQuit = SWITCH_VideoQuit;
    device->GetDisplayModes = SWITCH_GetDisplayModes;
    device->SetDisplayMode = SWITCH_SetDisplayMode;
    device->CreateSDLWindow = SWITCH_CreateWindow;
    device->SetWindowSize = SWITCH_SetWindowSize;
    device->DestroyWindow = SWITCH_DestroyWindow;

    device->GL_LoadLibrary = SWITCH_GLES_LoadLibrary;
    device->GL_GetProcAddress = SWITCH_GLES_GetProcAddress;
    device->GL_UnloadLibrary = SWITCH_GLES_UnloadLibrary;
    device->GL_CreateContext = SWITCH_GLES_CreateContext;
    device->GL_MakeCurrent = SWITCH_GLES_MakeCurrent;
    device->GL_SetSwapInterval = SWITCH_GLES_SetSwapInterval;
    device->GL_GetSwapInterval = SWITCH_GLES_GetSwapInterval;
    device->GL_SwapWindow = SWITCH_GLES_SwapWindow;
    device->GL_DestroyContext = SWITCH_GLES_DestroyContext;
    device->GL_DefaultProfileConfig = SWITCH_GLES_DefaultProfileConfig;

    device->StartTextInput = SWITCH_StartTextInput;
    device->StopTextInput = SWITCH_StopTextInput;
    device->HasScreenKeyboardSupport = SWITCH_HasScreenKeyboardSupport;
    device->IsScreenKeyboardShown = SWITCH_IsScreenKeyboardShown;

    device->PumpEvents = SWITCH_PumpEvents;

    return device;
}

VideoBootStrap SWITCH_bootstrap = {
    "Switch",
    "Nintendo Switch Video Driver",
    SWITCH_CreateDevice
};

/*****************************************************************************/
/* SDL Video and Display initialization/handling functions                   */
/*****************************************************************************/
bool
SWITCH_VideoInit(SDL_VideoDevice *_this)
{
    SDL_DisplayMode mode;

    SDL_zero(mode);
    mode.w = 1920;
    mode.h = 1080;
    mode.refresh_rate = 60;
    mode.format = SDL_PIXELFORMAT_RGBA8888;

    SDL_AddBasicVideoDisplay(&mode);

    // init psm service
    psmInitialize();
    // init touch
    SWITCH_InitTouch();
    // init keyboard
    SWITCH_InitKeyboard();
    // init mouse
    SWITCH_InitMouse();
    // init software keyboard
    SWITCH_InitSwkb();

    return true;
}

void
SWITCH_VideoQuit(SDL_VideoDevice *_this)
{
    // this should not be needed if user code is right (SDL_GL_LoadLibrary/SDL_GL_UnloadLibrary calls match)
    // this (user) error doesn't have the same effect on switch thought, as the driver needs to be unloaded (crash)
    if(_this->gl_config.driver_loaded > 0) {
        SWITCH_GLES_UnloadLibrary(_this);
        _this->gl_config.driver_loaded = 0;
    }

    // exit touch
    SWITCH_QuitTouch();
    // exit keyboard
    SWITCH_QuitKeyboard();
    // exit mouse
    SWITCH_QuitMouse();
    // exit software keyboard
    SWITCH_QuitSwkb();
    // exit psm service
    psmExit();
}

bool
SWITCH_GetDisplayModes(SDL_VideoDevice *_this, SDL_VideoDisplay *display)
{
    SDL_DisplayMode mode;

    // 1920x1080 RGBA8888, default mode
    SDL_zero(mode);
    mode.w = 1920;
    mode.h = 1080;
    mode.refresh_rate = 60;
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    SDL_AddFullscreenDisplayMode(display, &mode);

    // 1280x720 RGBA8888
    SDL_zero(mode);
    mode.w = 1280;
    mode.h = 720;
    mode.refresh_rate = 60;
    mode.format = SDL_PIXELFORMAT_RGBA8888;
    SDL_AddFullscreenDisplayMode(display, &mode);

    return true;
}

bool
SWITCH_SetDisplayMode(SDL_VideoDevice *_this, SDL_VideoDisplay *display, SDL_DisplayMode *mode)
{
    SDL_WindowData *data = switch_window->internal;
    SDL_GLContext ctx = SDL_GL_GetCurrentContext();
    NWindow *nWindow = nwindowGetDefault();

    if (data && data->egl_surface != EGL_NO_SURFACE) {
        SDL_EGL_MakeCurrent(_this, NULL, NULL);
        SDL_EGL_DestroySurface(_this, data->egl_surface);
        nwindowSetDimensions(nWindow, mode->w, mode->h);
        data->egl_surface = SDL_EGL_CreateSurface(_this, switch_window, nWindow);
        SDL_EGL_MakeCurrent(_this, data->egl_surface, ctx);
    }

    return true;
}

bool
SWITCH_CreateWindow(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID create_props)
{
    Result rc;
    SDL_WindowData *window_data = NULL;
    NWindow *nWindow = NULL;

    if (switch_window) {
        return SDL_SetError("Switch only supports one window");
    }

    if (!_this->egl_data) {
        return SDL_SetError("EGL not initialized");
    }

    window_data = (SDL_WindowData *) SDL_calloc(1, sizeof(SDL_WindowData));
    if (!window_data) {
        return SDL_OutOfMemory();
    }

    nWindow = nwindowGetDefault();

    rc = nwindowSetDimensions(nWindow, window->w, window->h);
    if (R_FAILED(rc)) {
        return SDL_SetError("Could not set NWindow dimensions: 0x%x", rc);
    }

    window_data->egl_surface = SDL_EGL_CreateSurface(_this, window, nWindow);
    if (window_data->egl_surface == EGL_NO_SURFACE) {
        return SDL_SetError("Could not create GLES window surface");
    }

    // Setup driver data for this window
    window->internal = window_data;
    switch_window = window;

    // starting operation mode
    operationMode = appletGetOperationMode();

    // One window, it always has focus
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);

    // Window has been successfully created
    return true;
}

void
SWITCH_DestroyWindow(SDL_VideoDevice *_this, SDL_Window *window)
{
    SDL_WindowData *data = window->internal;

    if (window == switch_window) {
        if (data) {
            if (data->egl_surface != EGL_NO_SURFACE) {
                SDL_EGL_MakeCurrent(_this, NULL, NULL);
                SDL_EGL_DestroySurface(_this, data->egl_surface);
            }
            if(window->internal) {
                SDL_free(window->internal);
                window->internal = NULL;
            }
        }
        switch_window = NULL;
    }
}

void
SWITCH_SetWindowSize(SDL_VideoDevice *_this, SDL_Window *window)
{
    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        window->last_size_pending = false;
        return;
    }

    SDL_WindowData *data = window->internal;
    SDL_GLContext ctx = SDL_GL_GetCurrentContext();
    NWindow *nWindow = nwindowGetDefault();

    if(window->w != window->pending.w || window->h != window->pending.h) {
        if (data && data->egl_surface != EGL_NO_SURFACE) {
            SDL_EGL_MakeCurrent(_this, NULL, NULL);
            SDL_EGL_DestroySurface(_this, data->egl_surface);
            nwindowSetDimensions(nWindow, window->pending.w, window->pending.h);
            data->egl_surface = SDL_EGL_CreateSurface(_this, window, nWindow);
            SDL_EGL_MakeCurrent(_this, data->egl_surface, ctx);
            SDL_SendWindowEvent(window, SDL_EVENT_WINDOW_RESIZED, window->pending.w, window->pending.h);
        } else {
            window->last_size_pending = false;
        }
    }
}

void
SWITCH_PumpEvents(SDL_VideoDevice *_this)
{
    AppletOperationMode om;

    if (!appletMainLoop()) {
        SDL_Event ev;
        ev.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&ev);
        return;
    }

    // we don't want other inputs overlapping with software keyboard
    if(!switch_window || !SDL_TextInputActive(switch_window)) {
        SWITCH_PollTouch();
        SWITCH_PollKeyboard();
        SWITCH_PollMouse();
    }

    if (switch_window) {
        SWITCH_PollSwkb(switch_window);
    }

    // handle docked / un-docked modes
    // note that SDL_WINDOW_RESIZABLE is only possible in windowed mode,
    // so we don't care about current fullscreen/windowed status
    if(switch_window && switch_window->flags & SDL_WINDOW_RESIZABLE) {
        om = appletGetOperationMode();
        if(om != operationMode) {
            operationMode = om;
            if(operationMode == AppletOperationMode_Handheld) {
                SDL_SetWindowSize(switch_window, 1280, 720);
            } else {
                SDL_SetWindowSize(switch_window, 1920, 1080);
            }
        }
    }
}

#endif // SDL_VIDEO_DRIVER_SWITCH
