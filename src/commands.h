#pragma once

#include <string>
#include <functional>
#include <vector>
#include <stdexcept>


struct Command {
    std::string name;
    std::string description;
    std::function<void(int argc, char* argv[])> execute;

    Command(std::string name, std::string desc, std::function<void(int, char*[])> fn)
        : name(std::move(name)), description(std::move(desc)), execute(std::move(fn)) {}
};

struct CommandRegistry {
    std::vector<Command> commands;

    void addCommand(const Command& command) {
        commands.push_back(command);
    }

    void executeCommand(const std::string& name, int argc, char* argv[]) {
        for (const auto& command : commands) {
            if (command.name == name) {
                command.execute(argc, argv);
                return;
            }
        }
        throw std::runtime_error("Command not found: " + name);
    }
};
