# abogus_cpp

A cross-platform C++ library for generating Douyin (TikTok China) a_bogus signatures.

## Features

- **Cross-platform**: Supports macOS, Linux, and Windows
- **Thread-safe**: Safe to call from any thread (macOS dispatches to main thread automatically)
- **C Interface**: Pure C API for easy integration with any language
- **Standalone executable**: Ships with a pre-built Node.js executable for signature generation

## Quick Start

### Installation

#### Using CMake

```bash
mkdir build && cd build
cmake ..
make
make install
```

#### Using FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    abogus
    GIT_REPOSITORY https://github.com/your-username/abogus_cpp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(abogus)

target_link_libraries(your_target PRIVATE abogus)
```

### Usage

```cpp
#include <abogus.h>
#include <iostream>

int main() {
    const char* userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";
    const char* params = "aid=6383&app_name=douyin_web&web_rid=123456";
    
    char* signature = get_abogus(userAgent, params);
    if (signature) {
        std::cout << "a_bogus=" << signature << std::endl;
        free_abogus(signature);
    } else {
        std::cerr << "Failed to generate signature" << std::endl;
    }
    
    return 0;
}
```

### Building the JS Executable

The library requires a `get_abogus` executable at runtime:

```bash
cd js
npm install
./build.sh macos-arm64  # or: macos-x64, linux, win, all
```

Set the `ABOGUS_JS_PATH` environment variable if the executable is in a non-standard location:

```bash
export ABOGUS_JS_PATH=/path/to/abogus_cpp/js
```

## API Reference

### `char* get_abogus(const char* userAgent, const char* params)`

Generate a_bogus signature.

- **Parameters**:
  - `userAgent`: HTTP User-Agent header (must match actual request)
  - `params`: URL query parameters string
- **Returns**: Signature string (caller must free with `free_abogus`), or `nullptr` on failure

### `void free_abogus(char* ptr)`

Free signature string returned by `get_abogus`.

### `const char* abogus_version(void)`

Get library version string.

## Directory Structure

```
abogus_cpp/
├── include/
│   └── abogus.h          # Public header
├── src/
│   └── abogus.cpp        # Implementation
├── js/
│   ├── a_bogus.js        # Signature algorithm
│   ├── get_abogus_node.js
│   ├── package.json
│   └── build.sh          # Build script
├── CMakeLists.txt
└── README.md
```

## License

MIT License
