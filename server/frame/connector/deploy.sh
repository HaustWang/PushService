#!/bin/bash

readonly ARGS="$@"
BIN=connector

Build()
{
    make -j 4
}

Start()
{
    echo "start $BIN "
    ./$BIN -p 11114 -s 1
    ./$BIN -p 11115 -s 2
    ps -ef | grep -v grep | grep -w $BIN
}

Stop()
{
    echo "stop $BIN"
    killall -9 $BIN
}

Restart()
{
    Stop 
    sleep 1
    Start 
}

Usage()
{
    echo "usage ./$0 start or stop or restarti or deploy"
}

Deploy()
{
	echo "usage ./$0 Deploy"
    cp /home/lanshigang/IM/server/frame/connector/src/connector .
}

main()
{
    if [ $# -ne 1 ]
    then
        Usage
        exit 0
    fi

    if [ $1 == "start" ]
    then
        Start
    elif [ $1 == "stop" ]
    then
        Stop
    elif [ $1 == "restart" ]
    then
        Restart
    elif [ $1 == "deploy" ]
    then
	Deploy
    else
        Usage
        exit 0
    fi

}

main $ARGS
