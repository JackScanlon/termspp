#pragma once

#include <string>
#include <utility>

namespace termspp {
namespace common {

/// Const. format str used by `common::Result` to fmt its description
constexpr const char *kResultFormatStr = "%s with msg: %s";

/// Enum describing the parsing / op status
///   - returned as a member of the `SctResult` object
enum class Status : uint8_t {
  kUnknownErr,           // Default err state describing an unknown / unexpected err
  kInvalidArguments,     // Raised when arguments are invalid/illegal, e.g. empty path string
  kFileNotFoundErr,      // File wasn't found when attempting to load the file
  kXmlReadErr,           // Err returned by pugixml parser
  kFileInitErr,          // Failed to initialise line reader
  kLineReaderErr,        // Failed to read line
  kAllocationErr,        // Failed to allocate memory
  kNoRowData,            // No row data was parsed for this row
  kPolicyErr,            // User-defined policy execution failure
  kRootDoesNotExistErr,  // Expected root node described by `mesh::kRecordSetNode` not found in document
  kNodeDoesNotExistErr,  // Node specified by spec does not exist
  kUnknownNodeTypeErr,   // Node type is not included in expected specification
  kEmptyNodeDataErr,     // Data resolved from node was empty
  kInvalidDataTypeErr,   // Failed to resolve data from node
  kSuccessful,           // No error
};

/// Op result descriptor
///   - defines status, and assoc. message, describing whether an op succeeded
struct Result {
  /// Default constructor
  Result() = default;

  /// Construct with a status
  explicit Result(Status status) : status_(status) {};

  /// Construct with a status and attach an err message
  Result(Status status, std::string message) : status_(status), message_(std::move(message)) {};

  /// Default destructor
  virtual ~Result() = default;

  /// Cast to bool op to test err state
  explicit operator bool() const {
    return status_ == Status::kSuccessful;
  }

  /// Output stream friend insertion operator
  friend auto operator<<(std::ostream &stream, const Result &obj)->std::ostream & {
    return stream << obj.Description();
  }

  /// Setter: set the result status
  auto SetStatus(Status status) -> void {
    status_ = status;
  }

  /// Setter: set the result message
  auto SetMessage(std::string str) -> void {
    message_ = std::move(str);
  }

  /// Getter: get the result status
  [[nodiscard]] auto Status() const -> Status {
    return status_;
  }

  /// Getter: get the message, if any, associated with this result
  [[nodiscard]] auto Message() const -> std::string {
    return message_;
  }

  /// Getter: resolve the description assoc. with the result's status
  [[nodiscard]] virtual auto Description() const -> std::string {
    auto result = std::string{};
    switch (Status()) {
    case Status::kSuccessful:
      result = "Success";
      break;
    case Status::kInvalidArguments:
      result = "Bad arguments";
      break;
    case Status::kFileNotFoundErr:
      result = "Failed to load file";
      break;
    case Status::kXmlReadErr:
      result = "Failed to parse MeSH XML document";
      break;
    case Status::kFileInitErr:
      result = "Failed to initialise line reader";
      break;
    case Status::kLineReaderErr:
      result = "Failed to read line";
      break;
    case Status::kAllocationErr:
      result = "Failed to allocate memory";
      break;
    case Status::kNoRowData:
      result = "No data was parsed for this row";
      break;
    case Status::kPolicyErr:
      result = "Failed to execute policy";
      break;
    case Status::kRootDoesNotExistErr:
      result = "Failed to find expected root node";
      break;
    case Status::kNodeDoesNotExistErr:
      result = "Failed to find expected descendant node";
      break;
    case Status::kUnknownNodeTypeErr:
      result = "Failed to resolve node type";
      break;
    case Status::kInvalidDataTypeErr:
    case Status::kEmptyNodeDataErr:
      result = "Failed to resolve node data";
      break;
    case Status::kUnknownErr:
    default:
      result = "Unknown error occurred whilst processing document";
      break;
    }

    auto msg = Message();
    if (!msg.empty()) {
      auto size = std::snprintf(nullptr, 0, kResultFormatStr, result.c_str(), msg.c_str());
      if (size > 0) {
        size++;

        auto fmt = std::string(size, '0');
        std::snprintf(fmt.data(), size, kResultFormatStr, result.c_str(), msg.c_str());
        return fmt;
      }
    }

    return result;
  }

  /// Getter: Sugar for bool() conversion operator reflecting the success status
  [[nodiscard]] auto Ok() const -> bool {
    return status_ == Status::kSuccessful;
  }

private:
  enum Status status_ { Status::kSuccessful };  // Op status enum
  std::string message_;                         // Optional message alongside derived description
};

}  // namespace common
}  // namespace termspp
