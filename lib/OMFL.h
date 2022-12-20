#pragma once

#include <istream>
#include <variant>
#include <vector>
#include <set>

namespace omfl {

    struct default_tag {};
    template<typename tag = default_tag>
    class omfl_section;
    struct storage_t;
    using int_t = int32_t;
    using float_t = float;
    using array_t = std::vector<storage_t>;
    enum type_t {
        INT = 0,
        FLOAT = 1,
        STRING = 2,
        BOOL = 3,
        ARRAY = 4
    };
    using variant_storage = std::variant<int_t, float_t, std::string, bool, array_t>;
    struct storage_t : public variant_storage {
        using variant_storage::variant;

        [[nodiscard]] bool IsInt() const {
            return this->index() == INT;
        }
        [[nodiscard]] int_t AsInt() const {
            return std::get<INT>(*this);
        }
        [[nodiscard]] int_t AsIntOrDefault(int_t default_value) const {
            if(!IsInt()) {
                return default_value;
            }
            return std::get<INT>(*this);
        }

        [[nodiscard]] bool IsFloat() const {
            return this->index() == FLOAT;
        }
        [[nodiscard]] float_t AsFloat() const {
            return std::get<FLOAT>(*this);
        }
        [[nodiscard]] float_t AsFloatOrDefault(float_t default_value) const {
            if(!IsFloat()) {
                return default_value;
            }
            return std::get<FLOAT>(*this);
        }
        [[nodiscard]] bool IsString() const {
            return this->index() == STRING;
        }
        [[nodiscard]] std::string AsString() const
        {
            return std::get<STRING>(*this);
        }
        [[nodiscard]] std::string AsStringOrDefault(std::string default_value) const {
            if(!IsString()) {
                return default_value;
            }
            return std::get<STRING>(*this);
        }
        [[nodiscard]] bool IsBool() const {
            return this->index() == BOOL;
        }
        [[nodiscard]] bool AsBool() const {
            return std::get<BOOL>(*this);
        }
        [[nodiscard]] bool AsBoolOrDefault(bool default_value) const {
            if(!IsBool()) {
                return default_value;
            }
            return std::get<BOOL>(*this);
        }
        [[nodiscard]] bool IsArray() const {
            return this->index() == ARRAY;
        }
        const storage_t& operator[](size_t ind) const {
            if(this->index() == ARRAY) {
                auto& vector = std::get<ARRAY>(*this);
                if(ind >= vector.size()) {
                    return *this;
                }
                return vector[ind];
            }
            throw "Error";
        }
    };
    enum type_pair_t {
        VALUE = 0,
        SECTION = 1
    };
    template<typename Tag = default_tag>
    struct pair_t {
        std::string name;
        std::variant<storage_t, omfl_section<Tag>> value;
        const storage_t& operator[](size_t ind) const {
            return std::get<0>(value)[ind];
        }
        const pair_t<Tag>& Get(const std::string& name_req) const {
            if(value.index() == SECTION) {
                return std::get<SECTION>(value).Get(name_req);
            }
            return *this;
        }
        [[nodiscard]] bool IsInt() const {
            return std::get<VALUE>(value).IsInt();
        }
        [[nodiscard]] int_t AsInt() const {
            return std::get<VALUE>(value).AsInt();
        }
        [[nodiscard]] int_t AsIntOrDefault(int_t default_value) const {
            return std::get<VALUE>(value).AsIntOrDefault(default_value);
        }
        [[nodiscard]] bool IsFloat() const {
            return std::get<VALUE>(value).IsFloat();
        }
        [[nodiscard]] float_t AsFloat() const {
            return std::get<VALUE>(value).AsFloat();
        }
        [[nodiscard]] float_t AsFloatOrDefault(float_t default_value) const {
            return std::get<VALUE>(value).AsFloatOrDefault(default_value);
        }
        [[nodiscard]] bool IsString() const {
            return std::get<VALUE>(value).IsString();
        }
        [[nodiscard]] std::string AsString() const {
            return std::get<VALUE>(value).AsString();
        }
        [[nodiscard]] std::string AsStringOrDefault(std::string default_value) const {
            return std::get<VALUE>(value).AsStringOrDefault(default_value);
        }
        [[nodiscard]] bool IsBool() const {
            return std::get<VALUE>(value).IsBool();
        }
        [[nodiscard]] bool AsBool() const {
            return std::get<VALUE>(value).AsBool();
        }
        [[nodiscard]] bool AsBoolOrDefault(bool default_value) const {
            return std::get<VALUE>(value).AsBoolOrDefault(default_value);
        }
        [[nodiscard]] bool IsArray() const {
            return std::get<VALUE>(value).IsArray();
        }
        bool operator<(const pair_t &second) const {
            return name < second.name;
        }
    };

    template<typename tag>
    class omfl_section {

        bool is_valid = true;
        std::set<pair_t<tag>> pairs_or_sub_sections;

    public:
        omfl_section(std::nullptr_t) : is_valid(false) {}
        omfl_section() = default;

        [[nodiscard]] bool valid() const { return is_valid; }
        const pair_t<tag>& Get(const std::string& name) const {
            auto ind = name.find('.');
            if (ind == std::string::npos) {
                auto elem = pairs_or_sub_sections.find({name});
                if (elem == pairs_or_sub_sections.end()) {
                    throw "Error";
                }
                return *elem;
            } else {
                std::string next_req = name.substr(ind+1, std::string::npos);
                std::string prev_name = name.substr(0, ind);
                auto elem = pairs_or_sub_sections.find({prev_name});
                if (elem == pairs_or_sub_sections.end()) {
                    throw "error";
                }
                return (*elem).Get(next_req);
            }
        }

        std::pair<const omfl_section<default_tag>*, bool> get_section(const std::string& name) const {
            auto ind = name.find('.');
            if(ind == std::string::npos) {
                auto element = pairs_or_sub_sections.find({name});
                if(element == pairs_or_sub_sections.end()) {
                    return {nullptr, false};
                }
                return {std::addressof(std::get<SECTION>(element->value)), true};
            } else {
                std::string next_req = name.substr(ind+1, std::string::npos);
                std::string prev_name = name.substr(0, ind);
                auto elem = pairs_or_sub_sections.find({prev_name});
                if (elem == pairs_or_sub_sections.end()) {
                    return {nullptr, false};
                }
                return (std::get<SECTION>(elem->value)).get_section(next_req);
            }
        }

        void set_valid(bool value) { is_valid = value; }

        bool add_pair(std::string &&name, storage_t &&value) {
            auto res = pairs_or_sub_sections.insert({std::forward<std::string>(name), std::forward<storage_t>(value)});
            return res.second;
        }

        bool add_section(std::string name, omfl_section &&section) {
            auto ind = name.find('.');
            if(ind == std::string::npos)
            {
                auto res = pairs_or_sub_sections.insert({std::forward<std::string>(name), std::forward<omfl_section>(section)});
                return res.second;
            }
            else
            {
                std::string next_req = name.substr(ind+1, std::string::npos);
                std::string prev_name = name.substr(0, ind);
                auto elem = pairs_or_sub_sections.find({prev_name});
                if(elem == pairs_or_sub_sections.end())
                {
                    add_section(prev_name, {});
                    elem = pairs_or_sub_sections.find({prev_name});
                }

                return std::remove_const_t<omfl_section*>(static_cast<const omfl_section<default_tag>*>(std::addressof(std::get<SECTION>(elem->value))))->add_section(next_req, std::move(section));
            }
        }
    };

    using section = omfl_section<default_tag>;

} //namespace omfl