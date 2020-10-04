#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "dson.hpp"

#include <gtest/gtest.h>

using namespace dson;
using namespace std;

TEST(dson, parse_true) {
    dson_parser doc;
    EXPECT_EQ(doc.parse("true"), error_type::DSON_OK);
    auto& v = doc.root();
    EXPECT_EQ(v->type(), dson_type::DSON_TRUE);
}

TEST(dson, parse_false) {
    dson_parser doc;
    EXPECT_EQ(doc.parse("false"), error_type::DSON_OK);
    auto& v = doc.root();
    EXPECT_EQ(v->type(), dson_type::DSON_FALSE);
}

TEST(dson, parse_null) {
    dson_parser doc;
    EXPECT_EQ(doc.parse("null"), error_type::DSON_OK);
    auto& v = doc.root();
    EXPECT_EQ(v->type(), dson_type::DSON_NULL);
}

TEST(dson, parse_expect_value) {
    dson_parser doc;
    EXPECT_EQ(doc.parse(""), error_type::DSON_EXPECT_VALUE);
    auto& v = doc.root();
    EXPECT_EQ(v->type(), dson_type::DSON_NULL);

    EXPECT_EQ(doc.parse("   "), error_type::DSON_EXPECT_VALUE);
    auto& q = doc.root();
    EXPECT_EQ(q->type(), dson_type::DSON_NULL);
}

#define TEST_PARSE_INVALID_VALUE(json)                              \
    do {                                                            \
        EXPECT_EQ(doc.parse(json), error_type::DSON_INVALID_VALUE); \
        auto& v = doc.root();                                       \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                 \
    } while (0)

TEST(dson, parse_invalid_value) {
    dson_parser doc;
    TEST_PARSE_INVALID_VALUE("nu");
    TEST_PARSE_INVALID_VALUE("xxx");

    TEST_PARSE_INVALID_VALUE("+1");
    TEST_PARSE_INVALID_VALUE(".1415");
    TEST_PARSE_INVALID_VALUE("3.");
    TEST_PARSE_INVALID_VALUE("INF");
    TEST_PARSE_INVALID_VALUE("inf");
    TEST_PARSE_INVALID_VALUE("NaN");
    TEST_PARSE_INVALID_VALUE("nan");
    TEST_PARSE_INVALID_VALUE("NAN");
}

#define TEST_PARSE_ROOT_NOT_SINGULAR(json)                              \
    do {                                                                \
        EXPECT_EQ(doc.parse(json), error_type::DSON_ROOT_NOT_SINGULAR); \
        auto& v = doc.root();                                           \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                     \
    } while (0)

TEST(dson, parse_root_not_singular) {
    dson_parser doc;
    TEST_PARSE_ROOT_NOT_SINGULAR("null fa");

    TEST_PARSE_ROOT_NOT_SINGULAR("03142");
    TEST_PARSE_ROOT_NOT_SINGULAR("0x2");
    TEST_PARSE_ROOT_NOT_SINGULAR("0x0");
}

#define TEST_PARSE_NUMBER(expect, json)                   \
    do {                                                  \
        EXPECT_EQ(doc.parse(json), error_type::DSON_OK);  \
        auto& v = doc.root();                             \
        auto var = v->option_value();                     \
        EXPECT_EQ(std::get<double>(var.value()), expect); \
    } while (0)

TEST(dson, parse_number) {
    dson_parser doc;
    TEST_PARSE_NUMBER(0.0, "0");
    TEST_PARSE_NUMBER(0.0, "-0");
    TEST_PARSE_NUMBER(0.0, "-0.0");
    TEST_PARSE_NUMBER(2.0, "2");
    TEST_PARSE_NUMBER(-2.0, "-2");
    TEST_PARSE_NUMBER(4.5, "4.5");
    TEST_PARSE_NUMBER(-2.5, "-2.5");
    TEST_PARSE_NUMBER(3.1415, "3.1415");
    TEST_PARSE_NUMBER(2E10, "2E10");
    TEST_PARSE_NUMBER(2e10, "2e10");
    TEST_PARSE_NUMBER(3E+10, "3E+10");
    TEST_PARSE_NUMBER(4E-10, "4E-10");
    TEST_PARSE_NUMBER(-1E10, "-1E10");
    TEST_PARSE_NUMBER(-1e10, "-1e10");
    TEST_PARSE_NUMBER(-1E+10, "-1E+10");
    TEST_PARSE_NUMBER(-1E-10, "-1E-10");
    TEST_PARSE_NUMBER(3.14E+10, "3.14E+10");
    TEST_PARSE_NUMBER(3.14E-10, "3.14E-10");
}

#define TEST_PARSE_NUMBER_TOO_BIG(json)                              \
    do {                                                             \
        EXPECT_EQ(doc.parse(json), error_type::DSON_NUMBER_TOO_BIG); \
        auto& v = doc.root();                                        \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                  \
    } while (0)

TEST(dson, parse_number_too_big) {
    dson_parser doc;
    TEST_PARSE_NUMBER_TOO_BIG("2E400");
    TEST_PARSE_NUMBER_TOO_BIG("-2E400");
}

