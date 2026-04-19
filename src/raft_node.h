#pragma once
#include <string>
#include <vector>

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

    RaftNode (int id, std::vector<int> peers)
        : id(id)
        , peers (peers)
        , role (Role::Follower)
        , currentTerm (0)
        , votedFor (-1)
    {}
};