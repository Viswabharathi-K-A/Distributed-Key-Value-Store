#include "raft_node.h"
#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>
#include <thread>


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
        request.set_lastlogindex(0); // TODO: set to actual last log index 
        request.set_lastlogterm(0);  // TODO: set to actual last log term

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
    }
}