TEST(dson, dson_value_set_get) {
    dson_value v;
    v.set_type(dson_type::DSON_NUMBER);
    v.set_option_value(12.0);
    EXPECT_EQ(get<double>(v.option_value().value()), 12.0);

    v.set_option_value("hello, world");
    v.set_type(dson_type::DSON_STRING);
    EXPECT_EQ(get<string>(v.option_value().value()), "hello, world");
}

#define TEST_PARSE_STRING(json, expect)                               \
    do {                                                              \
        EXPECT_EQ(doc.parse(json), error_type::DSON_OK);              \
        auto& root = doc.root();                                      \
        EXPECT_EQ(get<string>(root->option_value().value()), expect); \
    } while (0)

TEST(dson, parse_string) {
    dson_parser doc;
    TEST_PARSE_STRING("\"\"", "");
    TEST_PARSE_STRING("\"Hello, world!\"", "Hello, world!");
    TEST_PARSE_STRING("\"welcome\\nto\"", "welcome\nto");
    TEST_PARSE_STRING("\"\\\" \\\\ \\b \\/ \\f \\n \\r \\t \"", "\" \\ \b / \f \n \r \t ");

    TEST_PARSE_STRING("\"\\u0024\"", "\x24");
    TEST_PARSE_STRING("\"\\u20AC\"", "\xE2\x82\xAC");
    TEST_PARSE_STRING("\"\\uD834\\uDD1E\"", "\xF0\x9D\x84\x9E");
}

#define TEST_PARSE_STRING_MISS_QUOTATION_MARK(json)                       \
    do {                                                                  \
        EXPECT_EQ(doc.parse(json), error_type::DSON_MISS_QUOTATION_MARK); \
        auto& v = doc.root();                                             \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                       \
    } while (0)

TEST(dson, parse_string_miss_quotation_mark) {
    dson_parser doc;
    TEST_PARSE_STRING_MISS_QUOTATION_MARK("\"");
    TEST_PARSE_STRING_MISS_QUOTATION_MARK("\"xxx");
}

#define TEST_PARSE_STRING_INVALID_ESCAPE(json)                              \
    do {                                                                    \
        EXPECT_EQ(doc.parse(json), error_type::DSON_INVALID_STRING_ESCAPE); \
        auto& v = doc.root();                                               \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                         \
    } while (0)

TEST(dson, parse_string_invalid_escape) {
    dson_parser doc;
    TEST_PARSE_STRING_INVALID_ESCAPE("\"\\h\"");
    TEST_PARSE_STRING_INVALID_ESCAPE("\"xx\\xxx\"");
    TEST_PARSE_STRING_INVALID_ESCAPE("\"\\0\"");
    TEST_PARSE_STRING_INVALID_ESCAPE("\"\\'\"");
}

#define TEST_PARSE_STRING_INVALID_CHAR(json)                              \
    do {                                                                  \
        EXPECT_EQ(doc.parse(json), error_type::DSON_INVALID_STRING_CHAR); \
        auto& v = doc.root();                                             \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                       \
    } while (0)

TEST(dson, parse_string_invalid_char) {
    dson_parser doc;
    TEST_PARSE_STRING_INVALID_CHAR("\"\x03\"");
    TEST_PARSE_STRING_INVALID_CHAR("\"\x1F\"");
    TEST_PARSE_STRING_INVALID_CHAR("\"cc\x1F\"");
    TEST_PARSE_STRING_INVALID_CHAR("\"\x1Ftt\"");
}

#define TEST_PARSE_UTF8_INVALID_HEX(json)                                 \
    do {                                                                  \
        EXPECT_EQ(doc.parse(json), error_type::DSON_INVALID_UNICODE_HEX); \
        auto& v = doc.root();                                             \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                       \
    } while (0)

TEST(dson, parse_utf8_invalid_hex) {
    dson_parser doc;
    TEST_PARSE_UTF8_INVALID_HEX("\"\\u012\"");
    TEST_PARSE_UTF8_INVALID_HEX("\"\\u0x00\"");
    TEST_PARSE_UTF8_INVALID_HEX("\"\\u 521\"");
}

#define TEST_PARSE_UTF8_INVALID_SURR(json)                                      \
    do {                                                                        \
        EXPECT_EQ(doc.parse(json), error_type::DSON_INVALID_UNICODE_SURROGATE); \
        auto& v = doc.root();                                                   \
        EXPECT_EQ(v->type(), dson_type::DSON_NULL);                             \
    } while (0)

TEST(dson, parse_utf8_invalid_surrogate) {
    dson_parser doc;
    TEST_PARSE_UTF8_INVALID_SURR("\"\\uD800\\uDBFF\"");
    TEST_PARSE_UTF8_INVALID_SURR("\"\\uD800\\uF000\"");
    TEST_PARSE_UTF8_INVALID_SURR("\"\\uDBFF\"");
    TEST_PARSE_UTF8_INVALID_SURR("\"\\uD800\"");
}

