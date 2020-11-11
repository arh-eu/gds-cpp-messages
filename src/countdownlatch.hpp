#include <chrono>
#include <condition_variable>
#include <mutex>

namespace thread_utils {
    class CountDownLatch {
    public:
        explicit CountDownLatch(const unsigned int count): m_count(count) { }
        virtual ~CountDownLatch() = default;

        void await() {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_count > 0) {
                m_cv.wait(lock, [this](){ return m_count == 0; });
            }
        }

        bool await(const std::chrono::milliseconds& timeout) {
            std::unique_lock<std::mutex> lock(m_mutex);
            bool result = true;
            if (m_count > 0) {
                result = m_cv.wait_for(lock, timeout, [this](){ return m_count == 0; });
            }

            return result;
        }

        bool await(const uint64_t timeout) {
            return await(std::chrono::milliseconds(timeout));
        }

        void countdown() {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_count > 0) {
                m_count--;
                m_cv.notify_all();
            }
        }

        unsigned int get_count() {
            std::unique_lock<std::mutex> lock(m_mutex);
            return m_count;
        }

    protected:
        std::mutex m_mutex;
        std::condition_variable m_cv;
        unsigned int m_count;
    };

}