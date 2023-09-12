cd bin

IO_ENGINE="mmap" # mmap or pio 
RorW="read" # read or write or mix
THREAD_NUM=1
let IO_SIZE=1024*4
ITER_NUM=1

echo 1 > /proc/sys/vm/drop_caches
echo 1 > /proc/sys/vm/drop_caches
# cgexec -g memory:yz_4g ./bin/IO_test
# cgexec -g memory:yz_4g ./IO_test ${IO_ENGINE} ${RorW} ${THREAD_NUM} ${IO_SIZE} ${ITER_NUM}
# nohup bpftrace /data/experiment_space/bpftrace/test.bt  &
./IO_test ${IO_ENGINE} ${RorW} ${THREAD_NUM} ${IO_SIZE} ${ITER_NUM}
# pkill --signal 2 -e bpftrace 