#include "include/handle_package/handle_package.h"
#include "include/build_project/build_project.h"
#include "include/dependency/dependency_service.h"
#include "include/version/version.h"
#include "helpers/color.h"
#include "commands.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>


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

/// oh look mom a git commit integration!
/// i commited war crimes. -- random dev in 2015
void git_integr_commit(int argc, char* argv[]) {
    check_arguments(argc, 4, "Usage: cppkg git commit <message>");
    GitService git;
    git.commit(argv[3]);
}

/// oh look mom a git push integration!
/// i pushed an old lady down the stairs --same random man but now from 2004
void git_integr_push(int argc, char* argv[]) {
    GitService git;
    /// The old lady? She never saw it coming.
    git.push();
}

/// fucking destroys your copmuter if it fails.
/// jk.
void git_integr_fail(int argc, char* argv[]) {
    std::cout << Color::red << "Unknown git command: " << argv[2] << Color::reset << std::endl;
    exit(1);
}

/// oh look mom a git integration!
void git_integration(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg git <command>");
    std::string argument = argv[2];

    if (argument == "commit") {
        git_integr_commit(argc, argv);
    } else if (argument == "push") {
        git_integr_push(argc, argv);
    } else {
        git_integr_fail(argc, argv);
    }
}

/*
 * just adds base commands to the registry
 * if you didnt already understand it idk whats in your brain
 */
void add_base_commands(CommandRegistry& registry) {
    Build build;
    HandlePackage handlePackage;

    registry.addCommand(Command("init", "Initialize a new package", [&handlePackage](int argc, char* argv[]) {
        check_arguments(argc, 3, "Usage: cppkg init <project-name>");
        handlePackage.create_package(argv[2]);
    }));

    registry.addCommand(Command("add", "Add a dependency (author/repo@version)", [](int argc, char* argv[]) {
        check_arguments(argc, 3, "Usage: cppkg add <author/repo>@<version>");
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
    }));

    registry.addCommand(Command("remove", "Remove a dependency", [](int argc, char* argv[]) {
        check_arguments(argc, 3, "Usage: cppkg remove <author/repo>");
        DependencyService deps;
        deps.remove(argv[2]);
    }));

    registry.addCommand(Command("build", "Build the package", [&build](int argc, char* argv[]) {
        std::string path = ".";
        if (argc >= 3) {
            path = argv[2];
        }
        build.build_project(path);
    }));

    registry.addCommand(Command("run", "Run the package", [&build](int argc, char* argv[]) {
        std::string path = ".";
        if (argc >= 3) {
            path = argv[2];
        }
        build.run_project(path);
    }));

    registry.addCommand(Command("clean", "Clean build artifacts and dependencies", [](int argc, char* argv[]) {
        std::string path = ".";
        if (argc >= 3) {
            path = argv[2];
        }
        DependencyService deps;
        deps.clean(path);
        std::cout << Color::green << "Cleaned build/target" << Color::reset << "\n";
    }));

    registry.addCommand(Command("help", "Show help", [&registry](int /*argc*/, char* /*argv*/[]) {
        std::cout << Color::green
                  << "Usage: cppkg <command> [args]\n\n"
                  << Color::reset;
        std::cout << Color::cyan << "Available commands:\n";

        // 1️⃣ Sort commands so the longest name appears first
        std::vector<Command> sorted = registry.commands;
        std::sort(sorted.begin(), sorted.end(), [](const Command& a, const Command& b) {
            return a.name.size() > b.name.size(); // descend
        });

        // get the longest d out the 3 :tongue:
        size_t max_len = 0;
        for (const auto& c : sorted) {
            max_len = std::max(max_len, c.name.size());
        }

        for (const Command& cmd : sorted) {
            std::cout << "   "
                << Color::yellow << cmd.name << Color::reset
                << " - " << cmd.description << '\n';
        }

        std::cout << '\n';
    }));

    registry.addCommand(Command("workspace", "Manage workspaces", [&handlePackage, &build](int argc, char* argv[]) {
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
    }));

    registry.addCommand(Command("version", "Print the version of cppkg", [](int argc, char* argv[]) {
        std::cout << Color::cyan << "cppkg commit: " << get_git_commit() << Color::reset << "\n";
    }));

    registry.addCommand(Command("git", "github integration inside of cppkg!", git_integration));
}

/// just look it up in google if you dont know what this does.
int main(int argc, char* argv[]) {
    CommandRegistry registry;
    add_base_commands(registry);

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