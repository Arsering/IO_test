#include <cstdint>
#include <map>
#include <atomic>
#include <string>
#include <thread>
#include <deque>
#include <fstream>
#include <vector>

namespace fio_test
{
#define MMAP_SIZE 1024 * 1024 * 1024
#define MMAP_ADVICE MADV_RANDOM
// #define FILE_OPEN_FLAGS O_RDWR | O_DIRECT
#define FILE_OPEN_FLAGS O_RDWR | O_DIRECT

    enum TestType
    {
        SCAN = 0,
        RANDOM = 1
    };

    class WTest
    {
    private:
        static std::atomic<size_t>
            thread_count_;

        std::unique_ptr<std::ofstream>
            log_file_;
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

    class RTest
    {
        static std::atomic<size_t> thread_count_;
        size_t thread_id_ = 0;
        int log_file_;
        size_t slot_size_ = 0;
        size_t io_size_ = 0;
        size_t io_num_ = 0;
        std::thread operator_;
        bool exit_flag_ = false;
        std::vector<int> order_;

        int ScanFile();
        int RandomReadFile();
        void OperationThread();
        std::string GetFilePath(std::string &file_name);

    public:
        static std::atomic<size_t> latencys_[100];

        ~RTest();
        /**
         * @brief Construct a new RTest object
         *
         * @param file_name
         * @param thread_num
         * @param file_size unit size is word (eight bytes)
         */
        RTest(TestType test_type, std::string &file_name, size_t slot_size, size_t io_size, size_t io_num, std::vector<int> order);
    };

    class MRTest
    {
        static std::atomic<size_t> thread_count_;
        size_t thread_id_ = 0;
        int log_file_;
        void *log_file_mmaped_;
        size_t slot_size_ = 0;
        size_t io_size_ = 0;
        size_t io_num_ = 0;
        std::thread operator_;
        bool exit_flag_ = false;
        std::vector<int> order_;

        int ScanFile();
        int RandomReadFile();
        void OperationThread();
        std::string GetFilePath(std::string &file_name);

    public:
        static std::atomic<size_t> latencys_[100];

        ~MRTest();
        /**
         * @brief Construct a new RTest object
         *
         * @param file_name
         * @param thread_num
         * @param file_size unit size is word (eight bytes)
         */
        MRTest(TestType test_type, std::string &file_name, size_t slot_size, size_t io_size, size_t io_num, std::vector<int> order);
    };

}