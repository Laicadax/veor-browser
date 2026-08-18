// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// Result<T, E>
// ─────────────────────────────────────────────────────────────────────────────
// A monadic, exception-free result type. Modeled after Rust's Result<T, E>.
//
// Usage:
//   Result<int, std::string> r = Result<int, std::string>::Ok(42);
//   if (r.IsOk()) { int v = r.Unwrap(); }
//
//   auto mapped = r.Map([](int x) { return x * 2; });
//   auto chained = r.AndThen([](int x) -> Result<float, std::string> {
//     return x > 0 ? Result<float, std::string>::Ok(3.14f)
//                  : Result<float, std::string>::Err("non-positive");
//   });
//
// Thread safety: Value semantics. Safe to copy/move across threads.
// No heap allocation (unless T or E allocate).
// ─────────────────────────────────────────────────────────────────────────────

template <typename T, typename E>
class Result {
 public:
  using ValueType = T;
  using ErrorType = E;

  // ── Constructors ──
  static Result Ok(T value) {
    return Result(std::in_place_index<0>, std::move(value));
  }

  static Result Err(E error) {
    return Result(std::in_place_index<1>, std::move(error));
  }

  // Default constructor creates an error with default-constructed E.
  // This is required for some std::container operations.
  Result() : Result(std::in_place_index<1>, E{}) {}

  // Copy / Move
  Result(const Result&) = default;
  Result& operator=(const Result&) = default;
  Result(Result&&) noexcept = default;
  Result& operator=(Result&&) noexcept = default;

  // ── Queries ──
  bool IsOk() const noexcept { return value_.index() == 0; }
  bool IsErr() const noexcept { return value_.index() == 1; }

  explicit operator bool() const noexcept { return IsOk(); }

  // ── Direct access (unchecked) ──
  // PRECONDITION: IsOk() == true. UB otherwise. Debug builds: DCHECK.
  T& Unwrap() & {
    // In debug builds, assert IsOk()
    return std::get<0>(value_);
  }

  const T& Unwrap() const& {
    return std::get<0>(value_);
  }

  T&& Unwrap() && {
    return std::get<0>(std::move(value_));
  }

  // PRECONDITION: IsErr() == true. UB otherwise.
  E& UnwrapErr() & {
    return std::get<1>(value_);
  }

  const E& UnwrapErr() const& {
    return std::get<1>(value_);
  }

  E&& UnwrapErr() && {
    return std::get<1>(std::move(value_));
  }

  // ── Safe access ──
  const T* OkOrNull() const noexcept {
    return IsOk() ? &std::get<0>(value_) : nullptr;
  }

  T* OkOrNull() noexcept {
    return IsOk() ? &std::get<0>(value_) : nullptr;
  }

  const E* ErrOrNull() const noexcept {
    return IsErr() ? &std::get<1>(value_) : nullptr;
  }

  E* ErrOrNull() noexcept {
    return IsErr() ? &std::get<1>(value_) : nullptr;
  }

  // ── Default fallback ──
  T UnwrapOr(T default_value) const& {
    return IsOk() ? Unwrap() : std::move(default_value);
  }

  T UnwrapOr(T default_value) && {
    return IsOk() ? std::move(*this).Unwrap() : std::move(default_value);
  }

  template <typename F>
  T UnwrapOrElse(F&& fn) const& {
    return IsOk() ? Unwrap() : std::forward<F>(fn)();
  }

  template <typename F>
  T UnwrapOrElse(F&& fn) && {
    return IsOk() ? std::move(*this).Unwrap() : std::forward<F>(fn)();
  }

  // ── Monadic operations ──

