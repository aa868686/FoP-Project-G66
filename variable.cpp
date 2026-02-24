//
// Created by amirf on 2/20/2026.
//
#include "variable.h"
#include <unordered_map>
#include <string>
namespace core {
    void variable_set(variable_store &store, const std::string &name, const core::Value &value) {
        if (store.variables.count(name) > 0) {
            store.variables[name].value = value;
        } else {
            Variable var;
            var.name = name;
            var.value = value;
            var.visible = true;
            store.variables[name] = var;
        }
    }

    core::Value variable_get(const variable_store &store, const std::string &name) {
        if (variable_exists(store, name)) return store.variables.at(name).value;
        else return value_make_int(0);
    }

    bool variable_exists(const variable_store &store, const std::string &name) {
        return (store.variables.count(name) > 0);
    }
}