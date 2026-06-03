#include "raft_service.h"
#include <sstream>

RaftServiceImpl::RaftServiceImpl(RaftNode& node) : node_(node) {}

grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context, const raft::RequestVoteRequest* request, raft::RequestVoteResponse* response) {
    
    // update term if candidate has higher term
    if (request->term() > node_.currentTerm) {
        node_.currentTerm = request->term();
        node_.votedFor = -1;
        node_.role = Role::Follower;
    }

    int myLastLogTerm = node_.log.empty() ? 0 : node_.log.back().term;
    int myLastLogIndex = node_.log.empty() ? 0 : (int)node_.log.size() - 1;
    response->set_term(node_.currentTerm);

    if (node_.votedFor == -1 || node_.votedFor == request->candidateid()) {
        if (request->term() >= node_.currentTerm) {
            if (myLastLogTerm < request->lastlogterm() || 
            (myLastLogTerm == request->lastlogterm() && myLastLogIndex <= request->lastlogindex())) {
                response->set_votegranted(true);
                node_.votedFor = request->candidateid();
            } else {
                response->set_votegranted(false);
            } 
        } else {
            response->set_votegranted(false);
        }    
    } else {
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
    std::string entry = request->entries(0);
    Command cmd = PutCommand{"", ""};
    std::istringstream ss(entry);
    std::string type;
    ss >> type;
    if (type != "PUT" && type != "DEL") {
    response->set_success(false);
    return grpc::Status::OK;
    }   
    if (type == "PUT") {
        std::string key, value;
        ss >> key >> value;
        cmd = PutCommand{key, value};
    } else if (type == "DEL") {
        std::string key;
        ss >> key;
        cmd = DeleteCommand{key};       
    }

    node_.log.push_back({request->term(), cmd});
    response->set_success(true);
    node_.persister_.save(node_.currentTerm, node_.votedFor, node_.log);
    return grpc::Status::OK;
}
