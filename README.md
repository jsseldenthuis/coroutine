# coroutine

This is a header-only stackless coroutine implementation in standard C99 and
C++11. [coroutine.h] contains a C adaptation of the C++ stackless coroutine from
<a href="https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/coroutine.html">Boost.Asio</a>
by Christopher M. Kohlhoff. This is itself a variant of Simon Tatham's
<a href="https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html">Coroutines in C</a>,
which was inspired by
<a href="http://www.lysator.liu.se/c/duffs-device.html">Duff's device</a>. The
API is designed to be a more powerfull version of
<a href="http://dunkels.com/adam/pt/">Protothreads</a> with a more natural
syntax.

The implementation (ab)uses the switch statement. It is therefore not
possible to yield from a coroutine from within a nested switch statement.

Since the implementation is stackless, variables local to the coroutine are
not stored between invocations. In C++, this drawback can be partially
mitigated by implementing the coroutine as a function object (see
[coroutine.hpp]) and making all local variables (private) data members.

## API

* `co_reenter(ctx)`:
  Defines the body of a stackless coroutine. When the body is executed at
  runtime, control jumps to the location immediately following the last
  `co_yield` or `co_fork` statement.

  Note that a function MUST NOT contain multiple `co_reenter` expressions with
  the same context.

* `co_yield <expression>`:
  Stores the context of the current stackless coroutine, evaluates the
  expression following the `co_yield` keyword, if any, and exits the scope of
  the #co_reenter statement. `co_yield break` terminates the coroutine.
  `co_yield continue` is equivalent to `co_yield`.

  A `co_yield` expression is valid only within a `co_reenter` statement. Since
  `co_reenter` is implemented using a `switch` statement, `co_yield` CANNOT be
  used from within a nested `switch` statement.

* `co_fork <expression>`:
  "Forks" a coroutine and executes the expression following the `co_fork`
  keyword as a child. This expression will typically create a copy of the
  coroutine context. After the expression completes, the coroutine continues and
  as a parent. If the coroutine is reentered with (the copy of) the context
  created by `co_fork`, the coroutine continues as a child until the next
  `co_yield` statement.

  See [coroutine.h] for an example of `co_fork`.

## C99 example

```c
#include "coroutine.h"

void my_coroutine(co_ctx_t* ctx) {
        ...
        // statements executed on every invocation of my_coroutine()
        ...
        co_reenter (ctx) {
                assert(!co_is_ready(ctx));
                ...
                // statements executed on the first invocation of
                // my_coroutine()
                ...
                // Store the context and exit the scope of the co_reenter
                // statement.
                co_yield;
                ...
                // statements executed on the second invocation of
                // my_coroutine()
                ...
                co_yield;
                ...
                // statements executed on the third invocation of
                // my_coroutine()
                ...
        }
        ...
        // statements executed on every invocation of my_coroutine() (unless the
        // function returns early)
        ...
}
```

## C++11 example

```cpp
#include "coroutine.hpp"

class MyCoroutine : public Coroutine {
 public:
  void operator()() {
    ...
    // statements executed on every invocation
    ...
    co_reenter (this) {
      ...
      // statements executed on the first invocation
      ...
      // Store the context and exit the scope of the co_reenter statement.
      co_yield;
      ...
      // statements executed on the second invocation
      ...
      co_yield;
      ...
      // statements executed on the third invocation
      ...
    }
    ...
    // statements executed on every invocation (unless this function returns
    // early)
    ...
  }
};
```

[coroutine.h]: coroutine.h
[coroutine.hpp]: coroutine.hpp
