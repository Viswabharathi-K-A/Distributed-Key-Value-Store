#pragma once
#include "raft.grpc.pb.h"
#include "raft_node.h"


class RaftServiceImpl final : public raft::RaftService::Service {
    public:
    explicit RaftServiceImpl(RaftNode& node);

    grpc::Status RequestVote(grpc::ServerContext* context, const raft::RequestVoteRequest* request, raft::RequestVoteResponse* response) override;

    grpc::Status AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) override;

    private:
    RaftNode& node_;
};

