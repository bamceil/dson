#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "dson.hpp"

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <sstream>

#if 1
#include <iostream>
#endif

using namespace std;

namespace dson {

class dson_generate_context {
public:
    string stringify(const shared_ptr<dson_value>& root);

private:
    void stringify_string(const string& str);

private:
    stringstream sstream_;

    static constexpr char HEX_DIGITS[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
};

class dson_parse_context {
public:
    explicit dson_parse_context(const string_view& view) : view_(view) {}

public:
    void skip_whitespace() { view_.remove_prefix(min(view_.find_first_not_of(" \t\n\r"), view_.size())); }

    error_type parse(const shared_ptr<dson_value>& value);

    bool is_completed() const { return view_.empty(); }

#if 1
    bool is_empty() const { return vec_.empty(); }
#endif

private:
    void encode_utf8(unsigned int u);
    pair<unsigned int, bool> parse_hex4(string_view& view);
    pair<string, error_type> parse_string();
    error_type parse_null(const shared_ptr<dson_value>& value);
    error_type parse_false(const shared_ptr<dson_value>& value);
    error_type parse_true(const shared_ptr<dson_value>& value);
    error_type parse_number(const shared_ptr<dson_value>& value);
    error_type parse_string(const shared_ptr<dson_value>& value);
    error_type parse_array(const shared_ptr<dson_value>& value);
    error_type parse_object(const shared_ptr<dson_value>& value);

private:
    string_view view_;
    vector<char> vec_;  // 解析字符串的临时存储
};

error_type dson_parse_context::parse_null(const shared_ptr<dson_value>& value) {
    assert(value);
    if (view_.size() < 4) return error_type::DSON_INVALID_VALUE;
    if (view_[0] != 'n' || view_[1] != 'u' || view_[2] != 'l' || view_[3] != 'l') return error_type::DSON_INVALID_VALUE;
    view_.remove_prefix(4);
    value->set_type(dson_type::DSON_NULL);
    return error_type::DSON_OK;
}

error_type dson_parse_context::parse_false(const shared_ptr<dson_value>& value) {
    assert(value);
    if (view_.size() < 5) return error_type::DSON_INVALID_VALUE;
    if (view_[0] != 'f' || view_[1] != 'a' || view_[2] != 'l' || view_[3] != 's' || view_[4] != 'e') return error_type::DSON_INVALID_VALUE;
    view_.remove_prefix(5);
    value->set_type(dson_type::DSON_FALSE);
    return error_type::DSON_OK;
}

error_type dson_parse_context::parse_true(const shared_ptr<dson_value>& value) {
    assert(value);
    if (view_.size() < 4) return error_type::DSON_INVALID_VALUE;
    if (view_[0] != 't' || view_[1] != 'r' || view_[2] != 'u' || view_[3] != 'e') return error_type::DSON_INVALID_VALUE;
    view_.remove_prefix(4);
    value->set_type(dson_type::DSON_TRUE);
    return error_type::DSON_OK;
}

error_type dson_parse_context::parse_number(const shared_ptr<dson_value>& value) {
    assert(value);
    string_view tmp(view_);
    if (tmp.front() == '-') tmp.remove_prefix(1);
    if (tmp.empty()) return error_type::DSON_INVALID_VALUE;
    if (tmp.front() == '0')
        tmp.remove_prefix(1);
    else {
        if (!isdigit(tmp.front())) return error_type::DSON_INVALID_VALUE;
        tmp.remove_prefix(min(tmp.find_first_not_of("123456789"), tmp.size()));
    }
    if (!tmp.empty() && tmp.front() == '.') {
        tmp.remove_prefix(1);
        if (tmp.empty() || !isdigit(tmp.front())) return error_type::DSON_INVALID_VALUE;
        tmp.remove_prefix(min(tmp.find_first_not_of("0123456789"), tmp.size()));
    }
    if (!tmp.empty() && (tmp.front() == 'e' || tmp.front() == 'E')) {
        tmp.remove_prefix(1);
        if (tmp.empty()) return error_type::DSON_INVALID_VALUE;
        if (tmp.front() == '+' || tmp.front() == '-') tmp.remove_prefix(1);
        if (tmp.empty() || !isdigit(tmp.front())) return error_type::DSON_INVALID_VALUE;
        tmp.remove_prefix(min(tmp.find_first_not_of("0123456789"), tmp.size()));
    }
    errno = 0;
    value->set_option_value(strtod(view_.data(), nullptr));
    double v = get<double>(value->option_value().value());
    if (errno == ERANGE && (v == HUGE_VAL || v == -HUGE_VAL)) return error_type::DSON_NUMBER_TOO_BIG;
    value->set_type(dson_type::DSON_NUMBER);
    view_ = tmp;
    return error_type::DSON_OK;
}

pair<string, error_type> dson_parse_context::parse_string() {
    view_.remove_prefix(1);
    string_view tmp(view_);
    int sz = vec_.size();
    while (!tmp.empty()) {
        char ch = tmp.front();
        switch (ch) {
            case '\"': {
                auto retp = make_pair(string(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end()), error_type::DSON_OK);
                vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
                tmp.remove_prefix(1);
                view_ = tmp;
                return retp;
            }
            case '\\':
                tmp.remove_prefix(1);
                if (tmp.empty()) {
                    vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
                    return make_pair("", error_type::DSON_INVALID_STRING_ESCAPE);
                }
                switch (tmp.front()) {
                    case '\"': vec_.push_back('\"'); break;
                    case '\\': vec_.push_back('\\'); break;
                    case '/': vec_.push_back('/'); break;
                    case 'b': vec_.push_back('\b'); break;
                    case 'f': vec_.push_back('\f'); break;
                    case 'n': vec_.push_back('\n'); break;
                    case 'r': vec_.push_back('\r'); break;
                    case 't': vec_.push_back('\t'); break;
                    case 'u': {
                        tmp.remove_prefix(1);
                        auto [u, err] = parse_hex4(tmp);
                        if (!err) return make_pair("", error_type::DSON_INVALID_UNICODE_HEX);
                        if (u >= 0xD800 && u <= 0xDBFF) {
                            if (tmp.size() < 2 || tmp[0] != '\\' || tmp[1] != 'u') {
                                vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
                                return make_pair("", error_type::DSON_INVALID_UNICODE_SURROGATE);
                            }
                            tmp.remove_prefix(2);
                            auto [un, e] = parse_hex4(tmp);
                            if (!e) return make_pair("", error_type::DSON_INVALID_UNICODE_HEX);
                            if (un < 0xDC00 || un > 0xDFFF) {
                                vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
                                return make_pair("", error_type::DSON_INVALID_UNICODE_SURROGATE);
                            }
                            u = (((u - 0xD800) << 10) | (un - 0xDC00)) + 0x10000;
                        }
                        encode_utf8(u);
                        continue;
                    }
                    default: vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end()); return make_pair("", error_type::DSON_INVALID_STRING_ESCAPE);
                }
                break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
                    return make_pair("", error_type::DSON_INVALID_STRING_CHAR);
                }
                vec_.push_back(ch);
        }
        tmp.remove_prefix(1);
    }
    vec_.erase(vec_.begin() + (sz > 0 ? sz - 1 : sz), vec_.end());
    return make_pair("", error_type::DSON_MISS_QUOTATION_MARK);
}

