#!/bin/bash

LOOP=0
BEGIN=0
END=50

cat cid.txt | while read LINE
do 
    if [ $LOOP -ge $BEGIN ] ; then
      if [ $LOOP -lt $END ]; then 
         cmd="./imclient -s 120.132.147.29 -p 9956 -c $LINE -m 28 -r 2"
         echo $cmd
         nohup $cmd 2>&1 >>/dev/null &
         #cmd="./imclient -s 192.168.203.216 -p 11114 -c 2008194 -m 28 -r 2 -a $LINE"
         #echo $cmd
         #nohup $cmd 2>&1 >>/dev/null &
         #sleep 5
         LOOP=`expr $LOOP + 1`
     fi
    fi
    
    LOOP=`expr $LOOP + 1`
done
