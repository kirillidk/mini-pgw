#include <cdr_writer.hpp>

#include <config.hpp>
#include <event_bus.hpp>
#include <utility.hpp>

#include <magic_enum/magic_enum.hpp>

cdr_writer::cdr_writer(std::shared_ptr<config> config, std::shared_ptr<event_bus> event_bus) :
    _config(std::move(config)), _event_bus(std::move(event_bus)) {
    setup();
}

void cdr_writer::setup() {

    auto cdr_file_path = _config->get_cdr_file();
    if (cdr_file_path.has_value()) {
        _file.open(cdr_file_path.value(), std::ios::out | std::ios::app);

        if (!_file.is_open()) {
            throw cdr_writer_exception("Cannot open CDR file: " + cdr_file_path.value().string());
        }
    } else {
        throw cdr_writer_exception("Unable to get CDR file path in config.json");
    }

    _event_bus->subscribe<events::create_session_event>([this](std::string imsi) {
        cdr_record record{
                .timestamp = utility::get_current_timestamp(), .imsi = std::move(imsi), .action = cdr_action::created};
        write_record(std::move(record));
    });

    _event_bus->subscribe<events::delete_session_event>([this](std::string imsi) {
        cdr_record record{
                .timestamp = utility::get_current_timestamp(), .imsi = std::move(imsi), .action = cdr_action::deleted};
        write_record(std::move(record));
    });

    _event_bus->subscribe<events::reject_session_event>([this](std::string imsi) {
        cdr_record record{
                .timestamp = utility::get_current_timestamp(), .imsi = std::move(imsi), .action = cdr_action::rejected};
        write_record(std::move(record));
    });
}

cdr_writer::~cdr_writer() {
    std::lock_guard<std::mutex> lock(_file_mutex);
    if (_file.is_open()) {
        _file.close();
    }
}

void cdr_writer::write_record(const cdr_record &record) {
    std::lock_guard<std::mutex> lock(_file_mutex);

    if (not _file.is_open()) {
        throw cdr_writer_exception("File is not open for writing");
    }

    std::string_view action_str = magic_enum::enum_name(record.action);

    _file << record.timestamp << ", " << record.imsi << ", " << action_str << '\n';
    _file.flush();
}
