#pragma once
#include "log_entry.h"
#include "kv_store.h"
#include "persister.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

enum class Role {
    Follower,
    Candidate,
    Leader
};

struct RaftNode {
    int              id;
    std::vector<int> peers;
    std::unordered_map<int, std::string> peerAddresses;
    Role             role;
    int              currentTerm;
    int              votedFor;
    int              votesReceived;
    std::chrono::milliseconds electionTimeout;
    std::chrono::steady_clock::time_point lastHeartbeat;
    std::vector<LogEntry> log;
    int commitIndex;                                        // highest log entry known to be commited
    int lastApplied;                                        // highest log entry applied to state machine                                       
    std::unordered_map<int, int> nextIndex;
    std::unordered_map<int, int> matchIndex;
    KVStore& kv;
    Persister persister_;


    RaftNode (int id, std::vector<int> peers, std::unordered_map<int, std::string> peerAddresses, KVStore& kv, const std::string& persistPath)
        : id(id)
        , peers (peers)
        , peerAddresses (peerAddresses)
        , kv(kv)
        , persister_(persistPath)
        , role (Role::Follower)
        , currentTerm (0)
        , votedFor (-1)
        , votesReceived (0)
        , electionTimeout (std::chrono::milliseconds(150 + rand() % 150))
        , lastHeartbeat (std::chrono::steady_clock::now())
        , commitIndex (0)
        , lastApplied (0)
        , nextIndex({})
        , matchIndex({})
    {}

    void startElection();

    void runElectionTimer();

    void sendHeartbeats();

    void runHeartbeatTimer();

    bool replicateCommand(const Command& cmd);

    void applyCommittedEntries(KVStore& kv);
};