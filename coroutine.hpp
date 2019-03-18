/**@file
 * This header file contains the C++ implementation of stackless coroutines.
 *
 * @see coroutine.h
 *
 * @copyright 2018-2019 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef COROUTINE_HPP_
#define COROUTINE_HPP_

#include "coroutine.h"

#undef co_restart
#define co_restart(ctx) co_restart__(to_co_ctx(ctx))

#undef co_is_ready
#define co_is_ready(ctx) co_is_ready__(to_co_ctx(ctx))

#undef co_reenter
#define co_reenter(ctx) co_reenter__(to_co_ctx(ctx))

/**
 * The parent class for function objects used as stackless coroutines. Derived
 * classes use `co_reenter (this) { ... }` in their implementation of
 * `operator()` to define the body of the coroutine. Note that local variables
 * are not stored between invocations. It is recommended to use data members for
 * variables that need to be restored.
 */
class Coroutine {
 public:
  /**
   * Resets the stackless coroutine so the next invocation starts at the
   * beginning.
   */
  void restart() noexcept { co_restart(this); }

  /// Returns true if the stackless coroutine has finished.
  bool is_ready() const noexcept { return ctx_.label == -1; }

  /// Returns true if the stackless coroutine is the parent of a fork.
  bool is_parent() const noexcept { return !is_child(); }

  /// Returns true if the stackless coroutine is the child of a fork.
  bool is_child() const noexcept { return ctx_.label < -1; }

 private:
  co_ctx_t ctx_ CO_CTX_INIT;

  friend co_ctx_t* to_co_ctx(Coroutine* coro) noexcept { return &coro->ctx_; }
  friend co_ctx_t* to_co_ctx(Coroutine& coro) noexcept { return &coro.ctx_; }
};

inline co_ctx_t* to_co_ctx(co_ctx_t* ctx) noexcept { return ctx; }

#endif  // !COROUTINE_HPP_