TEST(dson, parse_array) {
    dson_parser doc;
    EXPECT_EQ(doc.parse("[ ]"), error_type::DSON_OK);
    auto& root = doc.root();
    EXPECT_EQ(root->type(), dson_type::DSON_ARRAY);

    EXPECT_EQ(doc.parse("[ null, false, true,  3.1415, \"xyz\" ]"), error_type::DSON_OK);
    auto& tr = doc.root();
    EXPECT_EQ(tr->type(), dson_type::DSON_ARRAY);
    auto val = get<vector<shared_ptr<dson_value>>>(tr->option_value().value());
    EXPECT_EQ(val.size(), 5);
    EXPECT_EQ(val[0]->type(), dson_type::DSON_NULL);
    EXPECT_EQ(val[1]->type(), dson_type::DSON_FALSE);
    EXPECT_EQ(val[2]->type(), dson_type::DSON_TRUE);
    EXPECT_EQ(val[3]->type(), dson_type::DSON_NUMBER);
    EXPECT_EQ(val[4]->type(), dson_type::DSON_STRING);
    EXPECT_EQ(get<double>(val[3]->option_value().value()), 3.1415);
    EXPECT_EQ(get<string>(val[4]->option_value().value()), "xyz");

    EXPECT_EQ(doc.parse("[ [], [0], [0,1], [0,1,2] ]"), error_type::DSON_OK);
    auto& kt = doc.root();
    EXPECT_EQ(kt->type(), dson_type::DSON_ARRAY);
    val = get<vector<shared_ptr<dson_value>>>(kt->option_value().value());
    EXPECT_EQ(val.size(), 4);
    for (int i = 0; i < 4; ++i) {
        auto k = get<vector<shared_ptr<dson_value>>>(val[i]->option_value().value());
        EXPECT_EQ(k.size(), i);
        for (int j = 0; j < i; ++j) {
            auto v = get<double>(k[j]->option_value().value());
            EXPECT_EQ(v, j);
        }
    }
}

TEST(dson, parse_object) {
    dson_parser doc;
    EXPECT_EQ(doc.parse(" { }"), error_type::DSON_OK);
    auto& root = doc.root();
    EXPECT_EQ(root->type(), dson_type::DSON_OBJECT);

    EXPECT_EQ(error_type::DSON_OK, doc.parse(" { "
                                             "\"null\" : null , "
                                             "\"false\" : false , "
                                             "\"true\" : true , "
                                             "\"int\" : 123 , "
                                             "\"str\" : \"abc\", "
                                             "\"arr\" : [ 1, 2, 3 ],"
                                             "\"obj\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                                             " } "));
    auto& tr = doc.root();
    EXPECT_EQ(tr->type(), dson_type::DSON_OBJECT);
    auto value = get<unordered_map<string, shared_ptr<dson_value>>>(tr->option_value().value());
    EXPECT_EQ(value.size(), 7);

    auto& nullval = value["null"];
    EXPECT_TRUE(nullval);
    EXPECT_EQ(dson_type::DSON_NULL, nullval->type());

    auto& falseval = value["false"];
    EXPECT_TRUE(falseval);
    EXPECT_EQ(dson_type::DSON_FALSE, falseval->type());

    auto& trueval = value["true"];
    EXPECT_TRUE(trueval);
    EXPECT_EQ(dson_type::DSON_TRUE, trueval->type());

    auto& intval = value["int"];
    EXPECT_TRUE(intval);
    EXPECT_EQ(dson_type::DSON_NUMBER, intval->type());
    EXPECT_EQ(123, get<double>(intval->option_value().value()));

    auto& strval = value["str"];
    EXPECT_TRUE(strval);
    EXPECT_EQ(dson_type::DSON_STRING, strval->type());
    EXPECT_EQ("abc", get<string>(strval->option_value().value()));

    auto& arrval = value["arr"];
    EXPECT_TRUE(arrval);
    EXPECT_EQ(dson_type::DSON_ARRAY, arrval->type());
    auto arrV = get<vector<shared_ptr<dson_value>>>(arrval->option_value().value());
    EXPECT_EQ(3, arrV.size());
    for (int i = 1; i <= 3; ++i) EXPECT_EQ(get<double>(arrV[i - 1]->option_value().value()), i);

    auto& objval = value["obj"];
    EXPECT_TRUE(objval);
    EXPECT_EQ(dson_type::DSON_OBJECT, objval->type());
    auto objV = get<unordered_map<string, shared_ptr<dson_value>>>(objval->option_value().value());
    EXPECT_EQ(3, objV.size());
    auto& p1 = objV["1"];
    EXPECT_TRUE(p1);
    EXPECT_EQ(get<double>(p1->option_value().value()), 1);
    auto& p2 = objV["2"];
    EXPECT_TRUE(p2);
    EXPECT_EQ(get<double>(p2->option_value().value()), 2);
    auto& p3 = objV["3"];
    EXPECT_TRUE(p3);
    EXPECT_EQ(get<double>(p3->option_value().value()), 3);
}

int main(int argc, char* argv[]) {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
