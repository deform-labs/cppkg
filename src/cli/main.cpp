#include "../cppkg_engine/cppkg_engine.h"
#include "../api/version/version.h"
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <string>


cppkg::HandlePackage handlePackage;
cppkg::command_system cmd_;
cppkg::GithubClient git;
cppkg::Build build;

CommandRegistry registry;

/// dumbfuck if you cant understand ts
void handle_error(const std::runtime_error& e) {
    std::cout << cppkg::ux::color::red << e.what() << cppkg::ux::color::reset << std::endl;
}

/// ok so this one might need an explanation but it's pretty self-explanatory for amateurs B)
/// for dumbasses: it checks the amount of arguments against the minimum required
void check_arguments(int argc, int min_args, const std::string& usage) {
    if (argc < min_args) {
        std::cout << cppkg::ux::color::green << usage << cppkg::ux::color::reset << "\n";
        exit(1);
    }
}

/// OOH COMMANDS OHHH
/// Had to fix these comments because on github they looked off :|

/// workspaces everywhere employment for everbody!
void* workspace_command(int argc, char* argv[]) {
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
};

/// actually fucking destroys your pc (doesnt do that obv it just segmentation faults)
void* crash_pc_command(int argc, char* argv[]) {
    (void)argc; (void)argv;
    int* ptr = nullptr;
    *ptr = -1;
    return ptr;
};

/// commit number WHAT?
void* version_command(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << cppkg::ux::color::cyan << "cppkg version " << get_git_commit() << cppkg::ux::color::reset << std::endl;
    return nullptr;
};

/// remove command lmao ima remove this spike from your ass
void* remove_command(int argc, char* argv[]) {
    (void)argc;
    std::string name = argv[2];
    cppkg::dependency_impl deps;
    deps.remove(name);
    return nullptr;
};

/// cleans yo buh hole
void* clean_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) path = argv[2];
    else (void)argv;
    cppkg::dependency_impl deps;
    deps.clean(path);
    std::cout << cppkg::ux::color::green << "Cleaned build/target" << cppkg::ux::color::reset << "\n";
    return nullptr;
};

/// builds the project
void* Build_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) path = argv[2];
    else (void)argv;
    build.build_project(path);
    return nullptr;
};

/// helpp please!!!
void* help_command(int argc, char* argv[]) {
    (void)argc; (void)argv;
    std::cout << cppkg::ux::color::green << "Usage: cppkg <command> <args>\n\n" << cppkg::ux::color::reset;
    std::cout << cppkg::ux::color::cyan << "Available commands:\n" << cppkg::ux::color::reset;

    if (registry.commands.empty()) {
        std::cout << "   " << cppkg::ux::color::red << "(no commands registered)" << cppkg::ux::color::reset << "\n";
        return nullptr;
    }

    // this comment was nuked by the turtle
    size_t max_len = 0;
    for (const auto& cmd : registry.commands)
        max_len = std::max(max_len, cmd.name.size());

    /// I CHOOSE DEATH!
    for (const Command& cmd : registry.commands) {
        std::string padding(max_len - cmd.name.size() + 2, ' ');
        std::cout << "   " << cppkg::ux::color::yellow << cmd.name << cppkg::ux::color::reset
                  << padding << "- " << cmd.description << "\n";
    }

    std::cout << "\n";
    return nullptr;
};

/// initialize this ahh
void* init_command(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg init <project-name>");
    handlePackage.create_package(argv[2]);
    return nullptr;
};

/// oh look mom a git integration!
void* git_command(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg git <command>");
    std::string argument = argv[2];
    std::string git_command_help = std::string("Git integration for cppkg. ") +
        "\nUsage: cppkg git <command> \n" +
        "NOTE: this is just a passthrough. no real functionality is in cppkg. \n" +
        "meanwhile here is a list of available commands to use in cppkg git (NOT AN IMPLEMENTATION): \n" +
        "   commit - save a change to github. \n" +
        "          -m <message> - commit message. \n" +
        "          -a - add all changes. \n" +
        "          -v - verbose output. \n" +
        "   push - push changes to github. for flags use git --help. \n" +
        "   pull - pull changes from github. for flags use git --help. \n" +
        "   status - show the status of the repository. for flags use git --help. \n" +
        "   log - show the commit history. for flags use git --help. \n" +
        "   branch - show the current branch. for flags use git --help. \n" +
        "   checkout - switch to a different branch. for flags use git --help. \n" +
        "   merge - merge changes from one branch to another. for flags use git --help. \n" +
        "   add - add files to the staging area. for flags use git --help. \n" +
        "   rm - remove files from the staging area. for flags use git --help. \n" +
        "\n";
    if (argument == "help") {
        std::cout << cppkg::ux::color::cyan << git_command_help;
        std::cout << cppkg::ux::color::reset << "\n";
        return nullptr;
    }
    cmd_.run("git " + argument);
    return nullptr;
};

