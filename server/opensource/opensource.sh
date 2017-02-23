#!/bin/bash

libdir=`pwd`/libs

Build()
{
    if [ -f libs/libevent.a ]
    then
        echo "libevent ok"
    else
        cd libevent-2.0.21-stable/ && chmod a+x configure && ./configure --prefix=${libdir}&& make -j 4 && cp .libs/libevent.a ../libs/ && make install
        cd ../
    fi

    if [ -f libs/libprotobuf.a ]
    then
        echo "lib protobuf ok"
    else
        cd protobuf-2.5.0 && chmod a+x configure && ./configure --prefix=${libdir} && make -j 4 && cp src/.libs/libprotobuf.a ../libs/ && cp src/.libs/protoc ../../../com/proto/  && make install
        cd ../
    fi

    if [ -f libs/liblog4cplus.a ]
    then
        echo "lib log4cplus ok"
    else
        cd log4cplus-1.2.0-rc3 && chmod a+x configure && ./configure --prefix=${libdir} --enable-static=yes && make clean && make -j 4 && cp .libs/liblog4cplus.a ../libs/&& make install
        cd ../
    fi

    if [ -f libs/libhiredis.a ]
    then
        echo "lib hiredis ok"
    else
        cd redis-2.8.23/deps/hiredis/ && make clean && make static && cp libhiredis.a ../../../libs/
        cd ../../../
    fi

    if [ -f libs/libjson.a ]
    then
        echo "lib libjson ok"
    else
        cd json && make clean && make && cp libjson.a ../libs/
        cd ..
    fi

    if [ -f libs/libmysql_wrap.a ]
    then
        echo "lib libmysql_wrap ok"
    else
        cd mysql_wrap && make clean && make && cp libmysql_wrap.a ../libs/
        cd ..
    fi
}

Kill()
{
    echo "do nothing"
}

Restart()
{
    echo "do nothing"
}

Deploy()
{
    echo "do nothing"
}

Build
