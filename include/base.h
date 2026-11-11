#pragma once

// NOTE: this should remain C compatible, as it is included in the C API headers, which are intended
// to be includable (not buildable!) in a C codebase

#ifndef STC_API
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef STC_BUILDING_DLL
            #define STC_API __declspec(dllexport)
        #else
            #define STC_API __declspec(dllimport)
        #endif
    #else
        #if defined(__GNUC__) || defined(__clang__)
            #define STC_API __attribute__((visibility("default"), used))
        #else
            #define STC_API
        #endif
    #endif
#endif

#ifdef _MSC_VER
    #define STC_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define STC_FORCE_INLINE inline
#endif

#ifdef _MSC_VER
    #define STC_PRETTY_FUNC __FUNCSIG__
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_PRETTY_FUNC __PRETTY_FUNCTION__
#else
    #define STC_PRETTY_FUNC ""
#endif

// only has to be used in the API part, in headers which might be included in either C or C++
// everywhere else a regular noexcept works
#ifdef __cplusplus
    #define STC_NOEXCEPT noexcept
#else
    #define STC_NOEXCEPT
#endif