error_type dson_parse_context::parse_string(const shared_ptr<dson_value>& value) {
    assert(value);
    auto [str, err] = parse_string();
    if (err == error_type::DSON_OK) {
        value->set_option_value(str);
        value->set_type(dson_type::DSON_STRING);
    }
    return err;
}

error_type dson_parse_context::parse_array(const shared_ptr<dson_value>& value) {
    assert(value);
    view_.remove_prefix(1);
    skip_whitespace();
    if (!view_.empty() && view_.front() == ']') {
        view_.remove_prefix(1);
        value->set_option_value(vector<shared_ptr<dson_value>>(0));
        value->set_type(dson_type::DSON_ARRAY);
        return error_type::DSON_OK;
    }
    vector<shared_ptr<dson_value>> tmp;
    while (true) {
        shared_ptr<dson_value> ptr(new dson_value);
        error_type err = parse(ptr);
        if (err != error_type::DSON_OK) return err;
        tmp.push_back(ptr);
        skip_whitespace();
        if (!view_.empty() && view_.front() == ',') {
            view_.remove_prefix(1);
            skip_whitespace();
        }
        else if (!view_.empty() && view_.front() == ']') {
            view_.remove_prefix(1);
            value->set_type(dson_type::DSON_ARRAY);
            value->set_option_value(tmp);
            return error_type::DSON_OK;
        }
        else
            return error_type::DSON_MISS_COMMA_OR_SQUARE_BRACKET;
    }
}

