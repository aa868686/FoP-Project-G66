//
// Created by amirf on 2/20/2026.
//

#ifndef FOP_PROJECT_G66_LOGGER_H
#define FOP_PROJECT_G66_LOGGER_H
#include <string>
#include <vector>
namespace core {

    enum struct log_level {
        info, warning, error
    };

    struct log_entry {
        int cycle = 0;
        int line = 0;
        std::string command;
        std::string operation;
        std::string data;
        log_level level = log_level::info;
    };

    struct logger {
        std::vector<log_entry> entries;
        bool enabled = true;
        log_level min = log_level::info;
    };

    void logger_log(logger& logger, log_entry entry);
    void logger_clear(logger& logger);
    void logger_save(logger& logger, const std::string& filename);
}

#endif //FOP_PROJECT_G66_LOGGER_H
