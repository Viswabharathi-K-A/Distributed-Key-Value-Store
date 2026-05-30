#include "raft_node.h"
#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>



RaftNode::RaftNode (int id, std::vector<int> peers, std::unordered_map<int, std::string> peerAddresses, KVStore& kv, const std::string& persistPath)
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
{
    // Load persisted state
    persister_.load(currentTerm, votedFor, log);
}


void RaftNode::startElection() {
    role = Role::Candidate;     // become candidate
    currentTerm ++;             // increment term
    votedFor = this->id;    
    votesReceived = 1;          // vote for self
    int majority = (peers.size() + 1)/2 + 1;
    for (int peerId : peers) {
        auto channel = grpc::CreateChannel(
            peerAddresses[peerId],
            grpc::InsecureChannelCredentials()
        );
        auto stub = raft::RaftService::NewStub(channel);

        raft::RequestVoteRequest request;
        request.set_term(currentTerm);
        request.set_candidateid(id);
        request.set_lastlogindex(log.empty() ? 0 : (int)log.size() - 1);
        request.set_lastlogterm(log.empty() ? 0 : log.back().term);

        raft::RequestVoteResponse response;
        grpc::ClientContext context;

        grpc::Status status = stub->RequestVote(&context, request, &response);

        if (status.ok() && response.votegranted()) {
            votesReceived++;
            if (votesReceived >= majority) {
                role = Role::Leader;
                std::cout << " Node " <<id << " becomes leader for term " << currentTerm << "\n"; 
                break;
            }

        }

    }

    std::cout << " Node " <<id << " starting election for term " << currentTerm << "\n";

}

void RaftNode::runElectionTimer() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastHeartbeat
        );

        if (role != Role::Leader && elapsed > electionTimeout) {
            startElection();
            lastHeartbeat = std::chrono::steady_clock::now(); //reset timer
        }
    }
}

void RaftNode::sendHeartbeats(){
    for (int peerId : peers) {
        auto channel = grpc::CreateChannel(
            peerAddresses[peerId],
            grpc::InsecureChannelCredentials()
        );
        auto stub = raft::RaftService::NewStub(channel);

        raft::AppendEntriesRequest request;
        request.set_term(currentTerm);
        request.set_leaderid(id);

        raft::AppendEntriesResponse response;
        grpc::ClientContext context;
        grpc::Status status = stub->AppendEntries(&context, request, &response);

        if (status.ok() && response.term() > currentTerm) {
            currentTerm = response.term();
            role = Role::Follower;
        } 

    }
}


void RaftNode::runHeartbeatTimer() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (role == Role::Leader) {
            sendHeartbeats();
        }
        applyCommittedEntries(kv);
    }
}

bool RaftNode::replicateCommand(const Command& cmd) {
    if (role != Role::Leader) {
        std::cout << "Node " << id << " is not leader. Cannot replicate command.\n";
        return false;
    }
    int majority = (peers.size() + 1)/2 + 1;
    
    log.push_back({currentTerm, cmd});

    for ( int peerId : peers) {

        int peerNextIndex = log.empty() ? 0 : (int)log.size() - 1;

        while (peerNextIndex > 0) {

            auto channel = grpc::CreateChannel(
                peerAddresses[peerId],
                grpc::InsecureChannelCredentials()
            );
            auto stub = raft::RaftService::NewStub(channel);

            raft::AppendEntriesRequest request;
            request.set_term(currentTerm);
            request.set_leaderid(id);
            request.set_prevlogindex(peerNextIndex-1); // index of log entry before new entry
            request.set_prevlogterm(log[peerNextIndex-1].term); // term of log entry before new entry

            std::string entry;
            std::visit([&entry](const auto& cmd) {
                using T = std::decay_t<decltype(cmd)>;
                if constexpr (std::is_same_v<T, PutCommand>) {
                    entry = "PUT " + cmd.key + " " + cmd.value;
                } else if constexpr (std::is_same_v<T, DeleteCommand>) {
                    entry = "DEL " + cmd.key;
                }
            }, log[peerNextIndex].command);
            request.add_entries(entry);
            request.set_leadercommit(commitIndex);

            raft::AppendEntriesResponse response;
            grpc::ClientContext context;

            grpc::Status status = stub->AppendEntries(&context, request, &response);

            if (status.ok() && response.success()) {
                matchIndex[peerId] = peerNextIndex;
                nextIndex[peerId] = peerNextIndex + 1;
                break;
            } else if (status.ok() && response.term() > currentTerm) {
                currentTerm = response.term();
                role = Role::Follower;
                return false;
            } else {
                nextIndex[peerId] = --peerNextIndex;             
            }    
        }
    }

    int replicated = 1; // count self
    for (auto& pair : matchIndex) {
        if (pair.second >= (int)log.size() - 1) replicated++;
    }
    if (replicated >= majority) {
        commitIndex = log.size() - 1;
        std::cout << "Command committed at index " << commitIndex << "\n";
        return true;
    } else {
        return false;
    }

}

void RaftNode::applyCommittedEntries(KVStore& kv) {
    while (lastApplied < commitIndex) {
        lastApplied++;
        LogEntry& entry = log[lastApplied];
        ApplyResult result = kv.apply(entry.command);
        std::cout << "Applied log entry " << lastApplied << ": " << result.message << "\n";
    }
}