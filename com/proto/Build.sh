#!/bin/bash
md5_push_proto_server=17834f7b1613e9bfc8f2e398abcbd509
md5_push_proto_common=36e92ad26d0d12c04449bbd4395eb022
md5_push_proto_client=d68e9bd4f0ea6b47a576f7a07cd1a51e

CPP_OUT="../../server/frame/proto"

is_change=0

for f in $(ls *.proto)
do
    i=${f/.proto/}
    md5string="$""md5_$i"

    a=`eval "echo $md5string"`
    if [ "$a" == "" ]
    then
        sed "2 i${md5string:1}=" -i $0
    fi

    md5_str=`md5sum $f | awk '{print $1}'`
    if [ "$a" != "$md5_str" ]
    then
        echo "protoc convert $f"
        protoc --cpp_out=$CPP_OUT $f;

        is_change=1
        sed -i "s/${md5string:1}=$a/${md5string:1}=$md5_str/" $0
    fi
done

if [ $is_change == 1 ]
then
    cd $CPP_OUT
    rename .pb.cc .cpp *
    rename .pb.h .h *
    sed -i 's/\.pb.h\>/.h/g' *.cpp
    sed -i 's/\.pb.h\>/.h/g' *.h
fi
#ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .

#cd $CPP_OUT
#ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
#
#make -j 4
#
#cd -
