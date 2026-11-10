#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "scan.hpp"

TEST(ScanTest, String) {
    auto result = stdx::scan<std::string>("number", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), "number");
}

TEST(ScanTest, StringView) {
    std::string input = "hello";
    auto result = stdx::scan<std::string_view>(input, "{%s}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), "hello");
    EXPECT_EQ(std::get<0>(result->values()).data(), input.data());
}

TEST(ScanTest, SignedInteger) {
    auto result = stdx::scan<int32_t>("42", "{%d}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), 42);
}

TEST(ScanTest, NegativeInteger) {
    auto result = stdx::scan<int64_t>("-9000000000", "{}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), -9000000000);
}

TEST(ScanTest, Int8Limits) {
    auto min = stdx::scan<int8_t>("-128", "{%d}");
    auto max = stdx::scan<int8_t>("127", "{%d}");
    ASSERT_TRUE(min.has_value());
    ASSERT_TRUE(max.has_value());
    EXPECT_EQ(std::get<0>(min->values()), -128);
    EXPECT_EQ(std::get<0>(max->values()), 127);
}

TEST(ScanTest, UnsignedInteger) {
    auto result = stdx::scan<uint8_t>("255", "{%u}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), 255);
}

TEST(ScanTest, FloatingPoint) {
    auto result = stdx::scan<double>("-2.25", "{%f}");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(std::get<0>(result->values()), -2.25);
}

TEST(ScanTest, SeveralValues) {
    auto result =
        stdx::scan<std::string, int32_t, double>("name=Bob age=30 height=1.8", "name={} age={%d} height={%f}");
    ASSERT_TRUE(result.has_value());
    auto &[name, age, height] = result->values();
    EXPECT_EQ(name, "Bob");
    EXPECT_EQ(age, 30);
    EXPECT_DOUBLE_EQ(height, 1.8);
}

TEST(ScanTest, CvQualifiedTypes) {
    auto result = stdx::scan<const int32_t, const std::string>("7 abc", "{} {}");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<0>(result->values()), 7);
    EXPECT_EQ(std::get<1>(result->values()), "abc");
}

TEST(ScanTest, LiteralTextDoesNotMatch) {
    EXPECT_FALSE(stdx::scan<int32_t>("age 5", "years={}"));
    EXPECT_FALSE(stdx::scan<std::string>("xxname=Bob", "name={}"));
    EXPECT_FALSE(stdx::scan<int32_t>("5 end junk", "{} end"));
}

TEST(ScanTest, PlaceholderCountDoesNotMatchTypes) {
    EXPECT_FALSE((stdx::scan<int32_t, int32_t>("1", "{}")));
    EXPECT_FALSE(stdx::scan<int32_t>("1 2", "{} {}"));
}

TEST(ScanTest, InvalidInteger) {
    EXPECT_FALSE(stdx::scan<int32_t>("abc", "{%d}"));
    EXPECT_FALSE(stdx::scan<int32_t>("12abc", "{%d}"));
}

TEST(ScanTest, IntegerOutOfRange) {
    EXPECT_FALSE(stdx::scan<int8_t>("128", "{%d}"));
    EXPECT_FALSE(stdx::scan<int8_t>("-129", "{%d}"));
    EXPECT_FALSE(stdx::scan<uint8_t>("256", "{%u}"));
    EXPECT_FALSE(stdx::scan<uint32_t>("-1", "{%u}"));
}

TEST(ScanTest, InvalidFloat) {
    EXPECT_FALSE(stdx::scan<double>("abc", "{%f}"));
    EXPECT_FALSE(stdx::scan<double>("3.14abc", "{%f}"));
}

TEST(ScanTest, SpecifierDoesNotMatchType) {
    EXPECT_FALSE(stdx::scan<std::string>("42", "{%d}"));
    EXPECT_FALSE(stdx::scan<int32_t>("42", "{%s}"));
    EXPECT_FALSE(stdx::scan<float>("1.5", "{%u}"));
}

TEST(ScanTest, UnknownSpecifier) { EXPECT_FALSE(stdx::scan<int32_t>("1", "{%x}")); }

TEST(ScanTest, ReferenceTypeIsUnsupported) {
    auto result = stdx::scan<int &>("42", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "Unsupported target type");
}

TEST(ScanTest, BoolIsUnsupported) {
    auto result = stdx::scan<bool>("1", "{}");
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().message, "Unsupported target type");
}

static_assert(stdx::details::scannable<const uint64_t>);
static_assert(stdx::details::scannable<volatile double>);
static_assert(!stdx::details::scannable<int &>);
static_assert(!stdx::details::scannable<bool>);
static_assert(!stdx::details::scannable<char>);
static_assert(!stdx::details::scannable<std::vector<int>>);
