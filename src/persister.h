#pragma once
#include "log_entry.h"
#include <string>
#include <vector>

class Persister {
    public:
        Persister(const std::string& filename);

        void save(int currentTerm, int votedFor, const std::vector<LogEntry>& log);
        void load(int& currentTerm, int& votedFor, std::vector<LogEntry>& log);

    private:
        std::string filename_;
};