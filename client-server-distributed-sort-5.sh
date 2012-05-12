#!/bin/bash
MANAGER_NODE=1
SRC_NODES_LIST="2 3 4"
DST_NODES_LIST="7 8 9"


if [ $# -ne 1 ]
then
  echo "Error, Required 1 arg: Main path for sub dirs "
  exit
fi

rm -f log/*.log
mkdir /tmp/zmq

echo run node servers

DIR=$1/$MANAGER_NODE
mkdir -p $DIR
echo mkdir $DIR
gnome-terminal --geometry=80x20 -t "server manager $DIR" -x sh -c "./zf-server -s -odirect_io -d $DIR manager $MANAGER_NODE; bash"

for number in $SRC_NODES_LIST
do
	DIR=$1/$number
	mkdir -p $DIR
	echo mkdir $DIR
	gnome-terminal --geometry=80x20 -t "server source $DIR" -x sh -c "./zf-server -s -odirect_io -d $DIR source $number; bash"
done

for number in $DST_NODES_LIST
do
	DIR=$1/$number
	mkdir -p $DIR
	echo mkdir $DIR
	gnome-terminal --geometry=80x20 -t "server dest $DIR" -x sh -c "./zf-server -s -odirect_io -d $DIR dest $number; bash"
done

sleep 2
echo run node client

DIR=$1/$MANAGER_NODE
gnome-terminal --geometry=80x20 -t "manager node $MANAGER $DIR" -x sh -c "./man_node $DIR $MANAGER_NODE; bash"

for number in $SRC_NODES_LIST
do
	DIR=$1/$number
	gnome-terminal --geometry=80x20 -t "source node $DIR" -x sh -c "./src_node $DIR $number; bash"
done

for number in $DST_NODES_LIST
do
	DIR=$1/$number
	gnome-terminal --geometry=80x20 -t "dest node $number $DIR" -x sh -c "./dst_node $DIR $number; bash"
done
