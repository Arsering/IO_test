#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <vector>
#include <mutex>

#include "fio_mine.h"

#define _USE_GNU 1
#define _GNU_SOURCE

namespace fio_test
{
    std::atomic<size_t> WTest::thread_count_(0);

    int logger_start(const std::string& log_file_name)
    {
        log_file_.open(log_file_name, std::ios::out);
        return 0;
    }

    int logger_logging(size_t io_num, size_t start_times[], size_t stop_times[])
    {
        std::unique_lock<std::mutex> lock(log_file_lock_);
        log_file_ << io_num << std::endl;
        for (int i = 0; i < io_num; i++)
        {
            log_file_ << start_times[i] << "," << stop_times[i] << std::endl;
        }
        return 0;
    }
    int logger_stop()
    {
        log_file_.flush();
        log_file_.close();
        return 0;
    }

    WTest::WTest(std::string& file_name, size_t file_size) : file_size_(file_size),
        exit_flag_(false)
    {
        log_file_ = open(GetFilePath(file_name).c_str(), O_RDWR | O_CREAT);
        operator_ = std::thread([this]()
            { OperationThread(); });
    }
    WTest::~WTest()
    {
        operator_.join();
        fsync(log_file_);
        close(log_file_);
    }

    int WTest::WriteRandomData()
    {
        char* str = "abcdefgh";
        char* out_buf = (char*)malloc(1024);

        size_t buf_size = 0;
        while (buf_size < 1024)
        {
            buf_size += sprintf(out_buf + buf_size, "%s", str);
        }
        ftruncate(log_file_, 1024 * file_size_);
        void* log_file_mmaped = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, log_file_, 0);
        madvise(log_file_mmaped, MMAP_SIZE, MADV_SEQUENTIAL); // Turn off readahead

        for (int i = 0; i < file_size_; i++)
        {
            memcpy((char*)log_file_mmaped + 1024 * i, out_buf, 1024);
        }
        return 0;
    }
    void WTest::OperationThread()
    {
        WriteRandomData();
    }
    inline std::string WTest::GetFilePath(std::string& file_name)
    {
        std::string file_path = file_name + "test-" + std::to_string(thread_count_) + ".fio";
        std::cout << file_path << std::endl;
        return file_path;
    }

    std::atomic<size_t> IOTest::thread_count_(0);

    IOTest::IOTest(IOMode io_mode, IOEngine io_engine, std::string& test_path, size_t slot_size, size_t io_size, size_t io_num, size_t iter_num, std::vector<int> order) : io_mode_(io_mode), io_engine_(io_engine), slot_size_(slot_size), io_size_(io_size), io_num_(io_num), iter_num_(iter_num), order_(std::move(order)), exit_flag_(false)
    {
        thread_id_ = thread_count_.fetch_add(1);

        data_file_ = open(GetFilePath(test_path).c_str(), FILE_OPEN_FLAGS);
        if (data_file_ == -1)
        {
            std::cout << "Open file failed!!!" << std::endl;
        }
        start_time_ = (size_t*)calloc(io_num_ * iter_num_, sizeof(size_t));
        stop_time_ = (size_t*)calloc(io_num_ * iter_num_, sizeof(size_t));
        operator_ = std::thread([this]()
            { ReadFile(); });
    }

    IOTest::~IOTest()
    {
        operator_.join();
        logger_logging(io_num_, start_time_, stop_time_);
        free(start_time_);
        free(stop_time_);
        close(data_file_);
    }

    int IOTest::ReadFile()
    {
        if (io_mode_ == IOMode::SCAN)
        {
            std::vector<int> order_output(io_num_);
            std::iota(order_output.begin(), order_output.end(), 0);
            order_ = std::move(order_output);
        }
        switch (io_engine_)
        {
        case IOEngine::MMAP:
            return MRead();
        case IOEngine::PRW:
            return PRead();
        }
    }

    int IOTest::MRead()
    {
        void* log_file_mmaped = mmap(NULL, MMAP_SIZE, PROT_READ, MAP_SHARED | MAP_NORESERVE, data_file_, 0);
        madvise(log_file_mmaped, MMAP_SIZE, MMAP_ADVICE); // Turn off readahead

        char* in_buf = (char*)calloc(1, 1);
        size_t start = 0;
        size_t latency = 0;
        volatile int a = 0;
        for (int j = 0; j < iter_num_; j++)
        {
            for (auto i : order_)
            {
                size_t index = i + j * io_num_;
                start_time_[index] = logger::Profl::GetSystemTime();
                // Create one page fault
                memcpy(in_buf, (char*)log_file_mmaped + slot_size_ * i, 1);
                stop_time_[index] = logger::Profl::GetSystemTime();
                // Prevent this code section from compiler optimization
                if (strcmp(in_buf, "a") == 0)
                {
                    a = 0;
                }
                else
                {
                    a = 1;
                }
            }
        }
        free(in_buf);
        return 0;
    }

    int IOTest::PRead()
    {
        char* in_buf = (char*)aligned_alloc(512 * 8, io_size_);
        char* tmp_buf = (char*)malloc(1);
        size_t start = 0, end = 0;
        size_t latency = 0;
        volatile int a = 0;
        for (int j = 0; j < iter_num_; j++)
        {
            for (auto i : order_)
            {
                size_t index = i + j * io_num_;
                start_time_[index] = logger::Profl::GetSystemTime();
                int ret = pread(data_file_, in_buf, io_size_, slot_size_ * i);
                memcpy(tmp_buf, in_buf, 1);
                stop_time_[index] = logger::Profl::GetSystemTime();

                // Prevent this code section from compiler optimization
                if (strcmp(tmp_buf, "a") == 0)
                {
                    a = 0;
                }
                else
                {
                    a = 1;
                }
            }
        }

        free(in_buf);
        free(tmp_buf);
        return 0;
    }

    inline std::string IOTest::GetFilePath(std::string& file_name)
    {

        std::string file_path = file_name + "/test-" + std::to_string(thread_id_) + ".fio";
        // std::cout << file_path << std::endl;
        return file_path;
    }
}