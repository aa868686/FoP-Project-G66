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

}

#endif //FPROJECT_VALUE_H
