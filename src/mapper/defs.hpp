#pragma once

#include "termspp/common/utils.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace termspp {
namespace mapper {

namespace common = termspp::common;

/// Const. format str used by `mesh::MapResult` to fmt its description
constexpr const auto kMapResultFormatStr = std::string_view("%s with msg: %s");

/// Enum describing the parsing / op status
///   - returned as a member of the `MapResult` object
enum class MapStatus : uint8_t {
  kUnknownErr,       // Default err state describing an unknown / unexpected err
  kFileNotFoundErr,  // File wasn't found when attempting to load the file
  kFileInitErr,      // Failed to initialise line reader
  kLineReaderErr,    // Failed to read line
  kAllocationErr,    // Failed to allocate memory
  kNoRowData,        // No row data was parsed for this row
  kSuccessful,       // No error
};

/// Op result descriptor
///   - defines status, and assoc. message, describing whether an op succeeded
typedef common::Result<MapStatus, MapStatus::kSuccessful> MapResultBase;

struct MapResult final : public MapResultBase {
  // Default constructor
  MapResult() : MapResultBase(MapStatus::kUnknownErr) {};

  /// Construct with a status
  explicit MapResult(MapStatus status) : MapResultBase(status) {};

  /// Construct with a status and attach an err message
  MapResult(MapStatus status, std::string message) : MapResultBase(status, std::move(message)) {};

  /// Getter: resolve the description assoc. with the result's status
  [[nodiscard]] auto Description() const -> std::string override {
    auto result = std::string{};
    switch (Status()) {
    case MapStatus::kSuccessful:
      result = "Success";
      break;
    case MapStatus::kFileNotFoundErr:
      result = "Failed to load file";
      break;
    case MapStatus::kFileInitErr:
      result = "Failed to initialise line reader";
      break;
    case MapStatus::kLineReaderErr:
      result = "Failed to read line";
      break;
    case MapStatus::kAllocationErr:
      result = "Failed to allocate memory";
      break;
    case MapStatus::kNoRowData:
      result = "No data was parsed for this row";
      break;
    case MapStatus::kUnknownErr:
    default:
      result = "Unknown error occurred whilst processing Map document";
      break;
    }

    auto msg = Message();
    if (!msg.empty()) {
      auto size = std::snprintf(nullptr, 0, kMapResultFormatStr.data(), result.c_str(), msg.c_str());
      if (size > 0) {
        size++;

        auto fmt = std::string(size, '0');
        std::snprintf(fmt.data(), size, kMapResultFormatStr.data(), result.c_str(), msg.c_str());
        return fmt;
      }
    }

    return result;
  }
};

}  // namespace mapper
}  // namespace termspp
