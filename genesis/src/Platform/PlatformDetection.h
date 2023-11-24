#pragma once

#if defined(__linux__) || defined(__gnu_linux__)
    #define GN_PLATFORM_LINUX
    #if defined(__ANDROID__)
        #define GN_PLATFORM_ANDROID
    #endif
#elif defined(__unix__)
    #define GN_PLATFORM_UNIX
#elif defined(__APPLE__)
    #define GN_PLATFORM_APPLE
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR
        #define GN_PLATFORM_IOS_SIM
    #elif TARGET_OS_IPHONE
        #define GN_PLATFORM_IOS
    #elif TARGET_OS_MAC
        #define GN_PLATFORM_MACOS
    #else
        #error "Unknown Apple Platform"
    #endif
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define GN_PLATFORM_WINDOWS
#else
    #error "Unknown Platform"
#endif
