#pragma once
#include <unordered_map>
#include <optional>
#include <string>
#include <variant>

struct PutCommand {
    std::string key;
    std::string value;
};

struct DeleteCommand {
    std::string key;
};

using Command = std::variant<PutCommand, DeleteCommand>;


struct ApplyRsult {
    bool success;
    std::string message;
}



class KVStore {
    public:
    ApplyResult apply(const Command& cmd);
    std::optional<std::string> get(const std::string& key) const;
    
    private:
    std::unordered_map<std::string, std::string> data_;
};
