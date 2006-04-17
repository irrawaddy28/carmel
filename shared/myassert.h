//workaround for difficulties I've had getting a debugger breakpoint from a failed assert
#ifndef ASSERT_H
#define ASSERT_H

#include <graehl/shared/breakpoint.hpp>
#include <graehl/shared/config.h>
#include <cassert>


inline static void _my_assert(const char *file,unsigned line,const char *expr)
{
  Config::err() << file << "(" << line << ") Assertion failed: " << expr << std::endl;
  BREAKPOINT;
}

template <class T>
inline static void _my_assert(const char *file,unsigned line,const T&t,const char *expr,const char *expect)
{
  Config::err() << file << "(" << line << ") Assertion failed: (" << expr << ") was " << t << "; expected " << expect << std::endl;
  BREAKPOINT;
}

#ifdef DEBUG
# define Assert(expr) (expr) ? (void)0 :   \
                 _my_assert(__FILE__,__LINE__,#expr)
// WARNING: expr occurs twice (repeated computation)
# define Assert2(expr,expect) do {                                                        \
        /* Config::log() << #expr << ' ' << #expect << " = " << (expr expect) << std::endl;*/   \
        if (!((expr) expect)) _my_assert(__FILE__,__LINE__,expr,#expr,#expect);                 \
            } while(0)
# define Paranoid(a) do { a; } while (0)
#else
# define Assert(a)
# define Assert2(a,b)
# define Paranoid(a)
#endif

#endif
