//
// Created by amirf on 2/20/2026.
//

#include "value.h"
#include <stdexcept>

namespace core {
    Value value_make_int(int v) {
        Value val;
        val.type = value_type::type_int;
        val.int_val = v;
        return val;
    }

    Value value_make_float(float v) {
        Value val;
        val.type = value_type::type_float;
        val.float_val = v;
        return val;
    }

    Value value_make_string(const std::string &v) {
        Value val;
        val.type = value_type::type_string;
        val.string_val = v;
        return val;
    }

    Value value_make_bool(bool v) {
        Value val;
        val.type = value_type::type_bool;
        val.bool_val = v;
        return val;
    }

    int value_to_int(const Value& v) {
        switch (v.type) {
            case value_type::type_int:
                return v.int_val;
            case value_type::type_float:
                return (int)v.float_val;
            case value_type::type_string:
                try {
                    return std::stoi(v.string_val);
                }
                catch(...) {
                    return 0;
                }
            case value_type::type_bool:
                return (int)v.bool_val;
            default: return 0;
        }
    }
    float value_to_float(const Value& v) {
        switch (v.type) {
            case value_type::type_int:
                return v.int_val;
            case value_type::type_float:
                return v.float_val;
            case value_type::type_string:
                try {
                    return std::stof(v.string_val);
                }
                catch(...) {
                    return 0;
                }
            case value_type::type_bool:
                return (float)(int)v.bool_val;
            default: return 0;
        }
    }
    std::string value_to_string(const Value& v) {
        switch (v.type) {
            case value_type::type_int:
                return std::to_string(v.int_val);
            case value_type::type_float:
                return std::to_string(v.float_val);
            case value_type::type_string:
                return v.string_val;
            case value_type::type_bool:
                if (v.bool_val) return "true";
                return "false";
            default: return "";
        }
    }
    bool value_to_bool(const Value& v) {
        switch (v.type) {
            case value_type::type_int:
                if (v.int_val == 0) return false;
                return true;
            case value_type::type_float:
                if (v.float_val == 0) return false;
                return true;
            case value_type::type_string:
                if (v.string_val == "" || v.string_val == "0") return false;
                return true;
            case value_type::type_bool:
                return v.bool_val;
            default: return false;
        }
    }
}
