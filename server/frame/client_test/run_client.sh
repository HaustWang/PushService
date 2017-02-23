#!/bin/bash

LOOP=0
BEGIN=5000
END=5050

cat cid.txt | while read LINE
do 
    if [ $LOOP -ge $BEGIN ] ; then
      if [ $LOOP -lt $END ]; then 
         cmd="./imclient -s 192.168.203.216 -p 11114 -c $LINE -m 28 -r 2"
         echo $cmd
          nohup $cmd 2>&1 >>/dev/null &
         LOOP=`expr $LOOP + 1`
     fi
    fi
    
    LOOP=`expr $LOOP + 1`
done
