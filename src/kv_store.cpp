# include "kv_store.h"


std::optional<std::string> KVStore::get(const std::string& key) const {
    auto it = data_.find(key);
    if (it == data_.end()) return std::nullopt;
    return it->second;
}

ApplyResult KVStore::apply(const Command& cmd) {
    return std::visit([this](const auto& c) -> ApplyResult {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, PutCommand>) {
            data_[c.key] = c.value;
            return {true,"OK"};
        }

        else if constexpr (std::is_same_v<T, DeleteCommand>) {
            auto it = data_.find(c.key);
            if (it == data_.end()) return {true,"Key not found - no-op"};
            data_.erase(c.key);
            return {true,"OK"}; 
        }
    }, cmd);
}
