#include "logger.h"
#include <string>
#include <vector>
#include <fstream>

namespace core {
    void logger_log(logger& logger, log_entry entry) {
        if (!logger.enabled || (int)entry.level < (int)logger.min) return;
        logger.entries.push_back(entry);
    }
    void logger_clear(logger& logger) {
        logger.entries.clear();
    }
    void logger_save(logger& logger, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        for (const auto& entry: logger.entries) {
            file << "[Cycle: " << entry.cycle <<
            "] [Line: " << entry.line <<
            "] [CMD: " << entry.command << "] " <<
            entry.operation << " " <<
            entry.data << std::endl;
        }
    }


}