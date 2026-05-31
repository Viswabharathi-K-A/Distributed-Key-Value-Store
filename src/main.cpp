#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>

#include <grpcpp/grpcpp.h>

#include "kv_store.h"
#include "raft_node.h"
#include "raft_service.h"
#include "raft.grpc.pb.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <node_id> <port>\n";
        return 1;
    }

    int nodeId = std::stoi(argv[1]);
    std::string port = argv[2];

    std::vector<int> peers;
    std::unordered_map<int, std::string> peerAddresses;

    std::unordered_map<int, std::string> allNodes = {
        {0, "localhost:50050"},
        {1, "localhost:50051"},
        {2, "localhost:50052"},
        {3, "localhost:50053"},
        {4, "localhost:50054"}
    };

    for (auto& [id, addr] : allNodes) {
        if (id != nodeId) {
            peers.push_back(id);
            peerAddresses[id] = addr;
        }
    }

    std::string persistPath = "node_" + std::to_string(nodeId) + ".log";

    KVStore kvStore;
    RaftNode node(nodeId, peers, peerAddresses, kvStore, persistPath);
    RaftServiceImpl service(node);

    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:" + port, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    if (!server) {
        std::cerr << "Failed to start server on port " << port << "\n";
        return 1;
    }

    std::cout << "Node " << nodeId << " running on port " << port << "\n";

    std::thread electionThread(&RaftNode::runElectionTimer, &node);
    std::thread heartbeatThread(&RaftNode::runHeartbeatTimer, &node);

    electionThread.detach();
    heartbeatThread.detach();

    server->Wait();

    return 0;
}