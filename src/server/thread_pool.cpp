#include <thread_pool.hpp>

thread_pool::~thread_pool() {
    for (auto &w: _workers) {
        w.request_stop();
    }
    _cv.notify_all();
}

thread_pool::thread_pool(size_t threads_num) {
    for (size_t i = 0; i < threads_num; ++i) {
        _workers.emplace_back([this](std::stop_token st) {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock lock(_queue_mutex);

                    _cv.wait(lock, [&] { return st.stop_requested() || !_tasks.empty(); });

                    if (st.stop_requested() && _tasks.empty()) {
                        return;
                    }

                    task = std::move(_tasks.front());
                    _tasks.pop();
                }

                task();
            }
        });
    }
}