error_type dson_parse_context::parse_object(const shared_ptr<dson_value>& value) {
    assert(value);
    view_.remove_prefix(1);
    skip_whitespace();
    if (!view_.empty() && view_.front() == '}') {
        view_.remove_prefix(1);
        value->set_option_value(unordered_map<string, shared_ptr<dson_value>>());
        value->set_type(dson_type::DSON_OBJECT);
        return error_type::DSON_OK;
    }
    unordered_map<string, shared_ptr<dson_value>> mp;
    while (true) {
        if (view_.empty() || view_.front() != '"') return error_type::DSON_MISS_KEY;
        auto [str, err] = parse_string();
        if (err != error_type::DSON_OK) return err;
        skip_whitespace();
        if (view_.empty() || view_.front() != ':') return error_type::DSON_MISS_COLON;
        view_.remove_prefix(1);
        skip_whitespace();
        shared_ptr<dson_value> ptr(new dson_value);
        error_type e = parse(ptr);
        if (e != error_type::DSON_OK) return e;
        mp[str] = move(ptr);
        // ----
        skip_whitespace();
        if (!view_.empty() && view_.front() == ',') {
            view_.remove_prefix(1);
            skip_whitespace();
        }
        else if (!view_.empty() && view_.front() == '}') {
            view_.remove_prefix(1);
            value->set_option_value(mp);
            value->set_type(dson_type::DSON_OBJECT);
            return error_type::DSON_OK;
        }
        else
            return error_type::DSON_MISS_COMMA_OR_CURLY_BRACKET;
    }
}

