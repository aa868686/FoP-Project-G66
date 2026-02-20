//
// Created by amirf on 2/20/2026.
//

#ifndef FPROJECT_VALUE_H
#define FPROJECT_VALUE_H

#include <string>
namespace core {

    enum struct value_type {
        type_string, type_int, type_float, type_bool
    };

    struct Value {
        value_type type = value_type::type_int;
        int int_val = 0;
        float float_val = 0;
        std::string string_val = "";
        bool bool_val = false;
    };

    Value value_make_int(int v);
    Value value_make_float(float v);
    Value value_make_string(const std::string& v);
    Value value_make_bool(bool v);

    int value_to_int(const Value& v);
    float value_to_float(const Value& v);
    std::string value_to_string(const Value& v);
    bool value_to_bool(const Value& v);

    bool value_is_numeric(const Value& v);

    Value value_add(const Value& a, const Value& b);
    Value value_sub(const Value& a, const Value& b);
    Value value_mul(const Value& a, const Value& b);
    Value value_div(const Value& a, const Value& b);

    bool value_eq(const Value& a, const Value& b);
    bool value_lt(const Value& a, const Value& b);
    bool value_gt(const Value& a, const Value& b);

    Value value_and(const Value& a, const Value& b);
    Value value_or (const Value& a, const Value& b);
    Value value_not(const Value& a);

}

#endif //FPROJECT_VALUE_H
