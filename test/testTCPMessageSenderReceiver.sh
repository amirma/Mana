#!/bin/bash

echo "Testing TCPMessageReceiver and TCPMessageSender..."

./TestTCPMessageReceiver > out.txt &
receiver_pid=$!

sleep 0.2

./TestTCPMessageSender > /dev/null
sender_pid=$!

sleep 0.2
kill $receiver_pid

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