/// add command lmao ima add this spike up your ass
void* add_command(int argc, char* argv[]) {
    bool https = false;
    std::string spec = argv[2];
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--https") https = true;
        if (std::string(argv[i]) == "--help")
            std::cout << cppkg::ux::color::cyan
                      << "Add a dependency to the workspace or to the package.\n"
                      << " Usage: cppkg add <author/repo>@<version>"
                      << cppkg::ux::color::reset << "\n";
    }
    cppkg::dependency_impl deps(https);
    deps.add(spec);
    return nullptr;
};

/// runs the project
void* run_command(int argc, char* argv[]) {
    std::string path = ".";
    if (argc >= 3) path = argv[2];
    else (void)argv;
    build.run_project(path);
    return nullptr;
};

/// search for a repo on github
void* search_command(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg search <query>");
    cppkg::GithubClient git;
    auto results = git.search(argv[2]);
    if (results.empty()) {
        std::cout << cppkg::ux::color::red << "No results found for: " << argv[2] << cppkg::ux::color::reset << "\n";
        return nullptr;
    }
    for (const auto& r : results) {
        std::cout << "   " << cppkg::ux::color::yellow << r.full_name << cppkg::ux::color::reset
                  << " - " << r.description
                  << cppkg::ux::color::cyan << " Stars: " << r.stars << cppkg::ux::color::reset << "\n";
    }
    return nullptr;
};

/// finally a publish command
void* publish_command(int argc, char* argv[]) {
    check_arguments(argc, 3, "Usage: cppkg publish <version> --message <msg>");

    std::string version = argv[2];
    std::string message = "Release v" + version;

    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--message" && i + 1 < argc)
            message = argv[i + 1];
        if (std::string(argv[i]) == "--notes" && i + 1 < argc) {
            std::ifstream f(argv[i + 1]);
            if (f.is_open())
                message = std::string((std::istreambuf_iterator<char>(f)),
                                       std::istreambuf_iterator<char>());
        }
    }

    const char* token_env = std::getenv("CPPKG_TOKEN");
    if (!token_env) {
        std::cout << cppkg::ux::color::red
                  << "No GitHub token found!\n"
                  << "Set CPPKG_TOKEN environment variable:\n"
                  << "  export CPPKG_TOKEN=your_token_here\n"
                  << cppkg::ux::color::reset;
        return nullptr;
    }

    git.publish(version, message, token_env);
    return nullptr;
};

/*
 * just adds base commands to the registry
 * if you didnt already understand it idk whats in your brain
 */
void INITIALIZE(CommandRegistry& registry) {
    //descending spiral. just how i like it.
    registry.addCommand(Command("crash",     "crash the pc (JOKE. just crashes the executable.)", crash_pc_command));
    registry.addCommand(Command("clean",     "Clean build artifacts and dependencies",            clean_command));
    registry.addCommand(Command("search",    "Search for a dependency on github",                 search_command));
    registry.addCommand(Command("add",       "Add a dependency (author/repo@version)",            add_command));
    registry.addCommand(Command("publish",   "Publish the package to github",                     publish_command));
    registry.addCommand(Command("git",       "github integration inside of cppkg!",               git_command));
    registry.addCommand(Command("version",   "Print the version of cppkg",                        version_command));
    registry.addCommand(Command("workspace", "Manage workspaces",                                  workspace_command));
    registry.addCommand(Command("init",      "Initialize a new package",                           init_command));
    registry.addCommand(Command("remove",    "Remove a dependency",                                remove_command));
    registry.addCommand(Command("build",     "Build the package",                                  Build_command));
    registry.addCommand(Command("run",       "Run the package",                                    run_command));
    registry.addCommand(Command("help",      "Show help",                                          help_command));
}

/// just look it up in google if you dont know what this does.
int main(int argc, char* argv[]) {
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
