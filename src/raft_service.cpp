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
    // TODO: implement in Phase 3
    return grpc::Status::OK;
}