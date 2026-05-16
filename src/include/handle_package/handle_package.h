#pragma once
#include <string>

class HandlePackage {
public:
    void create_package(const std::string& name);
    void create_workspace(const std::string& name);
    void add_dependency(const std::string& package);
    void remove_dependency(const std::string& package);
};
