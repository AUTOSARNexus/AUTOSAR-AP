#ifndef VITO_AP_FUTURE_H_
#define VITO_AP_FUTURE_H_

#include <chrono>
#include <cstdint>
#include <future>

#include "ara/core/error_code.h"
#include "ara/core/future_error_domain.h"
#include "ara/core/result.h"

namespace ara::core {
/// @brief Specifies the state of a Future as returned by wait_for() and wait_until().
/// These definitions are equivalent to the ones from std::future_status. However, no item equivalent to
/// std::future_status::deferred is available here.
/// The numerical values of the enum items are implementation-defined.
// ReSharper disable once CppInconsistentNaming
enum class future_status : std::uint8_t {
  /// @brief the shared state is ready
  // ReSharper disable once CppInconsistentNaming
  ready,
  /// @brief the shared state did not become ready before the specified timeout has passed
  // ReSharper disable once CppInconsistentNaming
  timeout,
};

/// @brief Provides ara::core specific Future operations to collect the results of an asynchronous call.
/// @tparam T the type of values
/// @tparam E the type of errors
template <typename T, typename E = ErrorCode>
class Future final {
 public:
  /// @brief Default constructor.
  /// This function shall behave the same as the corresponding std::future function.
  Future() noexcept = default;

  /// @brief Copy constructor shall be disabled.
  Future(const Future&) = delete;

  /// @brief Move construct from another instance.
  /// This function shall behave the same as the corresponding std::future function.
  /// @param other the other instance
  Future(Future&& other) noexcept { std::swap(other.future_, future_); }

  /// @brief Destructor for Future objects.
  /// This function shall behave the same as the corresponding std::future function.
  ~Future() noexcept = default;

  /// @brief Copy assignment operator shall be disabled.
  Future& operator=(const Future&) = delete;

  /// @brief Move assign from another instance.
  /// This function shall behave the same as the corresponding std::future function.
  /// @param other the other instance
  /// @return *this
  Future& operator=(Future&& other) noexcept { future_ = std::move(other.future_); }

  /// @brief Get the value.
  /// This function shall behave the same as the corresponding std::future function.
  /// This function does not participate in overload resolution when the compiler toolchain does not support C++
  /// exceptions.
  /// @return value of type T
  T get() { return future_.get(); }

  /// @brief Get the result.
  /// @return a Result with either a value or an error
  Result<T, E> GetResult() noexcept {
    using R = Result<T, E>;
    try {
      return R{future_.get()};
    } catch (const std::future_error& ex) {
      switch (const auto error_code{static_cast<std::future_errc>(ex.code().value())}) {
        case std::future_errc::broken_promise:
          return R::FromError(future_errc::broken_promise);
        case std::future_errc::future_already_retrieved:
          return R::FromError(future_errc::future_already_retrieved);
        case std::future_errc::no_state:
          return R::FromError(future_errc::no_state);
        case std::future_errc::promise_already_satisfied:
          return R::FromError(future_errc::promise_already_satisfied);
        default:
          return R::FromError(future_errc::kInvalidArgument);
      }
    }
  }

  /// @brief Checks if the Future is valid, i.e. if it has a shared state.
  /// This function shall behave the same as the corresponding std::future function.
  /// @return true if the Future is usable, false otherwise
  bool valid() const noexcept;

  /// @brief Wait for a value or an error to be available.
  /// This function shall behave the same as the corresponding std::future function.
  void wait() const;

  /// @brief Wait for the given period, or until a value or an error is available.
  /// @tparam Rep
  /// @tparam Period
  /// @param timeout_duration maximal duration to wait for
  /// @return status that indicates whether the timeout hit or if a value is available
  template <typename Rep, typename Period>
  future_status wait_for(const std::chrono::duration<Rep, Period>& timeout_duration) const;

  /// @brief Wait until the given time, or until a value or an error is available.
  /// This function shall behave the same as the corresponding std::future function.
  /// @tparam Clock
  /// @tparam Duration
  /// @param deadline latest point in time to wait
  /// @return status that indicates whether the time was reached or if a value is available
  template <typename Clock, typename Duration>
  future_status wait_until(const std::chrono::time_point<Clock, Duration>& deadline) const;

  /// @brief Register a callable that gets called when the Future becomes ready.
  /// When func is called, it is guaranteed that get() and GetResult() will not block.
  /// func may be called in the context of this call or in the context of Promise::set_value() or Promise::SetError() or
  /// somewhere else.
  /// The return type of then depends on the return type of func (aka continuation). Let U be the return
  /// type of the continuation (i.e. a type equivalent to std::result_of_t<std::decay_t<F>(Future<T,E>)>). If U is
  /// Future<T2,E2> for some types T2, E2, then the return type of then() is Future<T2,E2>. This is known as implicit
  /// Future unwrapping. If U is Result<T2,E2> for some types T2, E2, then the return type of then() is Future<T2,E2>.
  /// This is known as implicit Result unwrapping. Otherwise it is Future<U,E>.
  /// @tparam F the type of the func argument
  /// @param func a callable to register
  /// @return a new Future instance for the result of the continuation.
  template <typename F>
  auto then(F&& func);

  /// @brief Register a callable that gets called when the Future becomes ready
  /// When func is called, it is guaranteed that get() and GetResult() will not block.
  /// func is called in the context of the provided execution context executor.
  /// The return type of depends on the return type of func (aka continuation).
  /// Let U be the return type of the continuation (i.e. a type equivalent to
  /// std::result_of_t<std::decay_t<F>(Future<T,E>)>). If U is Future<T2,E2> for some types T2, E2, then the return type
  /// of then() is Future<T2,E2>. This is known as implicit Future unwrapping. If U is Result<T2,E2> for some types T2,
  /// E2, then the return type of then() is Future<T2,E2>. This is known as implicit Result unwrapping. Otherwise it is
  /// Future<U,E>.
  /// @tparam F the type of the func argument
  /// @tparam ExecutorT the type of the executor argument
  /// @param func a callable to register
  /// @param executor the execution context in which to execute the Callable func
  /// @return a new Future instance for the result of the continuation
  template <typename F, typename ExecutorT>
  auto then(F&& func, ExecutorT&& executor);

  /// @brief Return whether the asynchronous operation has finished.
  /// If this function returns true, get(), GetResult() and the wait calls are guaranteed not to block. The behavior of
  /// this function is undefined if valid() returns false.
  /// @return true if the Future contains a value or an error, false otherwise.
  bool is_ready() const;

 private:
  std::future<T> future_;
};

/// @brief Specialization of class Future for "void" values.
/// @tparam E the type of error
template <typename E>
class Future<void, E> final {
 public:
  /// @brief Default constructor.
  /// This function shall behave the same as the corresponding std::future function.
  Future() noexcept;
};
}  // namespace ara::core

#endif