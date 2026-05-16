# cppkg

A package manager for C++ inspired by Cargo. Create, build, and manage C++ projects and workspaces from the command line.

## Installation

Clone the repo and build with CMake:

```powershell
git clone https://github.com/deform-labs/cppkg
cd cppkg
cmake -B build
cmake --build build
```

Then add the output directory to your `PATH` so you can run `cppkg` from anywhere.

## Demos
![init and build demo](demos/cppkg_demo.gif)
![workspace demo](demos/cppkg_demo_1.gif)
---
![demo](demos/demo_2.png)

## Commands

### `cppkg init <name>`
Scaffold a new C++ package. If run inside a workspace, it automatically registers the package as a member.

```
cppkg init my-project
```

Creates:
```
my-project/
├── target/          ← build artifacts and cloned dependencies live here
│   ├── deps/        ← cloned dependencies (e.g. fmt, spdlog, …)
│   └── …            ← CMake build output
├── src/
│   └── main.cpp
├── .gitignore
└── cppkg.toml
```

### `cppkg build [name]`
Build a package. Run from inside a package directory, or pass a path.

```
cppkg build         # builds the current directory
cppkg build my-project
```

* Reads `cppkg.toml` for project metadata.
* Generates a `CMakeLists.txt` that:
  * Adds every dependency under `target/deps/<repo>` via `add_subdirectory`.
  * Links the resulting libraries (if any) to the executable.
* Compiles the package source files (`src/*.cpp`) into `target/` using Ninja.

### `cppkg add <author/repo>@<version>`
Add a dependency to `cppkg.toml`.

* **Format:** `author/repo@version` (e.g. `fmtlib/fmt@10.1.0`).
* The command validates the remote repository (SSH first, HTTPS fallback) before writing to `cppkg.toml`.
* Use `--https` if you want to force HTTPS cloning.

```
cppkg add fmtlib/fmt@10.1.0
cppkg add spdlog/spdlog@1.12.0 --https
```

### `cppkg remove <author/repo>`
Remove a dependency from `cppkg.toml`.

```
cppkg remove fmtlib/fmt
cppkg remove spdlog/spdlog@1.12.0
```

### `cppkg clean [path]`
Remove all cloned dependencies and build artifacts.

```
cppkg clean            # cleans the current package
cppkg clean my-project # cleans a specific package
```

### `cppkg workspace init <name>`
Create a new workspace.

```
cppkg workspace init my-workspace
```

Creates:
```
my-workspace/
├── .gitignore
└── cppkg.toml   ← contains a [workspace] section
```

The workspace `cppkg.toml` will be updated automatically when you initialize packages inside it.

### `cppkg help`
Show available commands.

```
cppkg help
```

## Package Manifest

`cppkg.toml` for a package:

```toml
[package]
name = "my-project"
version = "0.1.0"
cpp_std = "c++20"

[dependencies]
fmtlib/fmt = "10.1.0"
spdlog/spdlog = "1.12.0"
```

*The key in `[dependencies]` is the full `author/repo` identifier; the value is the tag/branch to checkout.*

`cppkg.toml` for a workspace:

```toml
[workspace]
members = [
    "my-lib",
    "my-app",
]
```

## Workspaces

Workspaces group multiple packages under a single root. Initialize a workspace, then `cd` into it and run `cppkg init` to add members automatically.

```powershell
cppkg workspace init my-workspace
cd my-workspace
cppkg init my-lib
cppkg init my-app
```

The workspace `cppkg.toml` will be updated automatically:

```toml
[workspace]
members = [
    "my-lib",
    "my-app",
]
```

## License

MIT
