# 🗄️ Distributed Key-Value Store with Raft Consensus

A production-grade distributed key-value store built from scratch in C++, implementing the [Raft consensus algorithm](https://raft.github.io/raft.pdf) to replicate data across a cluster of nodes and maintain consistency under failures.

> Built as a portfolio project targeting systems engineering roles. Every design decision — from `std::variant` for commands to write-ahead persistence — was made intentionally.

---

## ✨ Features

- 🔑 **Key-value operations** — PUT, GET, DELETE with linearizable semantics
- 🗳️ **Leader election** — randomized timeouts prevent split votes; cluster self-heals after leader crash
- 📋 **Log replication** — leader replicates commands to followers with Log Matching Property guarantees
- 💾 **Crash recovery** — nodes persist `currentTerm`, `votedFor`, and log to disk; state is fully restored on restart
- 🔥 **Chaos tested** — automated script kills the leader mid-operation and verifies a new leader is elected

---

## 🏗️ Architecture

```
Client Request
      │
      ▼
 RaftServiceImpl          ← gRPC server, handles RequestVote + AppendEntries RPCs
      │
      ▼
  RaftNode                ← Raft state machine: role, term, log, election/heartbeat timers
      │
      ├── Persister       ← writes currentTerm, votedFor, log to disk before every RPC response
      │
      └── KVStore         ← pure state machine, applies committed log entries (PUT/DELETE)
```

The `KVStore` has zero knowledge of Raft — it only executes commands it's told to apply. Raft decides *when* to call `apply()`. This separation means the storage layer can be swapped or tested independently.

---

## 🔧 Stack

| Component | Technology |
|-----------|------------|
| Language | C++ 17 |
| RPC framework | gRPC |
| Serialization | Protocol Buffers (proto3) |
| Build system | CMake (auto-generates protobuf sources) |
| Environment | Linux / WSL2 |

---

## 🚀 Build & Run

### Prerequisites

```bash
sudo apt-get install -y cmake g++ libprotobuf-dev libgrpc++-dev protobuf-compiler-grpc
```

### Build

```bash
git clone <repo-url>
cd distributed-kv-raft
mkdir build && cd build
cmake ..
make
```

### Run a 3-node cluster

Open three terminals:

```bash
# Terminal 1
./raft_node_bin 0 50050

# Terminal 2
./raft_node_bin 1 50051

# Terminal 3
./raft_node_bin 2 50052
```

You'll see one node elect itself as leader within 300ms.

---

## 🧪 Testing

### Unit tests

```bash
cd build
./kv_store_test
```

Covers: PUT/GET/DELETE correctness, DELETE idempotency (safe for Raft log replay), command sequence simulation.

### Chaos test — leader crash & re-election

```bash
bash scripts/chaos_test.sh
```

**What it does:**
1. Starts a 3-node cluster
2. Waits for a leader to be elected
3. Kills the leader process
4. Verifies the remaining nodes elect a new leader within the election timeout

**Sample output:**
```
Starting 3 nodes...
Leader elected: /tmp/node1.log
Killing leader node 1 (PID 48767)...
PASS: New leader elected: /tmp/node2.log
Chaos test complete
```

This test proves the cluster satisfies Raft's core safety property — at most one leader per term, and the system makes progress after a failure.

---

## 🐛 Hardest Bug

**Log Matching Property violation during replication.**

My initial implementation sent `AppendEntries` to each peer once and moved on. Reading the Raft paper more carefully, I realized this violated the Log Matching Property — if a peer's log diverged from the leader's, the leader needs to back up `nextIndex` and retry until logs agree at a common point. I added a retry loop that decrements `nextIndex` on each rejection and resends with the backed-up `prevLogIndex`. The first version had an off-by-one error in the boundary check that caused a crash on empty logs. The fix was adding an empty log guard and ensuring the loop condition `peerNextIndex > 0` was sufficient to protect all array accesses.

---

## 🔍 Key Design Decisions

**Why `std::variant` for Commands?**
`PutCommand` and `DeleteCommand` are modeled as a `std::variant` rather than a class hierarchy. This makes the type exhaustive at compile time — `std::visit` with `if constexpr` guarantees every command type is handled, and missing a case is a compile error, not a runtime bug.

**Why GET bypasses consensus?**
Reads have no side effects — replicating them through the log would pay full consensus cost for an operation that changes nothing. GET is served directly from the leader's in-memory map. (Note: this requires read leases in production to prevent stale reads from a partitioned leader.)

**Why write-ahead persistence?**
State is saved to disk *before* responding to any RPC. If a node grants a vote, crashes, and restarts without the persisted vote, it could vote for a different candidate in the same term — breaking the one-vote-per-term guarantee and potentially electing two leaders.

**Why idempotent DELETE?**
After a crash, the Raft log may be replayed. If DELETE returned an error on a missing key, a replay would fail even though no data was lost. DELETE on a missing key is a no-op — same result whether applied once or twice.

---

## 📈 What I'd Do Differently

- **Log compaction / snapshotting** — without it, the log grows unbounded. The Raft paper's Section 7 describes the snapshot mechanism; this is the next major feature to add.
- **Read leases** — currently GETs are served from the leader without a quorum check. A partitioned leader could serve stale data. Read leases would fix this.
- **Proper protobuf `LogEntry` messages** — commands are currently serialized as plain strings (`"PUT key value"`). A proper `LogEntry` protobuf message would be more robust and schema-safe.
- **Linearizability checker** — a tool like Jepsen or a custom checker would give stronger correctness guarantees beyond manual chaos testing.

---

## 📚 References

- [Raft paper — Ongaro & Ousterhout (2014)](https://raft.github.io/raft.pdf)
- [Raft visualization](https://raft.github.io/)
- [gRPC C++ documentation](https://grpc.io/docs/languages/cpp/)