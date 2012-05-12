#!/bin/bash



if [ $# -ne 2 ]
then
  echo "Error, Required 2 args: 2 any mount paths, example /tmp/1/4 ..."
  exit
fi

#Directory for server to getZeroMQ work properly 
mkdir /tmp/zmq
mkdir -p $1
mkdir -p $2

#exec server for client source
gnome-terminal --geometry=80x20 -t "1server test $1 test2REQ" -x sh -c "./zf-server -s -odirect_io -d $1 test2REQ 1; bash"

#exec server for client manager
gnome-terminal --geometry=80x20 -t "2server test $2 test2REP" -x sh -c "./zf-server -s -odirect_io -d $2 test2REP 2; bash"

sleep 1

gnome-terminal --geometry=80x20 -t "test node $1 test2REQ" -x sh -c "./test_node $1 test2REQ 1; fusermount -u $1; bash"
gnome-terminal --geometry=80x20 -t "test node $2 test2REP" -x sh -c "./test_node $2 test2REP 2; fusermount -u $2; bash"
