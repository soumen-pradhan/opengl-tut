#pragma once

/// RAII wrapper for callable statements invoked by DEFER()
template <typename F>
struct ScopedDefer {
    ScopedDefer(F f)
        : f(f)
    {
    }

    ~ScopedDefer() { f(); }

    F f;
};

/// A common macro for string concatenation during preprocessing phase.
#define STR_CONCAT(a, b) _STR_CONCAT(a, b)
#define _STR_CONCAT(a, b) a##b

/// Implementation of "defer" keyword similar in Go and Zig to automatically
/// call resource cleanup at end of function scope without copy-pasted cleanup
/// statements or separate RAII wrapper data types.
#define DEFER(x) \
    const auto STR_CONCAT(tmpDeferVarName, __LINE__) = ScopedDefer([&]() { x; })
