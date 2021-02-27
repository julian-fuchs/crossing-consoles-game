#include <gtest/gtest.h>

#include <random>

#include "../src/communication/low_level/MockInputStream.h"
#include "../src/communication/messages/KeepAliveMessage.h"
#include "../src/communication/messages/MessageCoder.h"
#include "../src/communication/messages/PayloadMessage.h"

TEST(MessageCoder, KeepAliveMessage) {
  address_t target_address = 1234;
  KeepAliveMessage original_message(target_address);

  auto encoded_message = MessageCoder::Encode(&original_message);

  MockInputStream mock_input_stream;
  mock_input_stream.AddData(encoded_message);

  auto decoded_message = MessageCoder::Decode(mock_input_stream);
  EXPECT_TRUE(mock_input_stream.IsEmpty());

  EXPECT_EQ(original_message.GetMessageType(), decoded_message->GetMessageType());
}

TEST(MessageCoder, PayloadMessage) {
  address_t target_address = 1234;
  std::vector<uint8_t> original_payload;

  int original_payload_length = 123;
  original_payload.reserve(original_payload_length);
  for (int i = 0; i < original_payload_length; ++i) {
    original_payload.push_back(i);
  }
  PayloadMessage original_message(target_address, original_payload);

  auto encoded_message = MessageCoder::Encode(&original_message);

  MockInputStream mock_input_stream;
  mock_input_stream.AddData(encoded_message);
  auto decoded_message = MessageCoder::Decode(mock_input_stream);
  EXPECT_TRUE(mock_input_stream.IsEmpty());

  EXPECT_EQ(original_message.GetMessageType(), decoded_message->GetMessageType());
  auto &pm = dynamic_cast<PayloadMessage &>(*decoded_message);
  const auto &decoded_payload = pm.GetPayload();

  ASSERT_EQ(original_payload_length, pm.GetPayload().size());

  for (int i = 0; i < original_payload_length; ++i) {
    EXPECT_EQ(original_payload.at(i), decoded_payload.at(i));
  }
}

TEST(MessageCoder, InputTooShortException) {
  address_t target_address = 1234;
  std::vector<uint8_t> original_payload;

  int original_payload_length = 123;
  original_payload.reserve(original_payload_length);
  for (int i = 0; i < original_payload_length; ++i) {
    original_payload.push_back(i);
  }
  PayloadMessage original_message(target_address, original_payload);

  auto encoded_message = MessageCoder::Encode(&original_message);

  for (size_t i = 0; i < encoded_message.size(); ++i) {
    EXPECT_THROW(MessageCoder::Decode(encoded_message.data(), i), MessageCoder::InputTooShortException);
  }
}

TEST(MessageCoder, CrcIncorrectException) {
  // Create a message
  address_t target_address = 1234;
  std::vector<uint8_t> original_payload;

  int original_payload_length = 185;
  original_payload.reserve(original_payload_length);
  for (int i = 0; i < original_payload_length; ++i) {
    original_payload.push_back(i);
  }
  PayloadMessage original_message(target_address, original_payload);
  auto encoded_message = MessageCoder::Encode(&original_message);

  // Change random bytes in message
  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, encoded_message.size() - 1);

  for (int i = 0; i < 5; i++) {
    int position = dist(rd);
    encoded_message.at(position) = rd();
  }

  EXPECT_THROW(MessageCoder::Decode(encoded_message.data(), encoded_message.size()),
               MessageCoder::CrcIncorrectException);
}
