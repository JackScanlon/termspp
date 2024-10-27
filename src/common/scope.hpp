#pragma once

#include <utility>

namespace termspp {
namespace common {

/// Implementation of ScopedDeleter()
///  - used to manage a resource within a scope; executing the given
///    function when exiting the scope
///
template <typename T, typename Fn>
class ScopedDeleterImpl final {
public:
  explicit ScopedDeleterImpl(T *resource_, Fn fn_) : resource(resource_), fn(fn_) {};

  ~ScopedDeleterImpl() {
    if (resource) {
      fn(resource);
    }
  }

  // clang-format off
  ScopedDeleterImpl(ScopedDeleterImpl &&other)                       = delete;
  ScopedDeleterImpl(const ScopedDeleterImpl &other)                  = delete;
  auto operator=(const ScopedDeleterImpl &other)->ScopedDeleterImpl& = delete;
  // clang-format on

  auto release() -> T * {
    // clang-format off
    T *res = resource;
    // clang-format on
    resource = nullptr;
    return res;
  }

public:
  Fn fn{};
  T *resource{nullptr};
};

/// Management of a resource within a scope with a callback
///
/// Example:
/// ```cpp
///   auto cleanup =
///   termspp::common::ScopedDeleter(::fopen("some/path/to/file.txt"), [](FILE*
///   res) {
///     // e.g. close file handle ...
///     ::fclose(res);
///   });
/// ```
///
template <typename T, typename Fn>
[[nodiscard]] inline auto ScopedDeleter(T *resource_, Fn &&fn_) -> ScopedDeleterImpl<T, Fn> {
  return ScopedDeleterImpl<T, Fn>{resource_, fn_};
}

/// Implementation of OnScopeExit()
///  - used to execute a function when exiting the current scope
template <typename Fn>
class OnScopeExitImpl final {
public:
  explicit OnScopeExitImpl(Fn &&fn_) : fn(std::move(fn_)), active(true) {}

  ~OnScopeExitImpl() {
    if (active) {
      fn();
    }
  }

  OnScopeExitImpl(OnScopeExitImpl &&other) noexcept : fn(std::move(other.fn)), active(other.active) {
    other.active = false;
  }

  // clang-format off
  OnScopeExitImpl(const OnScopeExitImpl &other)                  = delete;
  auto operator=(const OnScopeExitImpl &other)->OnScopeExitImpl& = delete;
  // clang-format on

public:
  Fn   fn{};
  bool active{false};
};

/// RAII-like execution of a function when exiting the current scope
///
/// Example:
/// ```cpp
///   auto cleanup = termspp::common::OnScopeExit([&]() {
///     // ... clean up some resource ...
///   });
/// ```
///
template <typename Fn>
[[nodiscard]] inline auto OnScopeExit(Fn &&fn_) -> OnScopeExitImpl<Fn> {
  return OnScopeExitImpl<Fn>{fn_};
}

}  // namespace common
}  // namespace termspp
