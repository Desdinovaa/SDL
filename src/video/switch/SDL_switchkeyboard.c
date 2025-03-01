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
#include "../../SDL_internal.h"

#ifdef SDL_VIDEO_DRIVER_SWITCH

#include <switch.h>

#include "SDL_switchkeyboard.h"
#include "../../events/SDL_keyboard_c.h"

static bool keys[SDL_SCANCODE_COUNT] = {0};

void
SWITCH_InitKeyboard(void) {
    hidInitializeKeyboard();
}

void
SWITCH_PollKeyboard(void) {
    HidKeyboardState state;
    SDL_Scancode scancode;

    if (!SDL_GetKeyboardFocus()) {
        return;
    }

    if (hidGetKeyboardStates(&state, 1)) {
        for (scancode = SDL_SCANCODE_UNKNOWN; scancode < (SDL_Scancode) HidKeyboardKey_RightGui; scancode++) {
            bool pressed = hidKeyboardStateGetKey(&state, (int) scancode);
            if (pressed && !keys[scancode]) {
                SDL_SendKeyboardKey(SDL_GetTicksNS(), 0, scancode, scancode, pressed);
                keys[scancode] = true;
            } else if (!pressed && keys[scancode]) {
                SDL_SendKeyboardKey(SDL_GetTicksNS(), 0, scancode, scancode, pressed);
                keys[scancode] = false;
            }
        }
    }
}

void
SWITCH_QuitKeyboard(void) {
}

#endif /* SDL_VIDEO_DRIVER_SWITCH */
