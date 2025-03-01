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

#include "../SDL_syslocale.h"
#include "SDL_internal.h"

#include <switch.h>

static bool get_system_language(SetLanguage *language)
{
    Result rc = setInitialize();
    if (R_SUCCEEDED(rc)) {
        u64 language_code;
        if (R_SUCCEEDED(rc = setGetSystemLanguage(&language_code))) {
            rc = setMakeLanguage(language_code, language);
        }
        setExit();
    }

    return R_SUCCEEDED(rc);
}

bool SDL_SYS_GetPreferredLocales(char *buf, size_t buflen)
{
    static const char *LANG_TO_LOCALE[] = { "ja",    "en_US", "fr",    "de",    "it",
                                            "es",    "zh_CN", "ko",    "nl",    "pt",
                                            "ru",    "zh_TW", "en_GB", "fr_CA", "es",
                                            "zh",    "zh",    "pt_BR" };

    SetLanguage language;
    if (!get_system_language(&language) ||
        language >= sizeof(LANG_TO_LOCALE) / sizeof(*LANG_TO_LOCALE)) {
        return SDL_SetError("Couldn't get the system's language.");
    }

    SDL_strlcpy(buf, LANG_TO_LOCALE[language], buflen);
    return true;
}
