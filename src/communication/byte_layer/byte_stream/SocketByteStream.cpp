#include "SocketByteStream.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>
#include <utility>

#include "socket_libs.h"

using namespace communication;
using namespace communication::byte_layer;

SocketByteStream::SocketByteStream(file_descriptor_t socket_file_descriptor,
                                   std::shared_ptr<IConnectionSimulator> connection_simulator_incoming,
                                   std::shared_ptr<IConnectionSimulator> connection_simulator_outgoing)
    : socket_holder(std::make_shared<SocketHolder>(socket_file_descriptor))
    , connection_simulator_incoming(std::move(connection_simulator_incoming))
    , connection_simulator_outgoing(std::move(connection_simulator_outgoing)) {
  SocketSetNoneBlocking(socket_holder->file_descriptor);
}

std::shared_ptr<SocketByteStream> SocketByteStream::CreateClientSide(
    const std::shared_ptr<IConnectionSimulatorProvider> &connection_simulator_provider, uint16_t port) {
  file_descriptor_t socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_file_descriptor < 0) {
    throw std::runtime_error("socket failed");
  }

  struct sockaddr_in server_address {};
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);

  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (connect(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
    if (errno == ECONNREFUSED) {
      throw ConnectionRefusedException();
    }
    throw std::runtime_error("connect failed");
  }

  auto socket_byte_stream = std::make_shared<SocketByteStream>(
      socket_file_descriptor, connection_simulator_provider->make_incoming_connection_simulator(),
      connection_simulator_provider->make_outgoing_connection_simulator());

  socket_byte_stream->ConfigureSocket();

  return socket_byte_stream;
}

size_t SocketByteStream::ReadToDeque() {
  const int max_length = 1;
  uint8_t receive_buffer[max_length];

  ssize_t read_count = recv(socket_holder->file_descriptor, reinterpret_cast<char *>(receive_buffer), max_length, 0);
  if (read_count < 0) {
    if (errno == EAGAIN) {
      return 0;
    }
    throw std::runtime_error("recv failed");
  }

  for (ssize_t i = 0; i < read_count; ++i) {
    receive_deque.push_back(connection_simulator_incoming->Filter(receive_buffer[i]));
  }

  return read_count;
}

size_t SocketByteStream::Read(uint8_t *receive_buffer, size_t max_length) {
  size_t length = 0;
  while (length < max_length) {
    if (receive_deque.empty()) {
      auto read_count = ReadToDeque();
      if (read_count == 0) {
        return length;
      }
    }
    receive_buffer[length] = receive_deque.front();
    receive_deque.pop_front();
    length++;
  }
  return length;
}

std::string SocketByteStream::ReadStringBlocking(size_t max_length) {
  char *receive_buffer = new char[max_length + 1];

  size_t read_count = 0;

  while (read_count == 0) {
    read_count = Read(reinterpret_cast<uint8_t *>(receive_buffer), max_length);
  }

  receive_buffer[read_count] = '\0';
  std::string received(receive_buffer);
  delete[] receive_buffer;
  return received;
}

void SocketByteStream::Send(  // NOLINT(readability-make-member-function-const)
    const uint8_t *send_buffer, size_t length) {
  auto *modified_send_buffer = new uint8_t[length];

  for (size_t i = 0; i < length; ++i) {
    modified_send_buffer[i] = connection_simulator_outgoing->Filter(send_buffer[i]);
  }

  if (!catch_send_failed) {
    // broken pipe is handled in Connection / Connection Manager, connection will be closed there
    send(socket_holder->file_descriptor, reinterpret_cast<char *>(modified_send_buffer), length, MSG_NOSIGNAL);
  } else {
    ssize_t send_count =
        send(socket_holder->file_descriptor, reinterpret_cast<char *>(modified_send_buffer), length, 0);
    if (send_count < 0) {
      throw std::runtime_error("send failed");
    }
  }

  delete[] modified_send_buffer;
}

void SocketByteStream::SendString(const std::string &message) {
  Send(reinterpret_cast<const uint8_t *>(message.c_str()), message.size());
}

void SocketByteStream::SetConnectionSimulatorIncoming(std::shared_ptr<IConnectionSimulator> ConnectionSimulator) {
  connection_simulator_incoming = std::move(ConnectionSimulator);
}

void SocketByteStream::SetConnectionSimulatorOutgoing(std::shared_ptr<IConnectionSimulator> ConnectionSimulator) {
  connection_simulator_outgoing = std::move(ConnectionSimulator);
}

bool SocketByteStream::HasInput() {
  struct timeval timeout {
    0, 0
  };

  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(socket_holder->file_descriptor, &read_fds);

  int ready_count = select(socket_holder->file_descriptor + 1, &read_fds, nullptr, nullptr, &timeout);

  if (ready_count < 0) {
    throw(std::runtime_error("select failed"));
  } else if (ready_count > 0) {
    assert(ready_count == 1);
    assert(FD_ISSET(socket_holder->file_descriptor, &read_fds));
    return true;
  } else {
    return false;
  }
}
void SocketByteStream::ConfigureSocket() {
#ifndef _WIN32
  // Fix issue causing high latency. See: https://stackoverflow.com/a/39272176
  int tcp_no_delay_value = 1;
  if (setsockopt(socket_holder->file_descriptor, IPPROTO_TCP, TCP_NODELAY, (char *)&tcp_no_delay_value, sizeof(int)) ==
      -1) {
    throw std::runtime_error("setsockopt failed");
  }
#endif
}

void SocketByteStream::SetParamCatchSendFailed(bool catch_failed_param) {
  catch_send_failed = catch_failed_param;
}
