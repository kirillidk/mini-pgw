#pragma once

#include <any>
#include <functional>
#include <string>
#include <tuple>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <thread_pool.hpp>

namespace events {
    struct create_session_event {
        using param_type = std::tuple<std::string>;
    };

    struct delete_session_event {
        using param_type = std::tuple<std::string>;
    };

    struct reject_session_event {
        using param_type = std::tuple<std::string>;
    };

    struct graceful_shutdown_event {
        using param_type = std::tuple<>;
    };
} // namespace events

class event_bus {
public:
    explicit event_bus(std::shared_ptr<thread_pool> thread_pool) : _thread_pool(std::move(thread_pool)) {}

public:
    template<typename EventType, typename F>
    void subscribe(F &&func) {
        using ParamTuple = typename EventType::param_type;
        auto type_index = std::type_index(typeid(EventType));

        auto wrapper = [f = std::forward<F>(func)](const ParamTuple &params) { std::apply(f, params); };

        _handlers[type_index].emplace_back(std::function<void(const ParamTuple &)>(wrapper));
    }

    template<typename EventType, typename... Args>
    void publish(Args &&...args) {
        using ParamTuple = typename EventType::param_type;
        auto type_index = std::type_index(typeid(EventType));

        ParamTuple params = std::make_tuple(std::forward<Args>(args)...);

        for (const auto &handler_any: _handlers[type_index]) {
            auto handler = std::any_cast<std::function<void(const ParamTuple &)>>(handler_any);

            _thread_pool->enqueue([handler, params]() { handler(params); });
        }
    }

private:
    std::shared_ptr<thread_pool> _thread_pool;
    std::unordered_map<std::type_index, std::vector<std::any>> _handlers;
};
