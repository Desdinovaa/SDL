//
// Created by cpasjuste on 22/04/2020.
//

#include "SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_SWITCH

#include <switch.h>
#include "SDL_switchswkb.h"

static SwkbdInline kbd;
static SwkbdAppearArg kbdAppearArg;
static bool kbdInited;
static bool kbdShown;

void
SWITCH_InitSwkb()
{
}

void
SWITCH_PollSwkb(SDL_Window *window)
{
    if(kbdInited) {
        if(kbdShown) {
            swkbdInlineUpdate(&kbd, NULL);
        } else if(SDL_TextInputActive(window)) {
            SDL_StopTextInput(window);
        }
    }
}

void
SWITCH_QuitSwkb()
{
    if(kbdInited) {
        swkbdInlineClose(&kbd);
        kbdInited = false;
    }
}

bool
SWITCH_HasScreenKeyboardSupport(SDL_VideoDevice *_this)
{
    return true;
}

bool
SWITCH_IsScreenKeyboardShown(SDL_VideoDevice *_this, SDL_Window *window)
{
    return kbdShown;
}

static void
SWITCH_EnterCb(const char *str, SwkbdDecidedEnterArg* arg)
{
    if(arg->stringLen > 0) {
        SDL_SendKeyboardText(str);
    }

    kbdShown = false;
}

static void
SWITCH_CancelCb()
{
    SDL_StopTextInput(SDL_GetKeyboardFocus());
}

bool
SWITCH_StartTextInput(SDL_VideoDevice *_this, SDL_Window *window, SDL_PropertiesID props)
{
    Result rc;

    if(!kbdInited) {
        rc = swkbdInlineCreate(&kbd);
        if (R_SUCCEEDED(rc)) {
            rc = swkbdInlineLaunchForLibraryApplet(&kbd, SwkbdInlineMode_AppletDisplay, 0);
            if(R_SUCCEEDED(rc)) {
                swkbdInlineSetDecidedEnterCallback(&kbd, SWITCH_EnterCb);
                swkbdInlineSetDecidedCancelCallback(&kbd, SWITCH_CancelCb);
                swkbdInlineMakeAppearArg(&kbdAppearArg, SwkbdType_Normal);
                swkbdInlineAppearArgSetOkButtonText(&kbdAppearArg, "Submit");
                kbdAppearArg.dicFlag = 1;
                kbdAppearArg.returnButtonFlag = 1;
                kbdInited = true;
            } else {
                return SDL_SetError("Couldn't create the software keyboard: 0x%x", rc);
            }
        } else {
            return SDL_SetError("Couldn't launch the software keyboard: 0x%x", rc);
        }
    }

    if(kbdInited) {
        swkbdInlineSetInputText(&kbd, "");
        swkbdInlineSetCursorPos(&kbd, 0);
        swkbdInlineUpdate(&kbd, NULL);
        swkbdInlineAppear(&kbd, &kbdAppearArg);
        kbdShown = true;
    }

    return true;
}

bool
SWITCH_StopTextInput(SDL_VideoDevice *_this, SDL_Window *window)
{
    if(kbdInited) {
        swkbdInlineDisappear(&kbd);
    }

    kbdShown = false;
    return true;
}

#endif
