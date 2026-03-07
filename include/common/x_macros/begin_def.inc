#ifndef X
    #error "An X macro needs to be defined before including a .def file"
#endif

#if defined(STC_X) && !defined(STC_X_NO_FIRST) && !defined(STC_X_NO_LAST)
    #error "Probable missing #undef X from previous X-Macro usage"
#endif

#ifndef STC_X
    #define STC_X(node, kind) X(node, kind)
#endif

#if defined(X_FIRST) && !defined(STC_X_NO_FIRST)
    #define STC_X_FIRST(node, kind) X_FIRST(node, kind)
#else
    #define STC_X_FIRST(node, kind) STC_X(node, kind)
#endif

#if defined(X_LAST) && !defined(STC_X_NO_LAST)
    #define STC_X_LAST(node, kind) X_LAST(node, kind)
#else
    #define STC_X_LAST(node, kind) STC_X(node, kind)
#endif
