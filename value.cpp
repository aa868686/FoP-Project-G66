//
// Created by amirf on 2/20/2026.
//

#include "value.h"
#include <stdexcept>
#include <limits>
#include <cmath>

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
            case value_type::type_float: {
                float f = v.float_val;
                if (f == (int)f) return std::to_string((int)f);
                char buf[32];
                snprintf(buf, sizeof(buf), "%.4g", f);
                return std::string(buf);
            }
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

    bool value_is_numeric(const Value& v) {
        if (v.type == value_type::type_int || v.type == value_type::type_float) return true;
        return false;
    }

    Value value_add(const Value& a, const Value& b) {
        Value val;
        if (value_is_numeric(a) && value_is_numeric(b)) {
            val.type = value_type::type_float;
            val.float_val = value_to_float(a) + value_to_float(b);
        }
        else {
            val.type = value_type::type_string;
            val.string_val = value_to_string(a) + value_to_string(b);
        }
        return val;
    }
    Value value_sub(const Value& a, const Value& b) {
        return value_make_float(value_to_float(a) - value_to_float(b));
    }
    Value value_mul(const Value& a, const Value& b) {
        return value_make_float(value_to_float(a) * value_to_float(b));
    }
    Value value_div(const Value& a, const Value& b) {
        Value val;
        val.type = value_type::type_float;
        if (std::fabs(value_to_float(b)) < 1e-9f)
            return value_make_float(std::numeric_limits<float>::infinity());
        else {
            return value_make_float(value_to_float(a) / value_to_float(b));
        }
    }

    bool value_eq(const Value& a, const Value& b) {
        if (value_is_numeric(a) && value_is_numeric(b)) {
            return (std::fabs(value_to_float(a) - value_to_float(b)) < 1e-9f);
        }
        else {
            return (value_to_string(a) == value_to_string(b));
        }
    }
    bool value_lt(const Value& a, const Value& b) {
        return (value_to_float(a) < value_to_float(b));
    }
    bool value_gt(const Value& a, const Value& b) {
        return (value_to_float(a) > value_to_float(b));
    }

    Value value_and(const Value& a, const Value& b) {
        if (value_to_bool(a) && value_to_bool(b)) return value_make_bool(true);
        return value_make_bool(false);
    }
    Value value_or (const Value& a, const Value& b) {
        if (value_to_bool(a) || value_to_bool(b)) return value_make_bool(true);
        return value_make_bool(false);
    }
    Value value_not(const Value& a) {
        if (value_to_bool(a)) return value_make_bool(false);
        return value_make_bool(true);
    }
    Value value_mod(const Value& a, const Value& b) {
        int bv = value_to_int(b);
        if (bv == 0) return value_make_int(0);
        return value_make_int(value_to_int(a) % bv);
    }
    Value value_round(const Value& a) {
        return value_make_int((int)std::round(value_to_float(a)));
    }
    Value value_abs(const Value& a) {
        return value_make_float(std::fabs(value_to_float(a)));
    }
}
