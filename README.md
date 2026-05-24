# cppkg
[![CMake on multiple platforms](https://github.com/deform-labs/cppkg/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/deform-labs/cppkg/actions/workflows/cmake-multi-platform.yml)
[![CMake Hate](https://img.shields.io/badge/CMake-Hate-red)]()
[![Uses Your Compiler](https://img.shields.io/badge/Compiler-Your%20Choice-brightgreen)]()
[![Self-Hosting](https://img.shields.io/badge/Self--Hosting-Yes-brightgreen)]()
### Cargo-inspired package manager for C++. Because manually creating `src/`, `CMakeLists.txt`, and `build/` for every project is hell.
### also because i hate cmake.
 
### Cross-platform, self-hosting, and with Git out of the box.

## ALL COMMENTS IN THE FILES ARE ALL WRITTEN BY HUMANS!

## About AI generated code.
> i've always thought that ai code generation was a good idea, but i've always been skeptical about the quality of the code it produces.
> since cppkg is a really hard to develop project, i wanted to make sure that the code it generates is of high quality.
> and so i've decided to put all AI prompts for the AI generated code here, so you can see exactly what cppkg devs are asking the AI to generate.
> this doesnt mean the project is vibe coded, it just means AI was part of the development process.


>> please if youre interested review these chats to see the ai code generation process.

[DEEPSEEK](https://chat.deepseek.com/share/jvd8m4jsh0izxpvssu)

[CLAUDE](https://claude.ai/share/9d3617b1-bf39-4b5d-a8e9-257e42e7f4a3)

## Features
- `cppkg init` — scaffold a new C++ project in seconds
- `cppkg add author/repo@version` — fetch dependencies directly from GitHub
- `cppkg build` — generate CMakeLists, fetch deps, and compile
- `cppkg clean` — remove build artifacts and cloned dependencies
- Workspace support — manage multiple packages under one root
- SSH with HTTPS fallback for dependency cloning
- Cross-platform — Windows (MSVC), Linux (GCC, Clang)
## Installation
 
### From release
 
Download the latest binary from [Releases](https://github.com/deform-labs/cppkg/releases) and add it to your `PATH`.
 
### From source
 
```bash
git clone https://github.com/deform-labs/cppkg
cd cppkg
cmake -B build -S cppkg
cmake --build build
```
 
Add the output directory to your `PATH`.
 
> **Note:** cppkg is self-hosting — once you have a binary, you can use `cppkg build cppkg` to rebuild itself.
 
## Usage
 
### Create a project
 
```
cppkg init my-project
cd my-project
```
 
Creates:
```
my-project/
├── src/
│   └── main.cpp
├── .gitignore
└── cppkg.toml
```
 
### Add a dependency
 
```
cppkg add fmtlib/fmt@10.1.0
cppkg add gabime/spdlog@v1.12.0
cppkg add nlohmann/json@v3.11.2 --https
```
 
- Format: `author/repo@version`
- Validates the remote repo exists before writing to `cppkg.toml`
- SSH by default, HTTPS fallback automatic (or force with `--https`)
### Build
 
```
cppkg build          # build current directory
cppkg build my-project
```
 
- Reads `cppkg.toml` for project metadata
- Clones any missing dependencies into `target/deps/`
- Compiles with your own compiler! no cmake and nothing extra is needed. use your pre-existing compiler on your computer!
- **Automatically parallel** (uses all CPU cores) and **cached** (fast rebuilds)
### Remove a dependency
 
```
cppkg remove fmtlib/fmt
```
 
### Clean
 
```
cppkg clean
cppkg clean my-project
```
 
Removes `target/deps/` and build artifacts.
 
## Package manifest
 
```toml
[package]
name = "my-project"
version = "0.1.0"
cpp_std = "c++20"
 
[dependencies]
fmtlib/fmt = "10.1.0"
gabime/spdlog = "v1.12.0"
```
 
The key under `[dependencies]` is `author/repo`, the value is the tag or branch to clone.
 
## Workspaces
 
Group multiple packages under one root.
 
```
cppkg workspace init my-workspace
cd my-workspace
cppkg init my-lib
cppkg init my-app
```
 
`my-workspace/cppkg.toml`:
 
```toml
[workspace]
members = [
    "my-lib",
    "my-app",
]
```
 
Packages initialized inside a workspace are automatically registered as members.
 
## Project layout
 
please look at the source files to understand the layout.
 
## Limitations
 
- No lockfile yet — versions are not pinned to exact commits
- No conflict resolution for transitive dependencies
## License
 
MIT
