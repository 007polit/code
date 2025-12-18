#pragma once
#include <string>
#include <optional>

namespace editer {
namespace utils {

// Result type for error handling
// Inspired by Rust's Result<T, E>
template<typename T = void>
class Result {
private:
    bool success_;
    std::optional<T> value_;
    std::string error_message_;

public:
    // Success constructor
    static Result ok(const T& value) {
        Result r;
        r.success_ = true;
        r.value_ = value;
        return r;
    }

    // Error constructor
    static Result error(const std::string& message) {
        Result r;
        r.success_ = false;
        r.error_message_ = message;
        return r;
    }

    // Check if result is success
    bool is_ok() const { return success_; }
    bool is_error() const { return !success_; }

    // Get value (only valid if is_ok())
    const T& value() const { return value_.value(); }
    T& value() { return value_.value(); }

    // Get error message (only valid if is_error())
    const std::string& error() const { return error_message_; }

    // Unwrap value or return default
    T value_or(const T& default_value) const {
        return success_ ? value_.value() : default_value;
    }

private:
    Result() : success_(false) {}
};

// Specialization for void (no return value)
template<>
class Result<void> {
private:
    bool success_;
    std::string error_message_;

public:
    // Success constructor
    static Result ok() {
        Result r;
        r.success_ = true;
        return r;
    }

    // Error constructor
    static Result error(const std::string& message) {
        Result r;
        r.success_ = false;
        r.error_message_ = message;
        return r;
    }

    // Check if result is success
    bool is_ok() const { return success_; }
    bool is_error() const { return !success_; }

    // Get error message (only valid if is_error())
    const std::string& error() const { return error_message_; }

private:
    Result() : success_(false) {}
};

} // namespace utils
} // namespace editer
