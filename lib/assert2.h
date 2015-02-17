
#pragma once

#include <cassert>
#include <string>


#if __GNUC__
#define assert2(expr, str) \
  ((expr)                  \
   ? __ASSERT_VOID_CAST (0)\
   : __assert_fail ((std::string(__STRING(expr)) + "; " + (str)).c_str(), __FILE__, __LINE__, __ASSERT_FUNCTION))
#elif __clang__
#define assert2(expr, str) \
  ((expr)                  \
   ? (void)(0)\
   : __assert_rtn ((std::string(__STRING(expr)) + "; " + (str)).c_str(), __FILE__, __LINE__, __func__))
#else
#define assert2(expr, str) assert(expr)
#endif
