#include <gtest/gtest.h>
#include <utility.hpp>

class UtilityTest : public ::testing::Test {};

TEST_F(UtilityTest, ValidImsi) {
    EXPECT_TRUE(utility::is_valid_imsi("123456789012345"));
    EXPECT_TRUE(utility::is_valid_imsi("123456"));
    EXPECT_TRUE(utility::is_valid_imsi("12345678901"));
}

TEST_F(UtilityTest, InvalidImsiLength) {
    EXPECT_FALSE(utility::is_valid_imsi("12345"));
    EXPECT_FALSE(utility::is_valid_imsi("1234567890123456"));
    EXPECT_FALSE(utility::is_valid_imsi(""));
}

TEST_F(UtilityTest, InvalidImsiChars) {
    EXPECT_FALSE(utility::is_valid_imsi("12345a"));
    EXPECT_FALSE(utility::is_valid_imsi("123456-789"));
    EXPECT_FALSE(utility::is_valid_imsi("123 456"));
}

TEST_F(UtilityTest, DecodeValidBcd) {
    std::vector<uint8_t> packet = {0x01, 0x00, 0x05, 0x00, 0x21, 0x43, 0x65, 0x87};
    auto result = utility::decode_imsi_from_bcd(packet);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "12345678");
}

TEST_F(UtilityTest, DecodeWithPadding) {
    std::vector<uint8_t> packet = {0x01, 0x00, 0x04, 0x00, 0x21, 0x43, 0x65, 0xF7};
    auto result = utility::decode_imsi_from_bcd(packet);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "1234567");
}

TEST_F(UtilityTest, DecodePacketTooShort) {
    std::vector<uint8_t> packet = {0x01, 0x00};
    auto result = utility::decode_imsi_from_bcd(packet);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utility::decode_error::packet_too_short);
}

TEST_F(UtilityTest, DecodeInvalidImsiType) {
    std::vector<uint8_t> packet = {0x02, 0x00, 0x04, 0x00, 0x21, 0x43, 0x65, 0x87};
    auto result = utility::decode_imsi_from_bcd(packet);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utility::decode_error::invalid_imsi_type);
}

TEST_F(UtilityTest, DecodeInvalidBcdDigit) {
    std::vector<uint8_t> packet = {0x01, 0x00, 0x04, 0x00, 0x21, 0xA3, 0x65, 0x87};
    auto result = utility::decode_imsi_from_bcd(packet);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utility::decode_error::invalid_bcd_digit);
}

TEST_F(UtilityTest, EncodeValidImsi) {
    auto result = utility::encode_imsi_to_bcd("12345678");

    ASSERT_TRUE(result.has_value());
    std::vector<uint8_t> expected = {0x01, 0x00, 0x05, 0x00, 0x21, 0x43, 0x65, 0x87};
    EXPECT_EQ(result.value(), expected);
}

TEST_F(UtilityTest, EncodeOddLengthImsi) {
    auto result = utility::encode_imsi_to_bcd("1234567");

    ASSERT_TRUE(result.has_value());
    std::vector<uint8_t> expected = {0x01, 0x00, 0x05, 0x00, 0x21, 0x43, 0x65, 0xF7};
    EXPECT_EQ(result.value(), expected);
}

TEST_F(UtilityTest, EncodeInvalidImsi) {
    auto result = utility::encode_imsi_to_bcd("123a5");

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utility::encode_error::invalid_imsi_format);
}

TEST_F(UtilityTest, TimestampFormat) {
    auto timestamp = utility::get_current_timestamp();

    EXPECT_GT(timestamp.length(), 19);
    EXPECT_EQ(timestamp[4], '-');
    EXPECT_EQ(timestamp[7], '-');
    EXPECT_EQ(timestamp[10], ' ');
    EXPECT_EQ(timestamp[13], ':');
    EXPECT_EQ(timestamp[16], ':');
    EXPECT_EQ(timestamp[19], '.');
}
