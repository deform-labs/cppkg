#include "include/handle_package/handle_package.h"
#include "include/dependency/dependency_service.h"
#include "include/build_project/build_project.h"
#include "include/version/version.h"
#include "helpers/color.h"
#include "commands.h"
#include "the_shell.h"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>


/// dumbfuck if you cant understand ts
void handle_error(const std::runtime_error& e) {
    std::cout << Color::red << e.what() << Color::reset << std::endl;
}

/// ok so this one might need an explanation but it's pretty self-explanatory for amateurs :sunglasses:
/// for dumbasses: it checks the amount of arguments against the minimum required
void check_arguments(int argc, int min_args, const std::string& usage) {
    if (argc < min_args) {
        std::cout << Color::green << usage << Color::reset << "\n";
        exit(1);
    }
}

/// OOH COMMANDS OHHH
void* workspace_command(int argc, char* argv[]) {
    Build build;
    HandlePackage handlePackage;

    check_arguments(argc, 3, "Usage: cppkg workspace <init|build> <name>");
    std::string sub = argv[2];

    if (sub == "init") {
        check_arguments(argc, 4, "Usage: cppkg workspace init <name>");
        handlePackage.create_workspace(argv[3]);
    } else if (sub == "build") {
        if (argc >= 4) {
            build.build_project(argv[3]);
        } else {
            build.build_project(".");
        }
    }

    return nullptr;
}; /// workspaces everywhere employment for everbody!
void* crash_pc_command(int argc, char* argv[]) {
    int* ptr = nullptr;
    *ptr = -1;
    return ptr;
}; /// actually fucking destroys your pc
void* version_command(int argc, char* argv[]) {
    std::cout << Color::cyan << "cppkg version " << get_git_commit() << Color::reset << std::endl;
    return nullptr;
}; /// commit number WHAT?
void* remove_command(int argc, char* argv[]) {
    std::string name = argv[2];
    DependencyService deps;
    deps.remove(name);
    return nullptr;
}; /// remove command lmao ima remove this spike from your ass
void* clean_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) {
        path = argv[2];
    }
    DependencyService deps;
    deps.clean(path);
    std::cout << Color::green << "Cleaned build/target" << Color::reset << "\n";
    return nullptr;
}; /// cleans yo buh hole
void* Build_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) {
        path = argv[2];
    }
    Build build;
    build.build_project(path);
    return nullptr;
}; /// builds the project
void* help_command(int argc, char* argv[]) {
    CommandRegistry registry;

    std::cout << Color::green << "Usage: cppkg <command> <args>\n\n" << Color::reset;
    std::cout << Color::cyan << "Available commands:\n" << Color::reset;

    std::vector<Command> commands = registry.commands;
    if (commands.empty()) {
        std::cout << "   " << Color::red << "(no commands registered)" << Color::reset << "\n";
        return nullptr;
    }

    // this comment was nuked by the turtle
    size_t max_len = 0;
    for (const auto& cmd : commands) {
        max_len = std::max(max_len, cmd.name.size());
    }

    /// I CHOOSE DEATH!
    for (const Command& cmd : commands) {
        std::string name = Color::yellow + cmd.name + Color::reset;
        std::string desc = " - " + cmd.description + "\n";
        std::cout << "   " << name << desc;
    }

    std::cout << "\n";
    return nullptr;
}; /// helpp please!!!
void* init_command(int argc, char* argv[]) {
    HandlePackage handlePackage;
    check_arguments(argc, 3, "Usage: cppkg init <project-name>");
    handlePackage.create_package(argv[2]);
    return nullptr;
}; /// initialize this ahh
void* git_command(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg git <command>");
    std::string argument = argv[2];
    shell_.run("git " + argument);
    return nullptr;
}; /// oh look mom a git integration!
void* add_command(int argc, char* argv[]) {
    bool https = false;
    std::string spec = argv[2];
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--https") https = true;
        if (std::string(argv[i]) == "--help")
            std::cout << Color::cyan <<
                "Add a dependency to the workspace or to the package.\n Usage: cppkg add <author/repo>@<version>";
    }
    DependencyService deps(https);
    deps.add(spec);
    return nullptr;
}; /// add command lmao ima add this spike up your ass
void* run_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) {
        path = argv[2];
    }
    Build build;
    build.run_project(path);
    return nullptr;
}; /// runs the project

/*
 * just adds base commands to the registry
 * if you didnt already understand it idk whats in your brain
 */
void INITIALIZE(CommandRegistry& registry) {
    //descending spiral. just how i like it.
    registry.addCommand(Command("git", "github integration inside of cppkg!", git_command));
    registry.addCommand(Command("clean", "Clean build artifacts and dependencies", clean_command));
    registry.addCommand(Command("add", "Add a dependency (author/repo@version)", add_command));
    registry.addCommand(Command("version", "Print the version of cppkg", version_command));
    registry.addCommand(Command("build", "Build the package", Build_command));
    registry.addCommand(Command("workspace", "Manage workspaces", workspace_command));
    registry.addCommand(Command("init", "Initialize a new package", init_command));
    registry.addCommand(Command("remove", "Remove a dependency", remove_command));
    registry.addCommand(Command("crash", "crash the pc", crash_pc_command));
    registry.addCommand(Command("run", "Run the package", run_command));
    registry.addCommand(Command("help", "Show help", help_command));
}

/// just look it up in google if you dont know what this does.
int main(int argc, char* argv[]) {
    CommandRegistry registry;
    INITIALIZE(registry);

    check_arguments(argc, 2, "Usage: cppkg <command>");

    try {
        registry.executeCommand(argv[1], argc, argv);
    } catch (const std::runtime_error& e) {
        handle_error(e);
        return 1;
    }

    return 0;
}




































































































































































































































































































































































































































































































































































































































/// the real jackpot were the comments along the way -- yydev-official 2026.