  // Map: transforms T -> U if Ok, propagates Err.
  template <typename F>
  auto Map(F&& fn) & {
    using U = std::invoke_result_t<F, T&>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)(Unwrap()));
    }
    return Result<U, E>::Err(UnwrapErr());
  }

  template <typename F>
  auto Map(F&& fn) const& {
    using U = std::invoke_result_t<F, const T&>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)(Unwrap()));
    }
    return Result<U, E>::Err(UnwrapErr());
  }

  template <typename F>
  auto Map(F&& fn) && {
    using U = std::invoke_result_t<F, T&&>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)(std::move(*this).Unwrap()));
    }
    return Result<U, E>::Err(std::move(*this).UnwrapErr());
  }

  // MapErr: transforms E -> F if Err, propagates Ok.
  template <typename F>
  auto MapErr(F&& fn) & {
    using G = std::invoke_result_t<F, E&>;
    if (IsErr()) {
      return Result<T, G>::Err(std::forward<F>(fn)(UnwrapErr()));
    }
    return Result<T, G>::Ok(Unwrap());
  }

  template <typename F>
  auto MapErr(F&& fn) const& {
    using G = std::invoke_result_t<F, const E&>;
    if (IsErr()) {
      return Result<T, G>::Err(std::forward<F>(fn)(UnwrapErr()));
    }
    return Result<T, G>::Ok(Unwrap());
  }

  template <typename F>
  auto MapErr(F&& fn) && {
    using G = std::invoke_result_t<F, E&&>;
    if (IsErr()) {
      return Result<T, G>::Err(std::forward<F>(fn)(std::move(*this).UnwrapErr()));
    }
    return Result<T, G>::Ok(std::move(*this).Unwrap());
  }

  // AndThen: chains Results. F must return Result<U, E>.
  template <typename F>
  auto AndThen(F&& fn) & {
    using ResultU = std::invoke_result_t<F, T&>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)(Unwrap());
    }
    return ResultU::Err(UnwrapErr());
  }

  template <typename F>
  auto AndThen(F&& fn) const& {
    using ResultU = std::invoke_result_t<F, const T&>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)(Unwrap());
    }
    return ResultU::Err(UnwrapErr());
  }

  template <typename F>
  auto AndThen(F&& fn) && {
    using ResultU = std::invoke_result_t<F, T&&>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)(std::move(*this).Unwrap());
    }
    return ResultU::Err(std::move(*this).UnwrapErr());
  }

  // OrElse: provides alternative Result if Err.
  template <typename F>
  auto OrElse(F&& fn) & {
    using ResultT = std::invoke_result_t<F, E&>;
    static_assert(std::is_same_v<typename ResultT::ValueType, T>,
                  "OrElse must return Result with same value type");
    if (IsErr()) {
      return std::forward<F>(fn)(UnwrapErr());
    }
    return ResultT::Ok(Unwrap());
  }

  template <typename F>
  auto OrElse(F&& fn) const& {
    using ResultT = std::invoke_result_t<F, const E&>;
    static_assert(std::is_same_v<typename ResultT::ValueType, T>,
                  "OrElse must return Result with same value type");
    if (IsErr()) {
      return std::forward<F>(fn)(UnwrapErr());
    }
    return ResultT::Ok(Unwrap());
  }

  template <typename F>
  auto OrElse(F&& fn) && {
    using ResultT = std::invoke_result_t<F, E&&>;
    static_assert(std::is_same_v<typename ResultT::ValueType, T>,
                  "OrElse must return Result with same value type");
    if (IsErr()) {
      return std::forward<F>(fn)(std::move(*this).UnwrapErr());
    }
    return ResultT::Ok(std::move(*this).Unwrap());
  }

 private:
  std::variant<T, E> value_;

  template <std::size_t I, typename... Args>
  explicit Result(std::in_place_index_t<I> idx, Args&&... args)
      : value_(idx, std::forward<Args>(args)...) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Result<void, E> — Specialization for void value type
// ─────────────────────────────────────────────────────────────────────────────
// Uses std::optional<E> instead of std::variant<void, E> (which is ill-formed).

namespace detail {

// Empty tag type representing Ok state for void Results.
struct VoidOk {};

}  // namespace detail

template <typename E>
class Result<void, E> {
 public:
  using ValueType = void;
  using ErrorType = E;

  static Result Ok() {
    return Result(detail::VoidOk{});
  }

  static Result Err(E error) {
    return Result(std::move(error));
  }

  Result() : Result(E{}) {}

  Result(const Result&) = default;
  Result& operator=(const Result&) = default;
  Result(Result&&) noexcept = default;
  Result& operator=(Result&&) noexcept = default;

  bool IsOk() const noexcept { return !error_.has_value(); }
  bool IsErr() const noexcept { return error_.has_value(); }

  explicit operator bool() const noexcept { return IsOk(); }

  void Unwrap() const {
    // PRECONDITION: IsOk() == true.
  }

  E& UnwrapErr() & {
    return error_.Unwrap();
  }

  const E& UnwrapErr() const& {
    return error_.Unwrap();
  }

  E&& UnwrapErr() && {
    return std::move(error_.Unwrap());
  }

