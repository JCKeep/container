#!/bin/bash

pid=$1
netns_name=$2
filename=/var/run/netns/${netns_name}

echo ${tmp}
if [ -e "$filename" ] || [ -L "$filename" ]; then
    rm "$filename"
fi

if [ ! -d "/var/run/netns" ]; then
    mkdir /var/run/netns
fi

ln -s /proc/${pid}/ns/net /var/run/netns/${netns_name}

ip link add veth0 type veth peer name veth1
ip link set veth1 netns ${netns_name}

ip addr add 192.168.0.2/24 dev veth0
ip netns exec ${netns_name} ip addr add 192.168.0.1/24 dev veth1

ip link set dev veth0 up
ip netns exec ${netns_name} ip link set dev veth1 up