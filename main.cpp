#include "boost/filesystem.hpp"
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <string.h>

#include "fio_mine.h"
void create_test_files(std::string &path, size_t thread_num)
{
    std::deque<fio_test::WTest> thread_pool;
    for (int i = 0; i < thread_num; i++)
    {
        thread_pool.emplace_back(path, MMAP_SIZE / 1024);
    }
}

void test_pread(std::string &path, size_t thread_num, size_t slot_size, size_t io_size, size_t io_num)
{
    std::deque<fio_test::RTest> thread_pool;
    for (int i = 0; i < thread_num; i++)
    {
        thread_pool.emplace_back(path, slot_size, io_size, io_num);
    }
}

void test_mread(std::string &path, size_t thread_num, size_t slot_size, size_t io_size, size_t io_num)
{
    std::deque<fio_test::MRTest> thread_pool;
    for (int i = 0; i < thread_num; i++)
    {
        thread_pool.emplace_back(path, slot_size, io_size, io_num);
    }
}

void test_memcpy(int block_num)
{
    size_t start_time = logger::Profl::GetSystemTime();
    size_t duration = 0;

    size_t buf_size = 512 * block_num;

    size_t exp_num = 1000;
    char *buf_1 = (char *)aligned_alloc(512 * 8, buf_size);
    char *buf_3 = (char *)calloc(buf_size, 1);
    buf_1[1] = 'a';
    char *buf_2 = (char *)aligned_alloc(512 * 8, buf_size);
    memcpy(buf_2, buf_3, buf_size);
    for (int i = 0; i < exp_num; i++)
    {
        start_time = logger::Profl::GetSystemTime();
        memcpy(buf_2, buf_1, buf_size);
        duration += logger::Profl::GetSystemTime() - start_time;
        std::cout << buf_2[1] << std::endl;
        buf_2 = (char *)aligned_alloc(512 * 8, buf_size);
        memcpy(buf_2, buf_3, buf_size);
    }
    std::cout << buf_size << std::endl;
    std::cout << "duration  = " << duration / block_num / exp_num << std::endl;
}

int main(int argc, char *argv[])
{
    // FILE *fp = freopen("/dev/null", "w", stdout);
    char tmp[256];
    getcwd(tmp, 256);

    boost::filesystem::path cur_dir(tmp);
    boost::filesystem::path root_dir = cur_dir.parent_path();

    std::string file_dir = "/data/lgraph_db";
    std::string file_name = "test";
    boost::filesystem::path test_dir(file_dir + "/test_dir");

    boost::filesystem::create_directory(test_dir);
    std::string path = (test_dir / file_name).string();
    std::cout
        << "Current working directory: " << path << std::endl;

    size_t thread_num = 100;
    size_t durations[thread_num];
    // create_test_files(path, thread_num);
    // return 0;

    std::string log_dir = test_dir.string();
    logger::profl_init(log_dir);

    size_t io_size = 512 * 1;
    size_t slot_size = 128 * 1024;
    size_t io_num = 1024 * 4;

    std::string test_type = argv[1];

    if (strcmp(argv[1], "pread") == 0)
    {
        test_pread(path, thread_num, slot_size, io_size, io_num);
    }
    else if (strcmp(argv[1], "mread") == 0)
    {
        test_mread(path, thread_num, slot_size, io_size, io_num);
    }
    else if (strcmp(argv[1], "mm") == 0)
    {

        test_memcpy(atoi(argv[2]));
    }
    else
    {
        std::cout << "Illegal argument value" << std::endl;
    }

    size_t duration = 0;
    for (int i = 0; i < thread_num; i++)
    {
        if (strcmp(argv[1], "pread") == 0)
        {

            duration += fio_test::RTest::durations[i].load();
            // std::cout << "pread" << fio_test::RTest::durations[i].load() << std::endl;
        }
        else
        {
            duration += fio_test::MRTest::durations[i].load();
            // std::cout << "mread" << fio_test::MRTest::durations[i].load() << std::endl;
        }
    }
    duration /= thread_num;
    std::cout << "Average duration = " << duration << std::endl;
    return 0;
}