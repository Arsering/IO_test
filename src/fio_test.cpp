#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "fio_mine.h"

#define _USE_GNU 1
#define _GNU_SOURCE

namespace fio_test
{

    std::atomic<size_t> WTest::thread_count_(0);

    WTest::WTest(std::string &file_name, size_t file_size) : file_size_(file_size),
                                                             exit_flag_(false)
    {
        log_file_.reset(new std::ofstream(GetFilePath(file_name)));
        operator_ = std::thread([this]()
                                { OperationThread(); });
    }
    WTest::~WTest()
    {
        operator_.join();
        log_file_->flush();
        log_file_->close();
    }

    int WTest::WriteRandomData()
    {
        char *str = "abcdefgh";
        char *out_buf = (char *)malloc(1024);

        size_t buf_size = 0;
        while (buf_size < 1024)
        {
            buf_size += sprintf(out_buf + buf_size, "%s", str);
        }
        for (int i = 1; i < file_size_; i++)
        {
            log_file_->write(out_buf, 1024);
        }
        return 0;
    }
    void WTest::OperationThread()
    {
        WriteRandomData();
    }
    inline std::string WTest::GetFilePath(std::string &file_name)
    {
        std::string file_path = file_name + "-" + std::to_string(thread_count_.fetch_add(1)) + ".fio";
        std::cout << file_path << std::endl;
        return file_path;
    }

    std::atomic<size_t> RTest::thread_count_(0);
    std::atomic<size_t> RTest::durations[100];

    RTest::RTest(std::string &file_name, size_t slot_size, size_t io_size, size_t io_num) : slot_size_(slot_size), io_size_(io_size), io_num_(io_num),
                                                                                            exit_flag_(false)
    {
        thread_id_ = thread_count_.fetch_add(1);
        log_file_ = open(GetFilePath(file_name).c_str(), FILE_OPEN_FLAGS);
        // std::cout << log_file_ << std::endl;
        operator_ = std::thread([this]()
                                { OperationThread(); });
    }
    RTest::~RTest()
    {
        operator_.join();
        close(log_file_);
    }

    int RTest::ScanFile()
    {
        char *in_buf = (char *)aligned_alloc(512 * 8, io_size_);
        char *tmp_buf = (char *)malloc(1);
        size_t start = 0;
        size_t duration = 0;
        volatile int a = 0;

        for (int i = 0; i < io_num_; i++)
        {
            start = logger::Profl::GetSystemTime();
            int ret = pread(log_file_, in_buf, io_size_, slot_size_ * i);
            memcpy(tmp_buf, in_buf, 1);
            duration += logger::Profl::GetSystemTime() - start;
            if (strcmp(tmp_buf, "a") == 0)
            {
                a = 0;
            }
            else
            {
                a = 1;
            }
        }
        durations[thread_id_].store(duration / io_num_);
        // std::cout << durations[thread_id_] << std::endl;

        free(in_buf);
        free(tmp_buf);
        return 0;
    }

    void RTest::OperationThread()
    {
        ScanFile();
    }
    inline std::string RTest::GetFilePath(std::string &file_name)
    {
        std::string file_path = file_name + "-" + std::to_string(thread_id_) + ".fio";
        // std::cout << file_path << std::endl;
        return file_path;
    }

    std::atomic<size_t> MRTest::thread_count_(0);
    std::atomic<size_t> MRTest::durations[100];

    MRTest::MRTest(std::string &file_name, size_t slot_size, size_t io_size, size_t io_num) : slot_size_(slot_size), io_size_(io_size), io_num_(io_num),
                                                                                              exit_flag_(false)
    {
        thread_id_ = thread_count_.fetch_add(1);
        log_file_ = open(GetFilePath(file_name).c_str(), FILE_OPEN_FLAGS);

        log_file_mmaped_ = mmap(NULL, MMAP_SIZE, PROT_READ, MAP_SHARED | MAP_NORESERVE, log_file_, 0);

        /* Turn off readahead. */
        madvise(log_file_mmaped_, MMAP_SIZE, MADV_RANDOM);

        operator_ = std::thread([this]()
                                { OperationThread(); });
    }
    MRTest::~MRTest()
    {
        operator_.join();
        munmap(log_file_mmaped_, MMAP_SIZE);
        close(log_file_);
    }

    int MRTest::ScanFile()
    {
        char *in_buf = (char *)calloc(1, 1);
        size_t start = 0;
        size_t duration = 0;
        volatile int a = 0;
        for (int i = 0; i < io_num_; i++)
        {
            start = logger::Profl::GetSystemTime();
            // Create one page fault
            memcpy(in_buf, (char *)log_file_mmaped_ + slot_size_ * i, 1);
            duration += logger::Profl::GetSystemTime() - start;
            // Prevent this code part from compiler optimization
            if (strcmp(in_buf, "a") == 0)
            {
                a = 0;
            }
            else
            {
                a = 1;
            }
        }
        durations[thread_id_].store(duration / io_num_);

        free(in_buf);
        return 0;
    }

    void MRTest::OperationThread()
    {
        ScanFile();
    }

    inline std::string MRTest::GetFilePath(std::string &file_name)
    {
        std::string file_path = file_name + "-" + std::to_string(thread_id_) + ".fio";
        // std::cout << file_path << std::endl;
        return file_path;
    }
}