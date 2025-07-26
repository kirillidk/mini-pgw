#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

#include <logger.hpp>

class thread_pool {
public:
    explicit thread_pool(size_t threads_num, std::shared_ptr<logger> logger);
    ~thread_pool();

    thread_pool(const thread_pool &) = delete;
    thread_pool &operator=(const thread_pool &) = delete;
    thread_pool(thread_pool &&) = delete;
    thread_pool &operator=(thread_pool &&) = delete;

    template<typename F, typename... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        auto result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(_queue_mutex);
            _tasks.emplace([task] { (*task)(); });
        }

        _cv.notify_one();
        return result;
    }

private:
    std::shared_ptr<logger> _logger;

    std::vector<std::jthread> _workers;
    std::queue<std::function<void()>> _tasks;
    std::mutex _queue_mutex;
    std::condition_variable _cv;
};
