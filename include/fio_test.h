#include <cstdint>
#include <map>
#include <atomic>
#include <string>
#include <thread>
#include <deque>
#include <fstream>
#include <vector>
#include <mutex>

namespace fio_test
{
#define MMAP_SIZE (size_t)2 * 1024 * 1024 * 1024
#define MMAP_ADVICE MADV_NORMAL
#define FILE_OPEN_FLAGS O_RDWR | O_DIRECT

    static std::ofstream log_file_;
    static std::mutex log_file_lock_;

    int logger_start(const std::string &log_file_name);

    int logger_logging(size_t io_num, size_t start_times[], size_t end_times[]);
    int logger_stop();

    enum IOMode
    {
        SCAN = 0,
        RANDOM = 1
    };

    enum IorO
    {
        RO = 0, // read only
        RW = 1, // read and write
        WO = 2  // write only
    };
    enum IOEngine
    {
        PRW = 0, // using pread/pwrite
        MMAP = 1
    };

    class WTest
    {
    private:
        static std::atomic<size_t> thread_count_;
        int log_file_;
        size_t file_size_ = 0;
        std::thread operator_;
        bool exit_flag_ = false;

        int WriteRandomData();
        void OperationThread();
        inline std::string GetFilePath(std::string &file_name);

    public:
        ~WTest();
        /**
         * @brief Construct a new WTest object
         *
         * @param file_name
         * @param thread_num
         * @param file_size unit size is word (eight bytes)
         */
        WTest(std::string &file_name, size_t file_size);
    };

    class IOTest
    {
        static std::atomic<size_t> thread_count_;
        static std::ofstream log_file_;
        static std::mutex log_file_lock_;

        size_t thread_id_ = 0;
        int data_file_;
        IOMode io_mode_;
        IOEngine io_engine_;
        size_t slot_size_ = 0;
        size_t io_size_ = 0;
        size_t io_num_ = 0;
        std::thread operator_;
        bool exit_flag_ = false;
        std::vector<int> order_;
        size_t *start_time_;
        size_t *stop_time_;

        int ReadFile();
        int PRead();
        int MRead();
        std::string GetFilePath(std::string &file_name);

    public:
        ~IOTest();
        /**
         * @brief Construct a new RTest object
         *
         * @param file_name
         * @param thread_num
         * @param file_size unit size is word (eight bytes)
         */
        IOTest(IOMode io_mode, IOEngine io_engine, std::string &file_name, size_t slot_size, size_t io_size, size_t io_num, std::vector<int> order);
    };
}