  const E* ErrOrNull() const noexcept {
    return error_ ? &*error_ : nullptr;
  }

  E* ErrOrNull() noexcept {
    return error_ ? &*error_ : nullptr;
  }

  template <typename F>
  auto Map(F&& fn) & {
    using U = std::invoke_result_t<F>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)());
    }
    return Result<U, E>::Err(UnwrapErr());
  }

  template <typename F>
  auto Map(F&& fn) const& {
    using U = std::invoke_result_t<F>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)());
    }
    return Result<U, E>::Err(UnwrapErr());
  }

  template <typename F>
  auto Map(F&& fn) && {
    using U = std::invoke_result_t<F>;
    if (IsOk()) {
      return Result<U, E>::Ok(std::forward<F>(fn)());
    }
    return Result<U, E>::Err(std::move(*this).UnwrapErr());
  }

  template <typename F>
  auto MapErr(F&& fn) & {
    using G = std::invoke_result_t<F, E&>;
    if (IsErr()) {
      return Result<void, G>::Err(std::forward<F>(fn)(UnwrapErr()));
    }
    return Result<void, G>::Ok();
  }

  template <typename F>
  auto MapErr(F&& fn) const& {
    using G = std::invoke_result_t<F, const E&>;
    if (IsErr()) {
      return Result<void, G>::Err(std::forward<F>(fn)(UnwrapErr()));
    }
    return Result<void, G>::Ok();
  }

  template <typename F>
  auto MapErr(F&& fn) && {
    using G = std::invoke_result_t<F, E&&>;
    if (IsErr()) {
      return Result<void, G>::Err(std::forward<F>(fn)(std::move(*this).UnwrapErr()));
    }
    return Result<void, G>::Ok();
  }

  template <typename F>
  auto AndThen(F&& fn) & {
    using ResultU = std::invoke_result_t<F>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)();
    }
    return ResultU::Err(UnwrapErr());
  }

  template <typename F>
  auto AndThen(F&& fn) const& {
    using ResultU = std::invoke_result_t<F>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)();
    }
    return ResultU::Err(UnwrapErr());
  }

  template <typename F>
  auto AndThen(F&& fn) && {
    using ResultU = std::invoke_result_t<F>;
    static_assert(std::is_same_v<typename ResultU::ErrorType, E>,
                  "AndThen requires the same error type");
    if (IsOk()) {
      return std::forward<F>(fn)();
    }
    return ResultU::Err(std::move(*this).UnwrapErr());
  }

  template <typename F>
  auto OrElse(F&& fn) & {
    using ResultT = std::invoke_result_t<F, E&>;
    static_assert(std::is_void_v<typename ResultT::ValueType>,
                  "OrElse must return Result<void, ...>");
    if (IsErr()) {
      return std::forward<F>(fn)(UnwrapErr());
    }
    return ResultT::Ok();
  }

  template <typename F>
  auto OrElse(F&& fn) const& {
    using ResultT = std::invoke_result_t<F, const E&>;
    static_assert(std::is_void_v<typename ResultT::ValueType>,
                  "OrElse must return Result<void, ...>");
    if (IsErr()) {
      return std::forward<F>(fn)(UnwrapErr());
    }
    return ResultT::Ok();
  }

  template <typename F>
  auto OrElse(F&& fn) && {
    using ResultT = std::invoke_result_t<F, E&&>;
    static_assert(std::is_void_v<typename ResultT::ValueType>,
                  "OrElse must return Result<void, ...>");
    if (IsErr()) {
      return std::forward<F>(fn)(std::move(*this).UnwrapErr());
    }
    return ResultT::Ok();
  }

 private:
  std::optional<E> error_;

  explicit Result(detail::VoidOk) {}
  explicit Result(E error) : error_(std::move(error)) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Common error types
// ─────────────────────────────────────────────────────────────────────────────

using StringError = std::string;

struct IOError {
  int code = 0;
  std::string message;
};

struct StorageError {
  int sqlite_code = 0;
  std::string message;
  std::string sql;


  std::string ToString() const {
    return "SQLite code=" + std::to_string(sqlite_code) + ", message=" + message + ", sql=" + sql;
  }
};


struct NetworkError {
  int net_error_code = 0;
  std::string message;
  bool is_certificate_error = false;
};

struct CryptoError {
  int code = 0;
  std::string message;
};

}  // namespace veor
