#include "raft_service.h"

RaftServiceImpl::RaftServiceImpl(RaftNode& node) : node_(node) {}

grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context, const raft::RequestVoteRequest* request, raft::RequestVoteResponse* response) {
    // TODO: implement in Phase 3
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) {
    // TODO: implement in Phase 3
    return grpc::Status::OK;
}