
#pragma once

#include <cassert>
#include <string>

#ifndef NDEBUG
#   define assert2(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (false)
#else
#   define assert2(condition, message) do { } while (false)
#endif
