#ifndef CROSSING_CONSOLES_KEEPALIVEMESSAGE_H
#define CROSSING_CONSOLES_KEEPALIVEMESSAGE_H

#include "Message.h"

class KeepAliveMessage : public Message {
 public:
  static MessageType message_type;

  // Draft message
  explicit KeepAliveMessage(address_t address);

  // Received message
  KeepAliveMessage(address_t address, MessageMetaData meta_data);

  [[nodiscard]] MessageType GetMessageType() const override;
};

#endif  // CROSSING_CONSOLES_KEEPALIVEMESSAGE_H
