/**
 * Copyright 2022 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

/**
 * @brief Implemnets the DateTime, Date and TimeZone classes.
 */

#pragma once
#include <cstdint>
#include <map>
#include <atomic>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>
#include <random>
#include <algorithm>

#include "value.h"

namespace logger
{
    struct log
    {
        size_t now = 0;

        std::string log_info = NULL;

        log()
        {
            now = 0;
            log_info = "";
        }
    };

    void profl_init(std::string &log_dir);
    void log_breakdown(char *log_info, bool flush = false);
    std::string &get_call_desc();

    bool set_call_desc(const std::string &call_desc_new);

    void set_call_id(size_t call_id);

    bool add_transaction_id();
    size_t get_transaction_id();

    bool add_step_id();
    size_t get_step_id();

    class Profl
    {
    private:
        std::atomic<bool> exit_flag_;
        std::string log_dir_;
        // the log_file records txn and kv operations
        uint64_t next_log_file_id_ = 0;
        // this file records only meta-operations like opening and dropping tables
        std::unique_ptr<std::ofstream> log_file_;

        std::mutex mutex_;
        std::condition_variable cond_;
        std::deque<std::unique_ptr<log[]>> waiting_logs_;
        size_t log_rotate_size_max_ = 10240;
        size_t log_rotate_size_curr_ = 0;
        size_t batch_size_ = 1024;
        size_t batch_time_ms_ = 500;
        size_t log_count = 0;

        // The flusher thread flushes wal on constant intervals and notifies waiting
        // txns. It also prepares new log files for rotation.

        std::thread profl_flusher_;

    public:
        Profl();

        Profl(const std::string &log_dir, Value header);

        ~Profl();

        int AppendGolobalLog(std::unique_ptr<log[]> logs_local);
        __always_inline static size_t GetSystemTime();

    private:
        // Called on transaction commit, when we flush the wal. This guarantees that each
        // log file will contain a complete transaction.
        // This function checks whether the last flush time is too long ago. If that is the
        // case, close current log file and create a new one. It also schedules an asynchronous
        // task to flush the kv store and delete the old log file.
        void FlusherThread();

        // open next log file, changing curr_log_path_ and next_log_file_id
        void OpenNextLogForWrite();

        // Get log path from id
        std::string GetLogFilePathFromId(uint64_t log_file_id) const;

        // Get dbi file path
        std::string GetDbiFilePath() const;

        inline void FlushLogs(std::deque<std::unique_ptr<log[]>> logs);
    };
    __always_inline size_t Profl::GetSystemTime()
    {
        size_t hi, lo;
        __asm__ __volatile__("rdtscp"
                             : "=a"(lo), "=d"(hi));
        return ((size_t)lo) | (((size_t)hi) << 32);
    }

    void shuffle_mine(std::vector<int> &input);
} // namespace logger
