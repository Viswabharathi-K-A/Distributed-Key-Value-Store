#pragma once
#include "kv_store.h"

struct LogEntry {
    int term;
    Command command;
};