void dson_parse_context::encode_utf8(unsigned int u) {
    if (u <= 0x7F)
        vec_.push_back(u & 0xFF);
    else if (u <= 0x7FF) {
        vec_.push_back(0xC0 | ((u >> 6) & 0xFF));
        vec_.push_back(0x80 | (u & 0x3F));
    }
    else if (u <= 0xFFFF) {
        vec_.push_back(0xE0 | ((u >> 12) & 0xFF));
        vec_.push_back(0x80 | ((u >> 6) & 0x3F));
        vec_.push_back(0x80 | (u & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        vec_.push_back(0xF0 | ((u >> 18) & 0xFF));
        vec_.push_back(0x80 | ((u >> 12) & 0x3F));
        vec_.push_back(0x80 | ((u >> 6) & 0x3F));
        vec_.push_back(0x80 | (u & 0x3F));
    }
}

pair<unsigned int, bool> dson_parse_context::parse_hex4(string_view& view) {
    if (view.size() < 4) return make_pair(0, false);
    unsigned int u = 0;
    for (int i = 0; i < 4; ++i) {
        char ch = view[i];
        u <<= 4;
        if (ch >= '0' && ch <= '9')
            u |= (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            u |= (ch - 'A' + 10);
        else if (ch >= 'a' && ch <= 'f')
            u |= (ch - 'a' + 10);
        else
            return make_pair(0, false);
    }
    view.remove_prefix(4);
    return make_pair(u, true);
}

error_type dson_parse_context::parse(const shared_ptr<dson_value>& value) {
    assert(value);
    if (view_.empty()) return error_type::DSON_EXPECT_VALUE;
    switch (view_.front()) {
        case 'n': return parse_null(value);
        case 'f': return parse_false(value);
        case 't': return parse_true(value);
        case '"': return parse_string(value);
        case '[': return parse_array(value);
        case '{': return parse_object(value);
        default: return parse_number(value);
    }
}

void dson_generate_context::stringify_string(const string& str) {
    sstream_ << '"';
    for (unsigned char c : str) {
        switch (c) {
            case '\"': sstream_ << "\\\""; break;
            case '\\': sstream_ << "\\\\"; break;
            case '\b': sstream_ << "\\b"; break;
            case '\f': sstream_ << "\\f"; break;
            case '\n': sstream_ << "\\n"; break;
            case '\r': sstream_ << "\\r"; break;
            case '\t': sstream_ << "\\t"; break;
            default:
                if (c < 0x20) {
                    sstream_ << "\\u00";
                    sstream_ << HEX_DIGITS[c >> 4];
                    sstream_ << HEX_DIGITS[c & 15];
                }
                else
                    sstream_ << c;
        }
    }
    sstream_ << '"';
}

string dson_generate_context::stringify(const shared_ptr<dson_value>& root) {
    assert(root);
    switch (root->type()) {
        case dson_type::DSON_NULL: sstream_ << "null"; break;
        case dson_type::DSON_FALSE: sstream_ << "false"; break;
        case dson_type::DSON_TRUE: sstream_ << "true"; break;
        case dson_type::DSON_NUMBER: sstream_ << get<double>(root->option_value().value()); break;
        case dson_type::DSON_STRING: stringify_string(get<string>(root->option_value().value())); break;
        case dson_type::DSON_ARRAY: {
            sstream_ << '[';
            auto arr = get<vector<shared_ptr<dson_value>>>(root->option_value().value());
            for (int i = 0; i < arr.size(); ++i) {
                if (i > 0) sstream_ << ',';
                stringify(arr[i]);
            }
            sstream_ << ']';
        } break;
        case dson_type::DSON_OBJECT: {
            sstream_ << '{';
            auto obj = get<unordered_map<string, shared_ptr<dson_value>>>(root->option_value().value());
            auto iter = obj.cbegin();
            for (int i = 0; i < obj.size(); ++i, ++iter) {
                if (i > 0) sstream_ << ',';
                stringify_string(iter->first);
                sstream_ << ':';
                stringify(iter->second);
            }
            sstream_ << '}';
        } break;
        default: assert(0 && "invaild type");
    }
    return sstream_.str();
}

}  // namespace dson

dson::error_type dson::dson_parser::parse(const std::string_view& json) {
    dson_parse_context ctx(json);
    ctx.skip_whitespace();
    error_type ret = ctx.parse(value_);
    if (ret == error_type::DSON_OK) {
        ctx.skip_whitespace();
        if (!ctx.is_completed()) {
            value_->set_type(dson_type::DSON_NULL);
            ret = error_type::DSON_ROOT_NOT_SINGULAR;
        }
    }
    assert(ctx.is_empty());
    return ret;
}

string dson::dson_generator::stringify_raw(const std::shared_ptr<dson_value>& root) {
    assert(root);
    dson_generate_context ctx;
    return ctx.stringify(root);
}

// int main(int argc, char const* argv[]) {
// #ifdef _WINDOWS
//     _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
// #endif
//     using namespace dson;
//     document doc;
//     error_type type = doc.parse(" { "
//                                 "\"null\" : null , "
//                                 "\"false\" : false , "
//                                 "\"true\" : true , "
//                                 "\"int\" : 123 , "
//                                 "\"str\" : \"abc\", "
//                                 "\"arr\" : [ 1, 2, 3 ],"
//                                 "\"obj\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
//                                 " } ");
//     auto root = doc.root();
//     cout << "endl" << endl;

//     return 0;
// }
