#include "raft_node.h"
#include <iostream>

void RaftNode::startElection() {
    role = Role::Candidate;     // become candidate
    currentTerm ++;             // increment term
    votedFor = this->id;    
    votesReceived = 1;          // vote for self

    std::cout << " Node " <<id << " starting election for term " << currentTerm << "\n";

}