#include <thread_pool.hpp>

#include <logger.hpp>

thread_pool::thread_pool(size_t threads_num, std::shared_ptr<logger> logger) : _logger(std::move(logger)) {
    _logger->debug("Thread pool initializing with " + std::to_string(threads_num) + " threads");

    for (size_t i = 0; i < threads_num; ++i) {
        _workers.emplace_back([this](std::stop_token st) {
            _logger->debug("Worker thread " + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
                           " started");
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock lock(_queue_mutex);
                    _cv.wait(lock, [&] { return st.stop_requested() || !_tasks.empty(); });

                    if (st.stop_requested() && _tasks.empty()) {
                        _logger->debug("Worker thread " +
                                       std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) +
                                       " stopping (no more tasks and stop requested)");
                        return;
                    }

                    task = std::move(_tasks.front());
                    _tasks.pop();
                }

                _logger->debug("Worker thread executing a task");
                task();
            }
        });
    }

    _logger->info("Thread pool initialized with " + std::to_string(threads_num) + " threads");
}

thread_pool::~thread_pool() {
    _logger->debug("Thread pool destruction started, requesting stop for all workers");
    for (auto &w: _workers) {
        w.request_stop();
    }

    _cv.notify_all();
    _logger->debug("Notified all workers to wake up and stop");

    _logger->info("Thread pool destroyed");
}