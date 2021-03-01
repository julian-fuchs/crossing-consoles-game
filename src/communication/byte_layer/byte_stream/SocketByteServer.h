#ifndef CROSSING_CONSOLES_BYTE_SERVER_H
#define CROSSING_CONSOLES_BYTE_SERVER_H

#include <cstdint>
#include <optional>
#include <stdexcept>

#include "SocketByteStream.h"

/**
 * \brief Creates a socket to listen for clients and creates `SocketByteStream`s to communicate with them.
 */
class SocketByteServer {
 public:
  explicit SocketByteServer(uint16_t port = socket_default_port, int max_connections = 5);

  std::optional<SocketByteStream> GetNewClient();

 private:
  std::shared_ptr<SocketHolder> socket_holder;
};

#endif  // CROSSING_CONSOLES_BYTE_SERVER_H
