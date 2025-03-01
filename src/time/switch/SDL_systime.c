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

#ifdef SDL_TIME_SWITCH

#include "../SDL_time_c.h"
#include <errno.h>
#include <switch.h>
#include <time.h>


void SDL_GetSystemTimeLocalePreferences(SDL_DateFormat *df, SDL_TimeFormat *tf)
{
    // As of system version 19.0.1, the Switch supports only supports those 18 languages
    static const SDL_DateFormat LANG_TO_DATE_FORMAT[] = {
        SDL_DATE_FORMAT_YYYYMMDD, // ja
        SDL_DATE_FORMAT_MMDDYYYY, // en-US
        SDL_DATE_FORMAT_DDMMYYYY, // fr
        SDL_DATE_FORMAT_DDMMYYYY, // de
        SDL_DATE_FORMAT_DDMMYYYY, // it
        SDL_DATE_FORMAT_DDMMYYYY, // es
        SDL_DATE_FORMAT_YYYYMMDD, // zh-CN
        SDL_DATE_FORMAT_YYYYMMDD, // ko
        SDL_DATE_FORMAT_DDMMYYYY, // nl
        SDL_DATE_FORMAT_DDMMYYYY, // pt
        SDL_DATE_FORMAT_DDMMYYYY, // ru
        SDL_DATE_FORMAT_YYYYMMDD, // zh-TW
        SDL_DATE_FORMAT_DDMMYYYY, // en-GB
        SDL_DATE_FORMAT_DDMMYYYY, // fr-CA
        SDL_DATE_FORMAT_DDMMYYYY, // es-419
        SDL_DATE_FORMAT_YYYYMMDD, // zh-Hans
        SDL_DATE_FORMAT_YYYYMMDD, // zh-Hant
        SDL_DATE_FORMAT_DDMMYYYY, // pt-BR
    };

    if (R_FAILED(setInitialize())) {
        return;
    }

    u64 language_code;
    if (R_FAILED(setGetSystemLanguage(&language_code))) {
        setExit();
        return;
    }

    SetLanguage language;
    Result rc = setMakeLanguage(language_code, &language);
    setExit();
    if (R_FAILED(rc)) {
        return;
    }

    if (language >= sizeof(LANG_TO_DATE_FORMAT) / sizeof(*LANG_TO_DATE_FORMAT)) {
        return;
    }

    if (df) {
        *df = LANG_TO_DATE_FORMAT[language];
    }
    if (tf) {
        // en-US is the only supported language that uses the 12hr system
        if (language_code == SetLanguage_ENUS) {
            *tf = SDL_TIME_FORMAT_12HR;
        } else {
            *tf = SDL_TIME_FORMAT_24HR;
        }
    }
}

bool SDL_GetCurrentTime(SDL_Time *ticks)
{
    if (!ticks) {
        return SDL_InvalidParamError("ticks");
    }

    struct timespec tp;
    if (clock_gettime(CLOCK_REALTIME, &tp) == 0) {
        *ticks = SDL_SECONDS_TO_NS(tp.tv_sec) + tp.tv_nsec;
        return true;
    }

    SDL_SetError("Failed to retrieve system time (%i)", errno);
    return false;
}

bool SDL_TimeToDateTime(SDL_Time ticks, SDL_DateTime *dt, bool localTime)
{
    if (!dt) {
        return SDL_InvalidParamError("dt");
    }

    const time_t tval = (time_t)SDL_NS_TO_SECONDS(ticks);
    struct tm tm;
    if (localTime) {
        localtime_r(&tval, &tm);
        dt->utc_offset = -_timezone;
    } else {
        gmtime_r(&tval, &tm);
        dt->utc_offset = 0;
    }

    dt->year = tm.tm_year + 1900;
    dt->month = tm.tm_mon + 1;
    dt->day = tm.tm_mday;
    dt->hour = tm.tm_hour;
    dt->minute = tm.tm_min;
    dt->second = tm.tm_sec;
    dt->nanosecond = ticks & SDL_NS_PER_SECOND;
    dt->day_of_week = tm.tm_wday;

    return true;
}

#endif // SDL_TIME_SWITCH
