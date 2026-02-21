//
// Created by amirf on 2/20/2026.
//

#ifndef FOP_PROJECT_G66_VARIABLE_H
#define FOP_PROJECT_G66_VARIABLE_H
#include <unordered_map>
#include <string>
#include "value.h"

namespace core {
    struct Variable {
        std::string name;
        Value value;
    };

    struct variable_store {
        std::unordered_map<std::string, Variable> variables;
    };

    void variable_set(variable_store& store, const std::string& name, const core::Value& value);
    Value variable_get(const variable_store& store, const std::string& name);
    bool variable_exists(const variable_store& store, const std::string& name);
}
#endif //FOP_PROJECT_G66_VARIABLE_H
