#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace termspp {
namespace common {

/// TODO(J): docs
static constexpr size_t        kArenaAlignment   = 64L;
static constexpr const int64_t kDefaultArenaSize = 4096LL;

/// TODO(J): docs
class Arena final {
public:
  static auto Create(int64_t csize = kDefaultArenaSize) -> std::unique_ptr<Arena>;

public:
  ~Arena();

  Arena(Arena const &)                   = delete;
  auto operator=(Arena const &)->Arena & = delete;

  [[nodiscard]] auto Allocate(int64_t size, uint8_t **ptr) -> bool;

  auto Release() -> void;

private:
  struct Region {
    uint8_t *buf;
    int64_t  size;
  };

  [[nodiscard]] auto allocateRegion(int64_t size) -> bool;

  auto releaseRegions(bool destroy = false) -> void;

private:
  uint8_t *cbuf_;
  int64_t  mcsize_;
  int64_t  rsize_;

  // NOTE(J): should probably be a linked list instread?
  std::vector<Region> regions_;

protected:
  explicit Arena(int64_t csize);
};

}  // namespace common
}  // namespace termspp
