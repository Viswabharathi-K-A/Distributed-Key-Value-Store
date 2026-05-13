#pragma once
#include <string>
#include <vector>
#include <chrono>

enum class Role {
    Follower,
    Candidate,
    Leader
};

struct RaftNode {
    int              id;
    std::vector<int> peers;
    Role             role;
    int              currentTerm;
    int              votedFor;
    int              votesReceived;
    std::chrono::milliseconds electionTimeout;
    std::chrono::steady_clock::time_point lastHeartbeat;

    RaftNode (int id, std::vector<int> peers)
        : id(id)
        , peers (peers)
        , role (Role::Follower)
        , currentTerm (0)
        , votedFor (-1)
        , votesReceived (0)
        , electionTimeout (std::chrono::milliseconds(150 + rand() % 150))
        , lastHeartbeat (std::chrono::steady_clock::now())
    {}

    void startElection();
};