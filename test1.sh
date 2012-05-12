#!/bin/bash



if [ $# -ne 6 ]
then
  echo "Error, Required 6 args: 2mount paths, testname1 id testname2 id"
  exit
fi

mkdir -p $1
mkdir -p $2

#exec server for client source
gnome-terminal --geometry=80x20 -t "1server test $1 $3" -x sh -c "./fuse_srv -s -odirect_io -d $1 $3 $4; bash"

#exec server for client manager
gnome-terminal --geometry=80x20 -t "2server test $2 $4" -x sh -c "./fuse_srv -s -odirect_io -d $2 $5 $6; bash"

sleep 2

gnome-terminal --geometry=80x20 -t "test node $1 $3" -x sh -c "./test_node $1 $3 $4; fusermount -u $1; bash"
gnome-terminal --geometry=80x20 -t "test node $2 $4" -x sh -c "./test_node $2 $5 $6; fusermount -u $2; bash"
