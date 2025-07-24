#pragma once

#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace events {
    struct create_session_event {};
    struct delete_session_event {};
    struct reject_session_event {};
} // namespace events

class event_bus {
public:
    event_bus() = default;

public:
    template<typename EventType, typename F>
    void subscribe(F &&func) {
        auto type_index = std::type_index(typeid(EventType));

        auto wrapper = [f = std::forward<F>(func)](std::string imsi) { f(std::move(imsi)); };

        _handlers[type_index].emplace_back(std::move(wrapper));
    }

    template<typename EventType>
    void publish(const std::string &imsi) {
        auto type_index = std::type_index(typeid(EventType));
        if (not _handlers.contains(type_index)) {
            return;
        }

        for (const auto &handler: _handlers[type_index]) {
            handler(imsi);
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(std::string)>>> _handlers;
};
