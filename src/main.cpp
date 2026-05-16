#include "handle_package/handle_package.h"
#include "commands/helpers/color.h"
#include "build/build.h"
#include "commands.h"
#include <iostream>

void handle_error(const std::runtime_error& e) {
    std::cout << Color::red << e.what() << Color::reset << std::endl;
}

void check_args(int argc, int min_args, const std::string& usage) {
    if (argc < min_args) {
        std::cout << Color::green << usage << Color::reset << "\n";
        exit(1);
    }
}

void add_base_commands(CommandRegistry& registry) {
    Build build;
    HandlePackage handlePackage;
    registry.addCommand(Command("init", "Initialize a new package", [&handlePackage](int argc, char* argv[]) {
        // init requires exactly "cppkg init <name>"
        check_args(argc, 3, "Usage: cppkg init <project-name>");
        handlePackage.create_package(argv[2]);
    }));

    registry.addCommand(Command("add", "Add a dependency", [&handlePackage](int argc, char* argv[]) {
        check_args(argc, 3, "Usage: cppkg add <package>@<version>");
        handlePackage.add_dependency(argv[2]);
    }));

    registry.addCommand(Command("remove", "Remove a dependency", [&handlePackage](int argc, char* argv[]) {
        check_args(argc, 3, "Usage: cppkg remove <package>");
        handlePackage.remove_dependency(argv[2]);
    }));

    registry.addCommand(Command("build", "Build the package", [&build](int argc, char* argv[]) {
        if (argc >= 3) {
            build.build_project(argv[2]);
        } else {
            build.build_project(".");
        }
    }));

    registry.addCommand(Command("help", "Show help", [&registry](int argc, char* argv[]) {
        std::cout << Color::green << "Usage: cppkg <command> [args]\n\n" << Color::reset;
        std::cout << Color::cyan << "Available commands:\n";
        for (const Command& command : registry.commands) {
            std::cout << "    " << Color::yellow << command.name << Color::reset << " - " << command.description << "\n";
        }
        std::cout << "\n";
    }));

    registry.addCommand(Command("workspace", "Manage workspaces", [&handlePackage, &build](int argc, char* argv[]) {
        check_args(argc, 3, "Usage: cppkg workspace <init|build> <name>");
        std::string sub = argv[2];
        if (sub == "init") {
            check_args(argc, 4, "Usage: cppkg workspace init <name>");
            handlePackage.create_workspace(argv[3]);
        } else if (sub == "build") {
            if (argc >= 4) {
                build.build_project(argv[3]);
            } else {
                build.build_project(".");
            }
        }
    }));
}

int main(int argc, char* argv[]) {
    CommandRegistry registry;
    add_base_commands(registry);

    check_args(argc, 2, "Usage: cppkg <command>");

    try {
        registry.executeCommand(argv[1], argc, argv);
    } catch (const std::runtime_error& e) {
        handle_error(e);
        return 1;
    }

    return 0;
}
