#include <iostream>
#include "commands.h"
#include "handle_package/handle_package.h"

void handle_error(const std::runtime_error& e) {
    std::cerr << e.what() << std::endl;
}

void check_args(int argc, int min_args, const std::string& usage) {
    if (argc < min_args) {
        std::cerr << usage << "\n";
        exit(1);
    }
}

void add_base_commands(CommandRegistry& registry) {
    registry.addCommand(Command("init", "Initialize a new package", [](int argc, char* argv[]) {
        // init requires exactly "cppkg init <name>"
        check_args(argc, 3, "Usage: cppkg init <project-name>");
        HandlePackage handler;
        handler.create_package(argv[2]);
    }));

    registry.addCommand(Command("add", "Add a dependency", [](int argc, char* argv[]) {
        // add requires exactly "cppkg add <package>@<version>"
        check_args(argc, 3, "Usage: cppkg add <package>@<version>");

        std::cout << "Adding: " << argv[2] << "\n";
    }));

    registry.addCommand(Command("remove", "Remove a dependency", [](int argc, char* argv[]) {
        // remove requires exactly "cppkg remove <package>@<version>"
        check_args(argc, 3, "Usage: cppkg remove <package>@<version>");

        std::cout << "Removing: " << argv[2] << "\n";
    }));
}

int main(int argc, char* argv[]) {
    CommandRegistry registry;
    add_base_commands(registry);

    // the command name is required
    check_args(argc, 2, "Usage: cppkg <command>");

    try {
        registry.executeCommand(argv[1], argc, argv);
    } catch (const std::runtime_error& e) {
        handle_error(e);
        return 1;
    }

    return 0;
}
