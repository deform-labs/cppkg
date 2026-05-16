# Contributing to cppkg

Thanks for your interest in contributing. This document covers how to get set up and how the codebase is structured.

## Requirements

- Windows (MSVC toolchain)
- CMake 3.10+
- Visual Studio 2022 Build Tools
- C++20

## Building from source

```powershell
git clone https://github.com/deform-labs/cppkg
cd cppkg
cmake -B build
cmake --build build
```

The output binary will be at `build/Debug/cppkg.exe`.

## Project structure

```
src/
├── main.cpp                        # entry point, command registration
├── commands.h                      # Command and CommandRegistry structs
├── commands/
│   ├── handle_package.cpp          # init, add, remove logic
│   ├── build.cpp                   # build command logic
│   ├── workspace.cpp               # workspace logic
│   └── helpers/
│       ├── color.h                 # ANSI color constants
│       └── create_file.h           # file creation utility
└── include/
    ├── handle_package/
    │   └── handle_package.h
    ├── build/
    │   └── build.h
    ├── workspace/
    │   └── workspace.h
    └── toml/
        ├── toml.h                  # Toml / TomlSection / TomlValue structs
        └── toml_parser.h           # parse_toml() and parse_array()
```

## Adding a new command

Commands are registered in `add_base_commands()` in `main.cpp`. To add a new one:

1. Register it in `main.cpp`:

```cpp
registry.addCommand(Command("mycommand", "Description", [](int argc, char* argv[]) {
    check_args(argc, 3, "Usage: cppkg mycommand <arg>");
    // your logic here
}));
```

2. If the logic is non-trivial, add a new `.cpp` file under `src/commands/` and a header under `src/include/`.

3. Since `CMakeLists.txt` uses `GLOB_RECURSE`, new files are picked up automatically on the next cmake run.

## Code style

- C++20
- Snake case for functions and variables
- Classes use PascalCase
- Keep command logic out of `main.cpp` — register there, implement elsewhere
- Use `Color::` constants for all terminal output
- Throw `std::runtime_error` for user-facing errors — they're caught and printed in red by `handle_error()`

## Submitting changes

1. Fork the repo
2. Create a branch: `git checkout -b my-feature`
3. Make your changes
4. Open a pull request with a clear description of what you changed and why
