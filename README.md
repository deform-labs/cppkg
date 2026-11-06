# cppkg
[![CMake on multiple platforms](https://github.com/deform-labs/cppkg/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/deform-labs/cppkg/actions/workflows/cmake-multi-platform.yml)
[![CMake Hate](https://img.shields.io/badge/CMake-Hate-red)]()
[![Uses Your Compiler](https://img.shields.io/badge/Compiler-Your%20Choice-brightgreen)]()
[![Self-Hosting](https://img.shields.io/badge/Self--Hosting-Yes-brightgreen)]()
### Cargo-inspired package manager for C++ with absolutely no dependencies! Because manually creating `src/`, `CMakeLists.txt`, and `build/` for every project is hell. also because i hate cmake.
 
### Cross-platform, self-hosting, and with Git out of the box.

## ALL COMMENTS IN THE FILES ARE ALL WRITTEN BY HUMANS!

## Features
- `cppkg init` — scaffold a new C++ project in seconds
- `cppkg add author/repo@version` — fetch dependencies directly from GitHub
- `cppkg build` — fetches dependencies,creates lock file and compiles. 
- `cppkg clean` — remove build artifacts and cloned dependencies
- Workspace support — manage multiple packages under one root
- SSH with HTTPS fallback for dependency cloning
- Cross-platform — Windows (MSVC), Linux (GCC, Clang)

> **NOTE** cppkg has cmakelists generation support for all cmake freaks out there
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
- Writes `compile_commands.json` for clangd and other tooling

### Editor support

Every `cppkg build` writes `compile_commands.json` at the project root, with one entry per source file:

```json
[
  {
    "directory": "/home/me/my-project",
    "file": "/home/me/my-project/src/main.cpp",
    "output": "/home/me/my-project/target/objs/main.o",
    "command": "g++ -std=c++20 -Wall -Wextra -O2 -I\"src\" -c \"/home/me/my-project/src/main.cpp\" -o \"/home/me/my-project/target/objs/main.o\" -MMD -MF \"/home/me/my-project/target/objs/main.o.d\""
  }
]
```

- Paths are absolute and `command` is exactly what cppkg runs, so any tool can replay an entry from any working directory.
- It is rewritten on every build, including builds where every object is already up to date, so it never advertises flags or include paths you have since changed.
- It sits next to `cppkg.toml`, not inside `target/`, so `cppkg clean` leaves it alone. Add it to `.gitignore` if you would rather not commit it.
- Point clangd at the project and it picks up the standard, `compiler_flags` and dependency include paths with no `compile_flags.txt` needed.

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

### Manifest reference

| Table | Key | Meaning |
|---|---|---|
| `[package]` | `name` | Project name. Required. Also the name of the output binary (`target/<name>`). |
| `[package]` | `version` | Package version. |
| `[package]` | `cpp_std` | C++ standard, default `c++20`. `c++20`, `20` and `latest` all work. |
| `[package]` | `compiler` | Compiler executable, e.g. `clang++`. Defaults to `g++` (`cl` on Windows). |
| `[package]` | `description`, `authors`, `license` | Metadata only; not used by the build. |
| `[compiler]` | `IncludePaths` | Extra `-I` roots, e.g. `["src", "vendor/include"]`. |
| `[compiler]` | `LibraryPaths` | Extra `-L` roots, e.g. `["vendor/lib"]`. |
| `[compiler]` | `Libraries` | Libraries to link; `["fmt"]` becomes `-lfmt`. |
| `[compiler]` | `compiler_flags` | Extra flags appended to the defaults (`-Wall -Wextra -O2`). |
| `[dependencies]` | `author/repo` | Dependency to clone; the value is its tag or branch. |
| `[workspace]` | `members` | Sub-packages of a workspace root. |

Everything except `name` is optional. Dependencies pulled into `target/deps/` get their header roots and `lib/` directories added to the compile and link paths automatically.

Unknown keys are reported rather than silently ignored, so a typo does not quietly do nothing:

```
[WARN] cppkg.toml: unknown key "Compiler_Flags" in [compiler], ignored
```

The manifest is read by a real TOML parser: comments anywhere, multi-line arrays with trailing commas, and both `"basic"` and `'literal'` strings with `\n`, `\t`, `\"`, `\\` and `\uXXXX` escapes. A malformed manifest names the file and line and stops the build instead of guessing:

```
cppkg.toml:3: duplicate key "name" in [package]
cppkg.toml:2: expected '=' after key "name"
cppkg.toml: missing key "name" in [package]
```
 
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
 
- No conflict resolution for transitive dependencies
## License

## Star History

<a href="https://www.star-history.com/?repos=deform-labs%2Fcppkg&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=deform-labs/cppkg&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=deform-labs/cppkg&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=deform-labs/cppkg&type=date&legend=top-left" />
 </picture>
</a>
 
MIT
