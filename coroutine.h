/**@file
 * This header file contains a stackless coroutine implementation. The
 * implementation is pure and compliant C99 and does not require any part of the
 * standard library. It is an adaptation of the C++ stackless coroutine from
 * <a href="https://www.boost.org/doc/libs/release/doc/html/boost_asio/reference/coroutine.html">Boost.Asio</a>
 * by Christopher M. Kohlhoff. This is itself a variant of Simon Tatham's
 * <a href="https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html">Coroutines in C</a>,
 * which was inspired by
 * <a href="http://www.lysator.liu.se/c/duffs-device.html">Duff's device</a>.
 *
 * The implementation (ab)uses the switch statement. It is therefore not
 * possible to yield from a coroutine from within a nested switch statement.
 *
 * Since the implementation is stackless, variables local to the coroutine are
 * not stored between invocations. In C++, this drawback can be partially
 * mitigated by implementing the coroutine as a function object and making all
 * local variables (private) data members.
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

#ifndef COROUTINE_H_
#define COROUTINE_H_

#ifndef CO_LABEL_TYPE
/**
 * The type used to store the label (i.e., program counter) of a stackless
 * coroutine. A smaller type may be used in constrained environments, especially
 * if the `__COUNTER__` macro is available.
 */
#define CO_LABEL_TYPE int
#endif

/**
 * The type holding the context (i.e., program counter) of a stackless
 * coroutine.
 */
typedef struct {
	CO_LABEL_TYPE label;
} co_ctx_t;

/// The static initializer for #co_ctx_t.
#define CO_CTX_INIT \
	{ \
		0 \
	}

/// Resets a stackless coroutine so the next invocation starts at the beginning.
#define co_restart(ctx) co_restart__(ctx)
#define co_restart__(ctx) ((void)((ctx)->label = 0))

/// Returns 1 if the stackless coroutine has finished.
#define co_is_ready(ctx) co_is_ready__(ctx)
#define co_is_ready__(ctx) ((ctx)->label == -1)

/**
 * Defines the body of a stackless coroutine. When the body is executed at
 * runtime, control jumps to the location immediately following the last
 * #co_yield or #co_fork statement.
 *
 * Note that a function MUST NOT contain multiple `co_reenter` expressions with
 * the same context.
 *
 * ```{.c}
 * void my_coroutine(co_ctx_t* ctx) {
 *         ...
 *         // statements executed on every invocation of my_coroutine()
 *         ...
 *         co_reenter (ctx) {
 *                 assert(!co_is_ready(ctx));
 *                 ...
 *                 // statements executed on the first invocation of
 *                 // my_coroutine()
 *                 ...
 *                 // Store the context and exit the scope of the co_reenter
 *                 // statement.
 *                 co_yield;
 *                 ...
 *                 // statements executed on the second invocation of
 *                 // my_coroutine()
 *                 ...
 *                 co_yield;
 *                 ...
 *                 // statements executed on the second invocation of
 *                 // my_coroutine()
 *                 ...
 *         }
 *         ...
 *         // statements executed on every invocation of my_coroutine() (unless
 *         // the function returns early)
 *         ...
 * }
 * ```
 */
#define co_reenter(ctx) co_reenter__(ctx)
#define co_reenter__(ctx) \
	for (CO_LABEL_TYPE *const _co_label_ = &(ctx)->label; \
			*_co_label_ != -1; *_co_label_ = -1) \
		if (0) { \
			goto _co_continue_; \
		_co_continue_: \
			continue; \
		} else if (0) { \
			goto _co_break_; \
		_co_break_: \
			break; \
		} else \
			switch (*_co_label_) \
			case 0:

#ifdef __COUNTER__
#define co_label__ (__COUNTER__ + 1)
#else
#define co_label__ __LINE__
#endif

/**
 * Stores the context of the current stackless coroutine, evaluates the
 * expression following the `co_yield` keyword, if any, and exits the scope of
 * the #co_reenter statement. `co_yield break` terminates the coroutine (i.e.,
 * after this statement, co_is_ready() returns 1). `co_yield continue` is
 * equivalent to `co_yield`.
 *
 * A `co_yield` expression is valid only within a #co_reenter statement. Since
 * #co_reenter is implemented using a `switch` statement, `co_yield` CANNOT be
 * used from within a nested `switch` statement.
 */
#define co_yield co_yield__(co_label__)
// clang-format off
#define co_yield__(label) \
	for (*_co_label_ = (label);;) \
		if (0) { \
			case (label): \
				break; \
		} else \
			switch (0) \
				for (;;) \
					if (1) \
						goto _co_continue_; \
					else \
						for (;;) \
							if (1) \
								goto _co_break_; \
							else /* falls through */ \
							case 0:
// clang-format on

/**
 * "Forks" a coroutine and executes the expression following the `co_fork`
 * keyword as a child (i.e., co_is_child() returns 1). This expression will
 * typically create a copy of the coroutine context. After the expression
 * completes, the coroutine continues and co_is_parent() returns 1. If the
 * coroutine is reentered with (the copy of) the context created by `co_fork`,
 * co_is_child() returns 1 until the next `co_yield` statement.
 *
 * For example:
 * ```{.c}
 * co_reenter (ctx) {
 *         do {
 *               ...
 *               // statements executed by parent
 *               ...
 *               co_fork create_copy_and_do_something(ctx);
 *         } while (co_is_parent());
 *         // This is where co_reenter resumes with the context from
 *         // create_copy_and_do_something().
 *         assert(co_is_child());
 *         ...
 *         // statements executed by child
 *         ...
 *         co_yield;
 *         // After a co_yield statement a coroutine is always considered a
 *         // parent.
 *         assert(co_is_parent());
 * }
 * ```
 *
 * This works especially well with function objects in C++. The canonical use
 * case is a server accepting and handling connections:
 * ```{.cpp}
 * class Server : public Coroutine {
 *  public:
 *   void operator()(int s) {
 *     co_reenter (this) {
 *       do {
 *         // Perform an asynchronous accept on a socket. When a connection is
 *         // accepted, async_accept() invokes this function object with the
 *         // accepted socket.
 *         co_yield async_accept(socket_, *this);
 *         // Create a copy of the current coroutine function object to handle
 *         // the connection, while the parent accepts the next incoming
 *         // connection.
 *         co_fork Server(*this)(s);
 *       } while (is_parent());
 *       // Handle the connection.
 *       co_yield async_read(s, ..., *this);
 *     }
 *   }
 *  private:
 *   // The socket used to listen for incoming connections.
 *   int socket_;
 * };
 * ```
 */
#define co_fork co_fork__(co_label__)
#define co_fork__(label) \
	for (*_co_label_ = -(label)-1;; *_co_label_ = (label)) \
		if (_co_label_ == (label)) { \
		case -(label)-1: break; \
		} else

/// Returns 1 if the calling stackless coroutine is the parent of a fork.
#define co_is_parent() (!co_is_child())

/// Returns 1 if the calling stackless coroutine is the child of a fork.
#define co_is_child() (*_co_label_ < -1)

#endif // !COROUTINE_H_
