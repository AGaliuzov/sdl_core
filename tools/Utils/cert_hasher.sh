#!/bin/sh

cd $1
for crt in *.crt; do
    awk '/BEGIN CERTIFICATE/{++n;}{print $0 >> n".tmp";}' $crt
    for i in *.tmp; do 
        openssl x509 -in $i -hash -noout| awk '{print $0".0"}' | xargs ln -sf $crt
    done
    rm *.tmp
done
sync
sync
