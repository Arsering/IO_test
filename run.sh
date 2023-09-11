cd bin

IO_ENGINE="pio" # mmap or pio 
RorW="read" # read or write or mix
THREAD_NUM=10
let IO_SIZE=1024*4
ITER_NUM=1

# cgexec -g memory:yz_4g ./bin/IO_test
./IO_test ${IO_ENGINE} ${RorW} ${THREAD_NUM} ${IO_SIZE} ${ITER_NUM}