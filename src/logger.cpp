#include <utility>
#include <fstream>

#include "logger.h"
#include "value.h"

namespace logger
{
    static std::unique_ptr<Profl> profl_logger;
    static const size_t buf_capacity = 128;
    void log_breakdown(char *log_info, bool flush)
    {
        static thread_local std::unique_ptr<log[]> logs_now((log *)calloc(buf_capacity,
                                                                          sizeof(log)));
        static thread_local size_t buf_size = 0;

        volatile size_t now = profl_logger->GetSystemTime();

        std::string log_tmp;
        log_tmp = log_info;
        logs_now[buf_size].now = now;
        logs_now[buf_size].log_info = log_tmp;
        ++buf_size;

        if (buf_size == buf_capacity || flush)
        {
            profl_logger->AppendGolobalLog(std::move(logs_now));

            // free(logs_now.release());
            if (!flush)
            {
                logs_now.reset((log *)calloc(buf_capacity, sizeof(log)));
                buf_size = 0;
            }
            buf_size = 0;
        }
    }

    void profl_init(std::string &log_dir)
    {
        char *buf = (char *)malloc(300);
        size_t buf_size = 0;
        auto t = std::chrono::system_clock::now();
        time_t tnow = std::chrono::system_clock::to_time_t(t);
        tm *date = std::localtime(&tnow);
        buf_size += std::strftime(buf, 24, "%Y%m%d%H%M%S: ", date);
        buf_size += sprintf(buf + buf_size, "%s\n",
                            "Log Format = {day-hour-minute-second-microsecond}: {[plugin name]} "
                            "{callID-TransactionID-StepID: start(1)/end(0)}");

        Value header;
        header.TakeOwnershipFrom(buf, buf_size);
        profl_logger.reset(new Profl(log_dir, std::move(header)));
    }

    Profl::~Profl()
    {
        exit_flag_ = true;
        profl_flusher_.join();
        FlushLogs(std::move(waiting_logs_));

        log_file_->flush();
        log_file_->close();
    }

    Profl::Profl(const std::string &log_dir, Value header)
        : log_dir_(log_dir), exit_flag_(false)
    {
        // now open log file for write
        OpenNextLogForWrite();

        log_file_->write(header.Data(), header.Size());

        // ok, start wal flusher
        profl_flusher_ = std::thread([this]()
                                     { FlusherThread(); });
    }

    void Profl::OpenNextLogForWrite()
    {
        if (next_log_file_id_ != 0)
        {
            log_file_->flush();
            log_file_->close();
        }
        log_rotate_size_curr_ = 0;
        log_file_.reset(new std::ofstream(GetLogFilePathFromId(next_log_file_id_++), std::ios::out));
    }

    std::string Profl::GetLogFilePathFromId(uint64_t log_file_id) const
    {
        char buf[100];
        sprintf(buf, "%s/%s%d\0", log_dir_.c_str(), "breakdown.log.", log_file_id);
        std::string path = buf;
        return path;
    }

    void Profl::FlusherThread()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (exit_flag_)
                break;

            bool need_flush = true;
            if (waiting_logs_.size() < batch_size_)
            {
                // wait for new txns
                cond_.wait_for(lock, std::chrono::milliseconds(batch_time_ms_));
                need_flush = (waiting_logs_.size() >= batch_size_);
            }
            if (need_flush)
            {
                std::deque<std::unique_ptr<log[]>> logs = std::move(waiting_logs_);
                waiting_logs_ = std::deque<std::unique_ptr<log[]>>();

                log_rotate_size_curr_ += logs.size();
                lock.unlock();

                FlushLogs(std::move(logs));
                if (log_rotate_size_curr_ >= log_rotate_size_max_)
                {
                    OpenNextLogForWrite();
                }
            }
        }
    }
    inline void Profl::FlushLogs(std::deque<std::unique_ptr<log[]>> logs)
    {
        for (auto &log : logs)
        {
            auto log_ptr = log.release();
            char *buf = (char *)calloc(4096, 1);
            size_t buf_size = 0;
            for (int i = 0; i < buf_capacity; i++)
            {
                if (log_ptr[i].now == 0)
                {
                    break;
                }
                if (buf_size + 120 < 4096)
                {
                    log_file_->write(buf, buf_size);
                    // free(buf);
                    // buf = (char*)calloc(4096, 1);
                    buf_size = 0;
                }
                buf_size += sprintf(buf + buf_size, "%zu: %s\n", log_ptr[i].now,
                                    log_ptr[i].log_info.c_str());
            }
            log_file_->write(buf, buf_size);
            free(buf);
            free(log_ptr);
        }
    }

    int Profl::AppendGolobalLog(std::unique_ptr<log[]> logs_local)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        waiting_logs_.emplace_back(std::move(logs_local));
        cond_.notify_one();
        return 0;
    }

} // namespace logger
