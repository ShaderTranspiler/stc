#ifdef STC_PROFILING
    #include <tracy/Tracy.hpp>
#else
    #define ZoneScopedN(name)
    #define ZoneText(text, size)
    #define ZoneScoped
#endif
