#ifndef COMMAND_H
    #define COMMAND_H
    #include <functional>
    #include <string>

    /// C O M M A N D
    struct Command {
        std::string name;
        std::string description;

        //for beginners this just gives you a way to run a function with arguments
        std::function<void(int argc, char* argv[])> execute;

        Command(std::string name, std::string desc, std::function<void*(int, char*[])> fn)
            : name(std::move(name)), description(std::move(desc)), execute(std::move(fn)) {}
    };
#endif