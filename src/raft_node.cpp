#include "raft_node.h"
#include "raft.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>


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