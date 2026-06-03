#!/bin/bash
BUILD=~/distributed-kv-raft/build

# clean up old state
rm -f $BUILD/node_*.log
rm -f /tmp/node*.log

# start nodes in background
$BUILD/raft_node_bin 0 50050 > /tmp/node0.log 2>&1 &
PID0=$!

$BUILD/raft_node_bin 1 50051 > /tmp/node1.log 2>&1 &
PID1=$!

$BUILD/raft_node_bin 2 50052 > /tmp/node2.log 2>&1 &
PID2=$!

echo "Starting 3 nodes..."
sleep 6

# find which node became leader
LEADER_LOG=$(grep -l "becomes leader" /tmp/node0.log /tmp/node1.log /tmp/node2.log 2>/dev/null | head -1)

if [ -z "$LEADER_LOG" ]; then
    echo "FAIL: No leader elected"
    kill $PID0 $PID1 $PID2
    cat /tmp/node0.log
    cat /tmp/node1.log
    cat /tmp/node2.log
    exit 1
fi

echo "Leader elected: $LEADER_LOG"

# extract node number from log filename (node0.log -> 0)
LEADER_NODE=$(echo $LEADER_LOG | grep -o '[0-9]')

# map node number to PID
if [ "$LEADER_NODE" == "0" ]; then LEADER_PID=$PID0
elif [ "$LEADER_NODE" == "1" ]; then LEADER_PID=$PID1
else LEADER_PID=$PID2
fi

echo "Killing leader node $LEADER_NODE (PID $LEADER_PID)..."
kill $LEADER_PID
sleep 6

# find which node became leader after killing the first leader
NEW_LEADER=$(grep -l "becomes leader" /tmp/node0.log /tmp/node1.log /tmp/node2.log 2>/dev/null | grep -v "$LEADER_LOG" | head -1)

if [ -z "$NEW_LEADER" ]; then
    echo "FAIL: No leader elected"
    kill $PID0 $PID1 $PID2
    exit 1
fi

echo "PASS: New leader elected: $NEW_LEADER"

# cleanup
kill $PID0 $PID1 $PID2 2>/dev/null
echo "Chaos test complete"



