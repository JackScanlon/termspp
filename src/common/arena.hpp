#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace termspp {
namespace common {

static constexpr const int64_t kDefaultArenaSize = 4096;

class Arena final {
public:
  static auto Create(int64_t csize = kDefaultArenaSize) -> std::unique_ptr<Arena>;

public:
  ~Arena();

  Arena(Arena const &)                   = delete;
  auto operator=(Arena const &)->Arena & = delete;

  auto Allocate(int64_t size, uint8_t **ptr) -> bool;
  auto Release() -> void;

private:
  struct Region {
    uint8_t *buf;
    int64_t  size;
  };

  auto allocateRegion(int64_t size) -> bool;
  auto releaseRegions(bool destroy = false) -> void;

private:
  int64_t  mcsize_;
  int64_t  rsize_;
  uint8_t *cbuf_;

  std::vector<Region> regions_;

protected:
  explicit Arena(int64_t csize);
};

}  // namespace common
}  // namespace termspp
