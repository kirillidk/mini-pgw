#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <session.hpp>

namespace events {
    struct create_session_event {};
    struct delete_session_event {};
} // namespace events

class event_bus {
public:
    event_bus() = default;

public:
    template<typename EventType, typename F>
    void subscribe(F &&func) {
        auto type_index = std::type_index(typeid(EventType));

        auto wrapper = [f = std::forward<F>(func)](const std::shared_ptr<session> &session) { f(session); };

        _handlers[type_index].emplace_back(std::move(wrapper));
    }

    template<typename EventType>
    void publish(const std::shared_ptr<session> &session) {
        auto type_index = std::type_index(typeid(EventType));
        if (not _handlers.contains(type_index)) {
            return;
        }

        for (const auto &handler: _handlers[type_index]) {
            handler(session);
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::shared_ptr<session> &session)>>>
            _handlers;
};
