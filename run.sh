cd bin

IO_ENGINE="mmap" # mmap or pio 
RorW="write" # read or write or mix
THREAD_NUM=1
let IO_SIZE=1024*4
ITER_NUM=1

# cgexec -g memory:yz_4g ./bin/IO_test
./IO_test ${IO_ENGINE} ${RorW} ${THREAD_NUM} ${IO_SIZE} ${ITER_NUM}