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

size_t test_pread(fio_test::TestType test_type, std::string &path, size_t thread_num, size_t slot_size, size_t io_size, size_t io_num, std::vector<int> order)
{
    {
        std::deque<fio_test::RTest> thread_pool;

        for (int i = 0; i < thread_num; i++)
        {
            thread_pool.emplace_back(test_type, path, slot_size, io_size, io_num, order);
        }
    }
    size_t latency = 0;
    for (int i = 0; i < thread_num; i++)
    {
        latency += fio_test::RTest::latencys_[i].load();
    }
    return latency / thread_num;
}

size_t test_mread(fio_test::TestType test_type, std::string &path, size_t thread_num, size_t slot_size, size_t io_size, size_t io_num, std::vector<int> order)
{
    {
        std::deque<fio_test::MRTest> thread_pool;
        for (int i = 0; i < thread_num; i++)
        {
            thread_pool.emplace_back(test_type, path, slot_size, io_size, io_num, order);
        }
    }

    size_t latency = 0;

    for (int i = 0; i < thread_num; i++)
    {
        latency += fio_test::MRTest::latencys_[i].load();
    }
    return latency / thread_num;
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

std::vector<int> get_order(std::string &file_name)
{
    std::ifstream inFile;
    inFile.open(file_name, std::ios::in);
    std::string str;
    std::vector<int> order_input;
    while (getline(inFile, str, ','))
    {
        if (str != " ")
        {
            order_input.push_back(std::stoi(str));
        }
    }
    inFile.close();
    return std::move(order_input);
}

std::vector<int> create_order(size_t io_num, const std::string &file_name)
{
    std::vector<int> order_output(io_num);

    std::iota(order_output.begin(), order_output.end(), 1);
    logger::shuffle_mine(order_output);
    std::ofstream outFile;
    outFile.open(file_name, std::ios::out | std::ios::trunc);
    for (int n : order_output)
    {
        outFile << n << ",";
    }
    outFile.close();
    return std::move(order_output);
}

int main(int argc, char *argv[])
{
    // FILE *fp = freopen("/dev/null", "w", stdout);
    char tmp[256];
    getcwd(tmp, 256);

    boost::filesystem::path cur_dir(tmp);
    boost::filesystem::path root_dir = cur_dir.parent_path();
    std::string order_file_path = (root_dir / "configuration" / "data.order").string();

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

    /** unit == byte*/
    size_t io_size = 512 * 8;
    size_t slot_size = 128 * 1024;
    size_t io_num = 1024 * 4;

    /** Get random order */
    std::cout << "Read order file from path " << order_file_path << std::endl;
    std::vector<int> order_input = get_order(order_file_path);
    if (order_input.size() != io_num)
    {
        std::cout << "Create and store new order into " << order_file_path << std::endl;
        order_input = create_order(io_num, order_file_path);
    }

    size_t latency = 0;
    std::cout << argv[1] << std::endl;
    fio_test::TestType test_type = fio_test::TestType::RANDOM;
    if (strcmp(argv[1], "pread") == 0)
    {
        latency = test_pread(test_type, path, thread_num, slot_size, io_size, io_num, std::move(order_input));
        std::cout << "Average latency of pread = " << latency << std::endl;
    }
    else if (strcmp(argv[1], "mread") == 0)
    {

        latency = test_mread(test_type, path, thread_num, slot_size, io_size, io_num, std::move(order_input));
        std::cout << "Average latency of mread = " << latency << std::endl;
    }
    else if (strcmp(argv[1], "mm") == 0)
    {

        test_memcpy(atoi(argv[2]));
    }
    else
    {
        std::cout << "Illegal argument value" << std::endl;
    }

    return 0;
}