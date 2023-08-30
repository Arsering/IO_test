cd bin
echo 1 > /proc/sys/vm/drop_caches
echo 1 > /proc/sys/vm/drop_caches
echo 1 > /proc/sys/vm/drop_caches
# rm -rf /data/lgraph_db/test_dir/*
# cgexec -g memory:yz_4g ./bin/IO_test
./IO_test mread