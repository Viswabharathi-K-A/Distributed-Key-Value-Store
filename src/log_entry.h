#pragma once
#include "kv_store.h"

struct LogEntry {
    int term;
    Command command;

    LogEntry() : term(0), command(PutCommand{"", ""}) {}
    LogEntry(int term, Command command) : term(term), command(command) {}
};