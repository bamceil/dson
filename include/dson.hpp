#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dson {
enum class dson_type { DSON_NULL, DSON_FALSE, DSON_TRUE, DSON_NUMBER, DSON_STRING, DSON_ARRAY, DSON_OBJECT };

enum class error_type {
    DSON_OK,
    DSON_EXPECT_VALUE,
    DSON_INVALID_VALUE,
    DSON_ROOT_NOT_SINGULAR,
    DSON_NUMBER_TOO_BIG,
    DSON_MISS_QUOTATION_MARK,
    DSON_INVALID_STRING_ESCAPE,
    DSON_INVALID_STRING_CHAR,
    DSON_INVALID_UNICODE_HEX,
    DSON_INVALID_UNICODE_SURROGATE,
    DSON_MISS_COMMA_OR_SQUARE_BRACKET,
    DSON_MISS_KEY,
    DSON_MISS_COLON,
    DSON_MISS_COMMA_OR_CURLY_BRACKET,
};

class dson_value {
public:
    using value_type = std::variant<double, std::string, std::vector<std::shared_ptr<dson_value>>, std::unordered_map<std::string, std::shared_ptr<dson_value>>>;

public:
    dson_value() : type_(dson_type::DSON_NULL) {}

    constexpr dson_type type() const { return type_; }

    void set_type(dson_type type) { type_ = type; }

    std::optional<value_type>& option_value() { return val_; }
    // Must use with set_type
    void set_option_value(const value_type& v) { val_.emplace(std::move(v)); }

private:
    dson_type type_;
    std::optional<value_type> val_;
};

class dson_parser {
public:
    dson_parser() : value_(new dson_value) {}

    error_type parse(const std::string_view& json);

    std::shared_ptr<dson_value> root() { return value_; }

private:
    std::shared_ptr<dson_value> value_;
};

class dson_generator {
public:
    std::string stringify_raw(const std::shared_ptr<dson_value>& root);
};

}  // namespace dson
