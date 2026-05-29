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
    node_.persister_.save(node_.currentTerm, node_.votedFor, node_.log);
    return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context, const raft::AppendEntriesRequest* request, raft::AppendEntriesResponse* response) {
    response->set_term(node_.currentTerm);
    
    if (request->term() < node_.currentTerm) {
        response->set_success(false);
        return grpc::Status::OK;
    }
    
    node_.currentTerm = request->term();
    node_.role = Role::Follower;
    node_.lastHeartbeat = std::chrono::steady_clock::now();

    // heartbeat - no entries to append
    if (request->entries_size() == 0) {
        response->set_success(true);
        return grpc::Status::OK;
    }

    // consistency check
    if (request->prevlogindex() > 0) {
        if (request->prevlogindex() >= (int)node_.log.size() ||
            node_.log[request->prevlogindex()].term != request->prevlogterm()) {
            response->set_success(false);
            return grpc::Status::OK;
        }
    }

    // append entries
    node_.log.resize(request->prevlogindex() + 1);
    node_.log.push_back({request->term(), Command{PutCommand{"placeholder", "placeholder"}}});
    response->set_success(true);
    node_.persister_.save(node_.currentTerm, node_.votedFor, node_.log);
    return grpc::Status::OK;
}
