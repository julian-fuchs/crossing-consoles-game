#include "MessageCoder.h"

#include <cassert>
#include <memory>
#include <optional>

#include "../../ProtocolDefinition.h"
#include "../message/ConnectionRequestMessage.h"
#include "../message/ConnectionResponseMessage.h"
#include "../message/KeepAliveMessage.h"
#include "../message/PayloadMessage.h"

using namespace communication;
using namespace communication::message_layer;

template <typename T>
static void WriteToStream(std::vector<uint8_t> &output, T input, unsigned int input_length, bool escape_flags = true) {
  for (unsigned int i = 0; i < input_length; ++i) {
    unsigned int byte_position = input_length - 1 - i;  // to get big endian mode
    uint8_t val = *(reinterpret_cast<uint8_t *>(&input) + byte_position);

    if (escape_flags) {
      // insert escape sequence if needed
      if (val == ProtocolDefinition::escape || val == ProtocolDefinition::flag) {
        output.push_back(ProtocolDefinition::escape);
      }
    }
    output.push_back(val);
  }
}

template <typename T>
static T ReadFromStreamWithCRC(byte_layer::IInputByteStream &stream, unsigned int type_length,
                               CRCHandler *crc_handler = nullptr, bool escape_flags = true) {
  T output = 0;
  for (unsigned int i = 0; i < type_length; ++i) {
    unsigned int byte_position = type_length - 1 - i;  // to get big endian mode

    T byte = 0;
    stream.Read(reinterpret_cast<uint8_t *>(&byte), 1);

    if (escape_flags) {
      // remove escape sequence if needed
      if (byte == ProtocolDefinition::escape) {
        stream.Read(reinterpret_cast<uint8_t *>(&byte), 1);
        if (!(byte == ProtocolDefinition::escape || byte == ProtocolDefinition::flag)) {
          throw MessageCoder::InvalidMessageException();
        }
      } else if (byte == ProtocolDefinition::flag) {
        throw MessageCoder::InvalidMessageException();
      }
    }
    if (crc_handler != nullptr) {
      crc_handler->AppendByte(byte);
    }
    output |= byte << 8 * byte_position;
  }

  return output;
}

std::vector<uint8_t> MessageCoder::Encode(Message *message) {
  std::vector<uint8_t> output;

  WriteToStream(output, ProtocolDefinition::flag, sizeof(ProtocolDefinition::flag), false);
  output.push_back(static_cast<uint8_t>(message->GetMessageType()));

  switch (message->GetMessageType()) {
    case MessageType::KEEP_ALIVE:
    case MessageType::CONNECTION_REQUEST:
    case MessageType::CONNECTION_RESPONSE:
      // no payload necessary
      break;
    case MessageType::PAYLOAD:
      auto *payload_message = reinterpret_cast<PayloadMessage *>(message);
      const auto &payload = payload_message->GetPayload();
      ProtocolDefinition::payload_length_t payload_length = payload.size();

      WriteToStream(output, payload_length, sizeof(payload_length));

      // insert escape sequences if needed
      for (const auto &payload_byte : payload) {
        if (payload_byte == ProtocolDefinition::escape || payload_byte == ProtocolDefinition::flag) {
          output.push_back(ProtocolDefinition::escape);
        }
        output.push_back(payload_byte);
      }
      break;
  }

  // offset removes start flag from CRC calculation
  int crc_start_offset = sizeof(ProtocolDefinition::flag);
  crc_value_t crc = CRCHandler::CalculateCRCValue(output.data() + crc_start_offset, output.size() - crc_start_offset);
  WriteToStream(output, crc, crc_length);

  WriteToStream(output, ProtocolDefinition::flag, sizeof(ProtocolDefinition::flag), false);

  return output;
}

std::shared_ptr<Message> MessageCoder::Decode(byte_layer::IInputByteStream &stream, bool expect_start_sequence) {
  CRCHandler crc_handler = CRCHandler();

  if (expect_start_sequence) {
    auto found_start_sequence =
        ReadFromStreamWithCRC<ProtocolDefinition::flag_t>(stream, sizeof(ProtocolDefinition::flag), nullptr, false);
    assert(found_start_sequence == ProtocolDefinition::flag);
  }

  auto message_type_value = ReadFromStreamWithCRC<char>(stream, sizeof(char), &crc_handler);
  if (message_type_value < 0 || (message_type_value > static_cast<uint8_t>(MessageType::HIGHEST_ELEMENT))) {
    throw InvalidMessageException();
  }

  auto message_type = static_cast<MessageType>(message_type_value);

  // TODO: create real metadata
  MessageMetaData message_meta_data(123, 456);

  std::shared_ptr<Message> message;

  switch (message_type) {
    case MessageType::KEEP_ALIVE:
      // TODO: use real address (also applies to lines below
      message = std::make_shared<KeepAliveMessage>(0, message_meta_data);
      break;
    case MessageType::CONNECTION_REQUEST:
      message = std::make_shared<ConnectionRequestMessage>(0, message_meta_data);
      break;
    case MessageType::CONNECTION_RESPONSE:
      message = std::make_shared<ConnectionResponseMessage>(0, message_meta_data);
      break;
    case MessageType::PAYLOAD: {
      auto payload_length = ReadFromStreamWithCRC<ProtocolDefinition::payload_length_t>(
          stream, sizeof(ProtocolDefinition::payload_length_t), &crc_handler);

      std::vector<uint8_t> payload;
      payload.reserve(payload_length);

      // remove all escape sequences from payload, crc is calculated from payload + escape sequences
      for (int i = 0; i < payload_length; ++i) {
        uint8_t byte = 0;
        stream.Read(&byte, 1);
        crc_handler.AppendByte(byte);
        if (byte == ProtocolDefinition::escape) {
          stream.Read(reinterpret_cast<uint8_t *>(&byte), 1);
          crc_handler.AppendByte(byte);
          if (!(byte == ProtocolDefinition::escape || byte == ProtocolDefinition::flag)) {
            throw MessageCoder::InvalidMessageException();
          }
        } else if (byte == ProtocolDefinition::flag) {
          throw MessageCoder::InvalidMessageException();
        }
        payload.push_back(byte);
      }

      message = std::make_shared<PayloadMessage>(0, payload, message_meta_data);
      break;
    }
    default:
      assert(false);
  }

  auto read_crc = ReadFromStreamWithCRC<crc_value_t>(stream, crc_length);

  if (!crc_handler.CheckCRCValue(read_crc)) {
    throw CrcIncorrectException();
  }

  auto end_sequence =
      ReadFromStreamWithCRC<ProtocolDefinition::flag_t>(stream, sizeof(ProtocolDefinition::flag), nullptr, false);
  if (end_sequence != ProtocolDefinition::flag) {
    throw InvalidMessageException();
  }

  return message;
}