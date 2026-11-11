#pragma once

#ifndef STC_API
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef STC_BUILDING_DLL
            #define STC_API __declspec(dllexport)
        #else
            #define STC_API __declspec(dllimport)
        #endif
    #else
        #if defined(__GNUC__) || defined(__clang__)
            #define STC_API __attribute__((visibility("default")))
        #else
            #define STC_API
        #endif
    #endif
#endif

// only has to be used in the API part, in headers which might be included in either C or C++
// everywhere else a regular noexcept works
#ifdef __cplusplus
    #define STC_NOEXCEPT noexcept
#else
    #define STC_NOEXCEPT
#endif
