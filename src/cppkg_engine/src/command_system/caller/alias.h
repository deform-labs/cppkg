#ifndef CPPKG_ALIAS_H
    #define CPPKG_ALIAS_H
    #include <filesystem>
    #include <istream>
    #include <fstream>
    #include <unordered_map>
    #include <string>

    namespace cppkg {
        class alias {
        public:
            static alias& get() {
                static alias instance;
                return instance;
            }

            void add_alias(const std::string& name, const std::string& command) {
                aliases_[name] = command;
                save_to_file();
            }

            void remove_alias(const std::string& name) {
                aliases_.erase(name);
                save_to_file();
            }

            std::string get_command(const std::string& alias) {
                auto it = aliases_.find(alias);
                return it != aliases_.end() ? it->second : "";
            }

            bool is_alias(const std::string& name) {
                return aliases_.find(name) != aliases_.end();
            }

            const std::unordered_map<std::string, std::string>& get_all() {
                return aliases_;
            }

        private:
            alias() { load_from_file(); }

            void save_to_file() {
                std::filesystem::create_directories(".cppkg");
                std::ofstream file(".cppkg/aliases.txt");
                for (const auto& [name, cmd] : aliases_) {
                    file << name << " " << cmd << "\n";
                }
            }

            void load_from_file() {
                std::filesystem::create_directories(".cppkg");
                std::ifstream file(".cppkg/aliases.txt");
                if (!file) return;
                std::string name, cmd;
                while (file >> name >> std::ws && std::getline(file, cmd)) {
                    aliases_[name] = cmd;
                }
            }

            std::unordered_map<std::string, std::string> aliases_;
        };
    }
#endif