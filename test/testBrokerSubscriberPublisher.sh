#!/bin/bash

echo "Testing broker, publisher, and subscriber..."

../bin/StartBroker --id b1 --url udp:127.0.0.1:2350 & > broker.txt
broker_pid=$!

sleep 1

./TestClient -id S1C1 -url udp:127.0.0.1:3351 -broker udp:127.0.0.1:2350 < mana.wkld > sub1 &
sub1_pid=$!

./TestClient -id S1C2 -url udp:127.0.0.1:3352 -broker udp:127.0.0.1:2350 < mana.wkld > sub2 &
sub2_pid=$!

./TestClient -id S1C1 -url udp:127.0.0.1:3451 -broker udp:127.0.0.1:2350 < mana.wkld > pub1 &
pub1_pid=$!

sleep 5

kill $broker_pid
kill $sub1_pid
kill $sub2_pid
kill $pub1_pid

read -d '' res <<"EOF"
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
Payload hash verified.
EOF

echo "$res" | diff out.txt - > fail.txt

if [ $? -eq 0 ]
then
    echo "Test passed successfully."
else
    echo "Test failed. See fail.txt"
    return -1
fi

rm -rf out.txt
