#pragma once

#if defined(min) || defined(max)
static_assert(false, "min or max macro defined before windows.h include");
#endif

#ifndef NOMINMAX
    #define NOMINMAX
    #define STC_DEFINED_NOMINMAX
#endif

#include <io.h>
#include <windows.h>

#ifdef STC_DEFINED_NOMINMAX
    #undef NOMINMAX
    #undef STC_DEFINED_NOMINMAX
#endif

#if defined(min) || defined(max)
static_assert(false, "min or max macro defined after windows.h include");
#endif
