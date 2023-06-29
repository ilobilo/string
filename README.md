# C++20/23 std::string implementation
Single-header, almost fully complete C++20/23 std::string implementation with sso support.\
Can be used in a freestanding environment with https://github.com/ilobilo/libstdcxx-headers and custom header ``<memory>`` that implements ``std::allocator<>``.\
See [memory](memory) for the reference implementation.

Note: This header has not been fully tested. if there are any compilation errors or some methods don't function as C++ standard states, please open a github issue.

## Including
* You can use this header by adding ``include`` to your headers search path:\
``-I$(this_dir)/include``
* If you use meson, you can also do:\
``dependency('string')``

## Supported options
* ``NOSTD_STRING_NUMERIC_CONVERSIONS``: Enable support for numeric conversion functions.
* ``NOSTD_STRING_FLOAT``: Enable floating point numeric conversion functions.
* ``NOSTD_STRING_LONG_DOUBLE``: Enable support for long double in numeric conversion functions. (Needs ``NOSTD_STRING_FLOAT``)
* ``NOSTD_STRING_EXCEPTIONS``: Enable exceptions support.
* ``NOSTD_STRING_FALLBACK_ASSERT``: Use ``assert()`` from ``<cassert>`` if exceptions are not available.
* ``NOSTD_STRING_FALLBACK_ABORT`` Use ``std::abort()`` from ``<cstdlib>`` if nether exceptions nor ``assert()`` is available.
* ``NOSTD_STRING_FALLBACK_ABORT_FUNCTION``: Function to run and then loop indefinitely, if nether exceptions, ``assert()`` nor ``std::abort()`` is available. (Default value: ``[] (auto) { }``. Function takes error string literal as an argument)
* ``NOSTD_STRING_STD_HASH``: Enable support for ``std::hash``. (Uses ``Murmurhash2-64a``)
* ``NOSTD_STRING_CONTAINERS_RANGES``: Enable support for C++23 ``P1206R4: Conversions from ranges to containers``.

Note: Every option is enabled by default.\
Note: Every option apart from ``NOSTD_STRING_STD_HASH`` will be automatically disabled if required headers or functions were not found or option that they depend on is not enabled.

## Example usage
```cpp
#include <nostd/string.hpp>
#include <iostream> // for std::cout

// currently constexpr string must fit in sso
// alternatively you can use std::string_view
constexpr nostd::string world("World");

auto main() -> int
{
    using namespace nostd::literals::string_literals;
    auto hello = "Hello"s;

    nostd::string str = hello + ", " + world + "!";

    // you could also overload std::cout to support nostd::string
    std::cout << str.c_str() << std::endl;
    return 0;
}
```
Output:
```
Hello, World!
```

## Currently missing functions:
* ``resize_and_overwrite``
* ``to_string(float/double/long double)``
* ``to_wstring(all)``