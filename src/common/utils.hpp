#pragma once

#include <cstring>
#include <functional>
#include <string_view>

namespace termspp {
namespace common {

/// Combine hash - see @Boost ref
constexpr const auto kFracGolden  = 0x9e3779b9;
constexpr const auto kLHashOffset = 6;

template <typename T, typename... Rest>
inline auto hashCombine(std::size_t &seed, T const &val, Rest &&...rest) -> void {
  auto hasher  = std::hash<T>{};
  seed        ^= hasher(val) + kFracGolden + (seed << kLHashOffset) + (seed >> 2);
  (hashCombine(seed, rest), ...);
}

// Hash `std::pair<T1, T2>` pairs
struct PairHash {
  template <class T1, class T2>
  auto operator()(const std::pair<T1, T2> &pair) const->size_t {
    size_t seed = 0;
    hashCombine(seed, pair.first, pair.second);

    return seed;
  }
};

/// Hash fn to group char* keys into buckets for `std::unordered_map`
struct CharHash {
public:
  auto operator()(const char *str) const->size_t {
    return std::hash<std::string_view>()(str);
  }
};

/// Comparator for `std::unordered_maps` with keys describing char* references
struct CharComp : public std::function<bool(const char *, const char *)> {
public:
  auto operator()(const char *str1, const char *str2) const->bool {
    return str1 == str2 || std::strcmp(str1, str2) == 0;
  }
};

/// Op result descriptor
///   - defines status, and assoc. message, describing whether an op succeeded
template <typename T, auto Success>
struct Result {
  /// Cast to bool op to test err state
  explicit operator bool() const {
    return status_ == Success;
  }

  /// Output stream friend insertion operator
  friend auto operator<<(std::ostream &stream, const Result &obj)->std::ostream & {
    return stream << obj.Description();
  }

  /// Setter: set the result status
  auto SetStatus(T status) -> void {
    status_ = status;
  }

  /// Setter: set the result message
  auto SetMessage(std::string str) -> void {
    message_ = std::move(str);
  }

  /// Getter: get the result status
  [[nodiscard]] auto Status() const -> T {
    return status_;
  }

  /// Getter: get the message, if any, associated with this result
  [[nodiscard]] auto Message() const -> std::string {
    return message_;
  }

  /// Getter: resolve the description assoc. with the result's status
  [[nodiscard]] virtual auto Description() const -> std::string {
    return std::string{"Result"};
  }

  /// Getter: Sugar for bool() conversion operator reflecting the success status
  [[nodiscard]] auto Ok() const -> bool {
    return status_ == Success;
  }

private:
  T           status_{Success};  // Op status enum
  std::string message_;          // Optional message alongside derived description

protected:
  /// Default constructor
  Result() = default;

  /// Construct with a status
  explicit Result(T status) : status_(status) {};

  /// Construct with a status and attach an err message
  Result(T status, std::string message) : status_(status), message_(std::move(message)) {};
};

}  // namespace common
}  // namespace termspp
