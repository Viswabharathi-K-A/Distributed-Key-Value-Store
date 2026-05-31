#include <fstream>
#include "persister.h"


Persister::Persister(const std::string& filename) : filename_(filename) {}

void Persister::save(int currentTerm, int votedFor, const std::vector<LogEntry>& log) {
    std::ofstream out(filename_, std::ios::binary);
    out.write(reinterpret_cast<const char*>(&currentTerm), sizeof(currentTerm));
    out.write(reinterpret_cast<const char*>(&votedFor), sizeof(votedFor));
    size_t logSize = log.size();
    out.write(reinterpret_cast<const char*>(&logSize), sizeof(logSize));
    for (const auto& entry : log) {
        out.write(reinterpret_cast<const char*>(&entry.term), sizeof(entry.term));
        std::visit([&out](const auto& cmd) {
            using T = std::decay_t<decltype(cmd)>;
            if constexpr (std::is_same_v<T, PutCommand>) {
                int type = 0;
                out.write(reinterpret_cast<const char*>(&type), sizeof(type));
                // write key
                int keyLen = cmd.key.size();
                out.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
                out.write(cmd.key.data(), keyLen);
                // write value
                int valLen = cmd.value.size();
                out.write(reinterpret_cast<const char*>(&valLen), sizeof(valLen));
                out.write(cmd.value.data(), valLen);
            } else if constexpr (std::is_same_v<T, DeleteCommand>) {
                int type = 1;
                out.write(reinterpret_cast<const char*>(&type), sizeof(type));
                // write key
                int keyLen = cmd.key.size();
                out.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
                out.write(cmd.key.data(), keyLen);
            }
        }, entry.command);
    }
}

void Persister::load(int& currentTerm, int& votedFor, std::vector<LogEntry>& log) {
    std::ifstream in(filename_, std::ios::binary);
    if (!in.is_open()) return; // no file yet — first startup
    
    in.read(reinterpret_cast<char*>(&currentTerm), sizeof(currentTerm));
    in.read(reinterpret_cast<char*>(&votedFor), sizeof(votedFor));
    size_t logSize;
    in.read(reinterpret_cast<char*>(&logSize), sizeof(logSize));
    log.clear();
    for (size_t i = 0; i < logSize; ++i) {
        LogEntry entry;
        in.read(reinterpret_cast<char*>(&entry.term), sizeof(entry.term));
        int cmdType;
        in.read(reinterpret_cast<char*>(&cmdType), sizeof(cmdType));
        if (cmdType == 0) { // PutCommand
            PutCommand cmd;
            int keyLen;
            in.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
            cmd.key.resize(keyLen);
            in.read(cmd.key.data(), keyLen);
            int valLen;
            in.read(reinterpret_cast<char*>(&valLen), sizeof(valLen));
            cmd.value.resize(valLen);
            in.read(cmd.value.data(), valLen);
            entry.command = cmd;
        } else if (cmdType == 1) { // DeleteCommand
            DeleteCommand cmd;
            int keyLen;
            in.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
            cmd.key.resize(keyLen);
            in.read(cmd.key.data(), keyLen);
            entry.command = cmd;
        }
        log.push_back(entry);
    }
}