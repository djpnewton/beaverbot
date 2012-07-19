#!/bin/sh

# set gateway
route add default gw 10.10.10.11
# use googles nameservers
#sh -c "echo 'nameserver 8.8.8.8' >> /etc/resolv.conf
