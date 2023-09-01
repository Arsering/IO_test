#include "boost/filesystem.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <string.h>
#include "include/yaml-cpp/yaml.h"

#include "fio_mine.h"
void create_test_files(std::string& path, size_t thread_num)
{
    std::deque<fio_test::WTest> thread_pool;
    for (int i = 0; i < thread_num; i++)
    {
        thread_pool.emplace_back(path, MMAP_SIZE / 1024);
    }
}

size_t analyze_logs(const std::string& log_file_path)
{
    std::ifstream log_file(log_file_path, std::ios::in);
    std::string tmp, str;
    size_t io_num = 0;
    std::vector<size_t> start_time;
    std::vector<size_t> stop_time;
    size_t duration = 0;
    while (getline(log_file, tmp))
    {
        io_num = std::stoull(tmp);
        while (io_num > 0)
        {
            getline(log_file, tmp, ',');
            start_time.push_back(std::stoull(tmp));
            getline(log_file, tmp);
            stop_time.push_back(std::stoull(tmp));
            io_num--;
        }
    }
    for (int i = 0; i < start_time.size(); i++)
    {
        duration += stop_time[i] - start_time[i];
    }

    duration /= start_time.size();
    return duration;
}

size_t test_read(fio_test::IOMode io_mode, fio_test::IOEngine io_engine, std::string& test_path, size_t thread_num, size_t slot_size, size_t io_size, size_t io_num, size_t iter_num, std::vector<int> order)
{
    std::string log_file_path = test_path + "/logs.log";
    fio_test::logger_start(log_file_path);
    {
        std::deque<fio_test::IOTest> thread_pool;

        for (int i = 0; i < thread_num; i++)
        {
            thread_pool.emplace_back(io_mode, io_engine, test_path, slot_size, io_size, io_num, iter_num, order);
        }
    }
    fio_test::logger_stop();
    return analyze_logs(log_file_path);
}

void test_memcpy(int block_num)
{
    size_t start_time = logger::Profl::GetSystemTime();
    size_t duration = 0;

    size_t buf_size = 512 * block_num;

    size_t exp_num = 1000;
    char* buf_1 = (char*)aligned_alloc(512 * 8, buf_size);
    char* buf_3 = (char*)calloc(buf_size, 1);
    buf_1[1] = 'a';
    char* buf_2 = (char*)aligned_alloc(512 * 8, buf_size);
    memcpy(buf_2, buf_3, buf_size);
    for (int i = 0; i < exp_num; i++)
    {
        start_time = logger::Profl::GetSystemTime();
        memcpy(buf_2, buf_1, buf_size);
        duration += logger::Profl::GetSystemTime() - start_time;
        std::cout << buf_2[1] << std::endl;
        buf_2 = (char*)aligned_alloc(512 * 8, buf_size);
        memcpy(buf_2, buf_3, buf_size);
    }
    std::cout << buf_size << std::endl;
    std::cout << "duration  = " << duration / block_num / exp_num << std::endl;
}

std::vector<int> get_order(std::string& file_name)
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

std::vector<int> create_order(size_t io_num, const std::string& file_name)
{
    std::vector<int> order_output(io_num);

    std::iota(order_output.begin(), order_output.end(), 0);
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

int main(int argc, char* argv[])
{
    // FILE *fp = freopen("/dev/null", "w", stdout);
    char tmp[256];
    getcwd(tmp, 256);

    boost::filesystem::path cur_dir(tmp);
    boost::filesystem::path root_dir = cur_dir.parent_path();
    std::string order_file_path = (root_dir / "configuration" / "data.order").string();

    std::string file_dir = "/data/lgraph_db";
    boost::filesystem::path test_dir(file_dir + "/test_dir");
    YAML::Node config = YAML::LoadFile((root_dir / "configuration" / "config.yaml").string());

    boost::filesystem::create_directory(test_dir);
    std::string test_path = test_dir.string();
    std::cout
        << "Current working directory: " << test_path << std::endl;

    std::string log_dir = test_dir.string();
    logger::profl_init(log_dir);

    size_t latency = 0;
    std::cout << argv[1] << std::endl;
    fio_test::IOMode io_mode = fio_test::IOMode::RANDOM;
    fio_test::IOEngine io_engine;
    if (strcmp(argv[1], "mread") == 0)
    {
        io_engine = fio_test::IOEngine::MMAP;
    }
    else if (strcmp(argv[1], "pread") == 0)
    {
        io_engine = fio_test::IOEngine::PRW;
    }

    /** unit == byte*/
    size_t thread_num = std::stoull(argv[2]);
    size_t io_size = std::stoull(argv[3]);
    size_t slot_size = 1024 * 128;
    size_t io_num = 1024 * 8;
    size_t iter_num = std::stoull(argv[4]); // for SSD saturation

    // create_test_files(path, thread_num);
    // return 0;

    /** Get random order */
    std::cout << "Read order file from path " << order_file_path << std::endl;
    std::vector<int> order_input = get_order(order_file_path);
    if (order_input.size() != io_num)
    {
        std::cout << "Create and store new order into " << order_file_path << std::endl;
        order_input = create_order(io_num, order_file_path);
    }

    latency = test_read(io_mode, io_engine, test_path, thread_num, slot_size, io_size, io_num, iter_num, std::move(order_input));
    std::cout << "Average latency of " << argv[1] << " = " << latency << std::endl;

    // if (strcmp(argv[1], "mm") == 0)
    // {

    //     test_memcpy(atoi(argv[2]));
    // }
    // else
    // {
    //     std::cout << "Illegal argument value" << std::endl;
    // }

    return 0;
}