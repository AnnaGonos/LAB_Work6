#include "parser.h"
#include <fstream>
#include <iostream>

using namespace omfl;

bool symbol_digits(char symbol) {
    return (symbol >= '0' && symbol <= '9');
}

bool symbol_numbers(char symbol) {
    return (symbol >= '0' && symbol <= '9') || symbol == '.';
}

bool symbol_abc(char symbol) {
    return (symbol >= 'a' && symbol <= 'z');
}

bool symbol_from_key(char symbol) {
    return ((symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z') || (symbol >= '0' && symbol <= '9') || (symbol == '-') || (symbol == '_'));
}

bool symbol_whitespace(char symbol) {
    return (symbol == ' ' || symbol == '\r' || symbol == '\t' || symbol == '\n');
}

int whitespace_ignore(std::stringstream& in) {
    while(!in.fail()) {
        int buffer = in.get();
        if(buffer == '#') {
            while(!in.fail() && in.get() != '\n');
        } else if(!symbol_whitespace(buffer)) {
            if(buffer < 0) {
                break;
            }
            return buffer;
        }
    }
    return '\0';
}

std::string parse_string(std::stringstream& in, bool& success) {
    std::string result_string = "";
    while(!in.fail()) {
        int buffer = in.get();
        if (buffer == '\"') {
            success = true;
            return result_string;
        }
        result_string += buffer;
    }
    success = false;
    return result_string;
}

int parse_func(std::stringstream& in, bool (*func)(char), std::string* name = nullptr) {
    while(!in.fail()) {
        int buffer = in.get();
        if(!func((char)buffer)) {
            return buffer;
        }
        if(name) {
            (*name) += (char) buffer;
        }
    }
    return '\0';
}

int parse_value(std::stringstream& in, storage_t& memory);

array_t parse_array(std::stringstream& in) {
    array_t array;
    int next_symbol = in.get();
    if(next_symbol == ']') {
        return array;
    }
    if (next_symbol < 0) {
        throw -1;
    }

    in.putback((char)next_symbol);
    while(true) {
        storage_t value;
        next_symbol = parse_value(in, value);
        array.push_back(value);
        if(symbol_whitespace(next_symbol)) {
            next_symbol = whitespace_ignore(in);
        }
        if(next_symbol == ']') {
            return array;
        }
        if(next_symbol != ',') {
            throw "Error";
        }
    }
}

int parse_value(std::stringstream& in, storage_t& memory) {
    std::string value_string = "";
    int next_symbol = '\0';

    value_string = parse_func(in, [](char symb) -> bool {return symb == ' '; }, nullptr);
    if ((value_string[0] >= '0' && value_string[0] <= '9') || value_string[0] == '-' || value_string[0] == '+' ) {
        next_symbol = parse_func(in, symbol_numbers, &value_string);
        std::stringstream local(value_string);
        auto dot = value_string.find('.');
        if(dot != std::string::npos) {
            if(dot == value_string.size() - 1) {
                throw -1;
            }
            if (!symbol_digits(value_string[dot - 1])) {
                throw -1;
            }
            float result;
            local >> result;
            memory = {result};
        } else {
            int32_t result;
            local >> result;
            memory = {result};
        }
        if(local.fail()) {
            throw -1;
        }
    } else if(value_string[0] == 't') {
        next_symbol = parse_func(in, symbol_abc, &value_string);
        if(value_string == "true") {
            memory = {true};
        } else {
            throw -1;
        }
    } else if(value_string[0] == 'f') {
        next_symbol = parse_func(in, symbol_abc, &value_string);
        if(value_string == "false") {
            memory = {false};
        } else {
            throw -1;
        }
    } else if(value_string[0] == '[') {
        auto array = parse_array(in);
        memory = std::move(array);
        next_symbol = in.get();
    } else if(value_string[0] == '\"') {
        value_string = "";
        bool success;
        value_string += parse_string(in, success);
        if(!success) {
            throw -1;
        }
        memory = {std::move(value_string)};
        next_symbol = in.get();
    }

    return next_symbol;
}

std::string parse_section(std::stringstream& in, section& current_section) {
    char first_symbol;
    while((first_symbol = whitespace_ignore(in)) != '\0') {
        if(first_symbol == '[') {
            std::string sub_name;
            for(std::string buffer; true; ) {
                int next_symbol = parse_func(in, symbol_from_key, &buffer);
                if(buffer.empty()) {
                    throw -1;
                }
                sub_name += buffer;
                buffer = "";
                if(next_symbol != '.') {
                    if(sub_name.empty() || next_symbol != ']') {
                        throw -1;
                    }
                    break;
                } else {
                    sub_name += '.';
                }
            }

            return sub_name;
        }

        if(!symbol_from_key(first_symbol))
            throw -1;

        std::string key_name = "";
        key_name += first_symbol;
        int next_symbol = parse_func(in, symbol_from_key, &key_name);

        if(symbol_whitespace(next_symbol)) {
            next_symbol = whitespace_ignore(in);
        }
        if(next_symbol != '=') {
            throw -1;
        }
        storage_t storage_value;
        next_symbol = parse_value(in, storage_value);
        if(!current_section.add_pair(std::move(key_name), std::move(storage_value))) {
            throw -1;
        }
        if(next_symbol == -1)
            break;
        if(!symbol_whitespace(next_symbol)) {
            throw -1;
        }
    }

    return "";
}

section omfl::parse(const std::filesystem::path &path) {
    std::ifstream file(path.string());
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str());
}

section omfl::parse(const std::string &str) {
    std::stringstream input(str);
    std::cout << input.str();

    try {
        section root;
        std::string sub_name = parse_section(input, root);
        while(!sub_name.empty()) {
            auto result = root.get_section(sub_name);
            std::string sub_name_next;
            if(result.second) {
                sub_name_next = parse_section(input, std::remove_const_t<section&>(*result.first));
            } else {
                section new_sub_section;
                sub_name_next = parse_section(input, new_sub_section);
                if(!root.add_section(sub_name, std::move(new_sub_section)))
                    throw -1;
            }
            sub_name = sub_name_next;
        }

        return root;
    } catch (...) {
        return {nullptr};
    }
}
