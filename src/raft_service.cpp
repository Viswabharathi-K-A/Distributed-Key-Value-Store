#include "raft_service.h"

RaftServiceImpl::RaftServiceImpl(RaftNode& node) : node_(node) {}

grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context, const raft::RequestVoteRequest* request, raft::RequestVoteResponse* response) {
    response->set_term(node_.currentTerm);
    if (node_.votedFor == -1 || node_.votedFor == request->candidateid()) {
        if (request->term() >= node_.currentTerm) {
            response->set_votegranted(true);
            node_.votedFor = request->candidateid();
        } else {
            response->set_votegranted(false);
        }
    }     else {
        response->set_votegranted(false);
    } 
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) {
    response->set_term(node_.currentTerm);
    if (request->term() < node_.currentTerm) {
        response->set_success(false);
    } else {
        node_.currentTerm =request->term();
        node_.role = Role::Follower;
        node_.lastHeartbeat = std::chrono::steady_clock::now();
        response->set_success(true);   
    }
    return grpc::Status::OK;
}