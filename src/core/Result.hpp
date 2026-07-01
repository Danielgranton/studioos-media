#pragma once

#include <string>

#include "StatusCode.hpp"

template<typename T>
class Result
{
public:
    bool success = false;
    StatusCode status = StatusCode::INTERNAL_ERROR;
    std::string message;
    T value{};

    static Result<T> ok(const T& value)
    {
        Result<T> result;
        result.success = true;
        result.status = StatusCode::SUCCESS;
        result.value = value;
        return result;
    }

    static Result<T> fail(
        const std::string& message,
        StatusCode status = StatusCode::INTERNAL_ERROR)
    {
        Result<T> result;
        result.success = false;
        result.status = status;
        result.message = message;
        return result;
    }
};