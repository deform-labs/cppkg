#include "../../engine/src/cppkg_engine.h"
#include "../../api/src/version/version.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

cppkg::package CliPackage;
cppkg::command_system CliCmd;
cppkg::github CliGit;
cppkg::build CliBuild;

CommandRegistry Registry;
namespace fs = std::filesystem;

void HandleError(const std::exception& e) {
    std::cerr << cppkg::ux::color::red << e.what()
              << cppkg::ux::color::reset << '\n';
}

void CheckArguments(int argc, int min_args, const std::string& usage) {
    if (argc < min_args)
        throw std::runtime_error(usage);
}

void* WorkspaceCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg workspace <init|build> [name]");
    std::string sub = argv[1];

    if (sub == "init") {
        CheckArguments(argc, 3, "Usage: cppkg workspace init <name>");
        CliPackage.create_workspace(argv[2]);
    } else if (sub == "build") {
        CliBuild.build_project(argc >= 3 ? argv[2] : ".");
    } else {
        throw std::runtime_error("Unknown workspace command: " + sub);
    }

    return nullptr;
}

void* VersionCommand(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << cppkg::ux::color::cyan << "cppkg version " << get_git_commit()
              << cppkg::ux::color::reset << '\n';
    return nullptr;
}

void* RemoveCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg remove <author/repo>");
    cppkg::dependency_impl deps;
    deps.remove(argv[1], fs::current_path().string());
    return nullptr;
}

void* CleanCommand(int argc, char* argv[]) {
    const fs::path path = argc >= 2 ? argv[1] : ".";
    cppkg::dependency_impl deps;
    fs::remove_all(path / "target");
    deps.clean(path.string());
    std::cout << cppkg::ux::color::green << "Cleaned build/target"
              << cppkg::ux::color::reset << '\n';
    return nullptr;
}

void* BuildCommand(int argc, char* argv[]) {
    CliBuild.build_project(argc >= 2 ? argv[1] : ".");
    return nullptr;
}

void* HelpCommand(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << cppkg::ux::color::green << "Usage: cppkg <command> <args>\n\n" << cppkg::ux::color::reset;
    std::cout << cppkg::ux::color::cyan << "Available commands:\n" << cppkg::ux::color::reset;

    if (Registry.commands.empty()) {
        std::cout << "   " << cppkg::ux::color::red << "(no commands registered)" << cppkg::ux::color::reset << "\n";
        return nullptr;
    }

    size_t max_len = 0;
    for (const auto& cmd : Registry.commands)
        max_len = std::max(max_len, cmd.name.size());

    for (const Command& cmd : Registry.commands) {
        const std::string padding(max_len - cmd.name.size() + 2, ' ');
        const std::string indent = "   " + std::string(max_len + 5, ' ');
        std::istringstream description(cmd.description);
        std::string line;
        std::getline(description, line);

        std::cout << "   " << cppkg::ux::color::yellow << cmd.name << cppkg::ux::color::reset
                  << padding << "- " << line << '\n';

        while (std::getline(description, line))
            std::cout << indent << line << '\n';
    }

    std::cout << '\n';
    return nullptr;
}

void* InitCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg init <name>");
    CliPackage.create_package(argv[1]);
    return nullptr;
}

void* GitCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg git <command>");
    std::string argument = argv[1];
    if (argument == "help") {
        std::cout << cppkg::ux::color::cyan
                  << "Git passthrough for cppkg.\n"
                  << "Usage: cppkg git <git arguments>\n"
                  << "Example: cppkg git status\n"
                  << cppkg::ux::color::reset;
        return nullptr;
    }
    CliCmd.run("git " + argument);
    return nullptr;
}

void* AddCommand(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--help") {
            std::cout << cppkg::ux::color::cyan
                      << "Add a dependency to the workspace or to the package.\n"
                      << "Usage: cppkg add <author/repo>@<version> [--https]"
                      << cppkg::ux::color::reset << '\n';
            return nullptr;
        }
    }

    CheckArguments(argc, 2, "Usage: cppkg add <author/repo>@<version>");
    bool https = false;
    std::string spec = argv[1];
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--https") https = true;
    }
    cppkg::dependency_impl deps(https);
    deps.add(spec, fs::current_path().string());
    return nullptr;
}

void* RunCommand(int argc, char* argv[]) {
    CliBuild.run_project(argc >= 2 ? argv[1] : ".");
    return nullptr;
}

void* SearchCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg search <query>");
    auto results = CliGit.search(argv[1]);
    if (results.empty()) {
        std::cout << cppkg::ux::color::red << "No results found for: " << argv[1]
                  << cppkg::ux::color::reset << '\n';
        return nullptr;
    }
    for (const auto& r : results) {
        std::cout << "   " << cppkg::ux::color::yellow << r.full_name << cppkg::ux::color::reset
                  << " - " << r.description
                  << cppkg::ux::color::cyan << " Stars: " << r.stars << cppkg::ux::color::reset << '\n';
    }
    return nullptr;
}

void* PublishCommand(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg publish <version> [--message <msg>] [--notes <path>]");

    std::string version = argv[1];
    std::string message = "Release v" + version;

    for (int i = 2; i < argc; ++i) {
        const std::string option = argv[i];
        if (option != "--message" && option != "--notes")
            throw std::runtime_error("Unknown publish option: " + option);
        if (i + 1 >= argc)
            throw std::runtime_error("Missing value for " + option);

        const std::string value = argv[++i];
        if (option == "--message") {
            message = value;
        } else {
            std::ifstream notes(value);
            if (!notes)
                throw std::runtime_error("Unable to open release notes: " + value);
            message.assign(std::istreambuf_iterator<char>(notes), std::istreambuf_iterator<char>());
        }
    }

    const char* token_env = std::getenv("CPPKG_TOKEN");
    if (!token_env)
        throw std::runtime_error("No GitHub token found. Set the CPPKG_TOKEN environment variable.");

    CliGit.publish(version, message, token_env);
    return nullptr;
}

void* CreateCmakeLists(int argc, char* argv[]) {
    CheckArguments(argc, 2, "Usage: cppkg cmake <project_dir>");

    fs::path project_dir = argv[1];
    CliBuild.create_lists(project_dir);
    return nullptr;
}

void InitializeCommands() {
    Registry.AddCommand(Command("clean", "Clean build artifacts and dependencies", CleanCommand));
    Registry.AddCommand(Command("search", "Search for a dependency on GitHub\n - e.g. cppkg search fmt\n", SearchCommand));
    Registry.AddCommand(Command("add", "Add a dependency (author/repo@version)\n - --https  force HTTPS instead of SSH\n", AddCommand));
    Registry.AddCommand(Command("publish", "Publish a package to GitHub\n - --message <message>  commit message\n - --notes <path>  release notes file\n", PublishCommand));
    Registry.AddCommand(Command("cmake", "Generate CMake compatibility files", CreateCmakeLists));
    Registry.AddCommand(Command("git", "Pass a command through to Git", GitCommand));
    Registry.AddCommand(Command("version", "Print the cppkg version", VersionCommand));
    Registry.AddCommand(Command("workspace", "Manage workspaces", WorkspaceCommand));
    Registry.AddCommand(Command("new", "Initialize a new package", InitCommand));
    Registry.AddCommand(Command("remove", "Remove a dependency", RemoveCommand));
    Registry.AddCommand(Command("build", "Build the package", BuildCommand));
    Registry.AddCommand(Command("run", "Build and run the package", RunCommand));
    Registry.AddCommand(Command("help", "Display this help message", HelpCommand));
}

int ExecuteCommand(int argc, char* argv[]) {
    std::string command_name = argv[1];
    std::vector<std::string> command_args;
    for (int i = 2; i < argc; ++i) command_args.push_back(argv[i]);

    auto& alias_manager = cppkg::alias::get();

    if (alias_manager.is_alias(command_name)) {
        std::string real_command = alias_manager.get_command(command_name);
        std::cout << "Expanding alias: " << command_name << " -> " << real_command << '\n';

        std::vector<std::string> expanded;
        std::stringstream ss(real_command);
        std::string token;
        while (ss >> token) expanded.push_back(token);

        if (!expanded.empty()) {
            command_name = expanded[0];
            command_args.clear();
            for (size_t i = 1; i < expanded.size(); ++i) {
                command_args.push_back(expanded[i]);
            }
        }
    }

    Registry.ExecuteCommand(command_name, command_args);
    return 0;
}

int main(int argc, char* argv[]) {
    InitializeCommands();

    if (argc < 2) {
        HelpCommand(argc, argv);
        return 0;
    }

    try {
        return ExecuteCommand(argc, argv);
    } catch (const std::exception& e) {
        HandleError(e);
        return 1;
    }
}
