#include <gtest/gtest.h>

#include <thread>
#include <utility>

#include "../src/communication/byte_layer/byte_stream/SocketByteServer.h"
#include "../src/communication/byte_layer/connection_simulator/ConnectionSimulatorFlaky.h"

using namespace communication;
using namespace communication::byte_layer;

class ConnectionSimulator : public ::testing::Test {
 public:
  ConnectionSimulator()
      : perfect(std::make_shared<ConnectionSimulatorPerfect>())
      , flaky_perfect(std::make_shared<ConnectionSimulatorFlaky>(ConnectionSimulatorFlaky::Parameters::Perfect()))
      , flaky(std::make_shared<ConnectionSimulatorFlaky>(ConnectionSimulatorFlaky::Parameters(0, 10))) {
  }

  std::shared_ptr<ConnectionSimulatorPerfect> perfect;
  std::shared_ptr<ConnectionSimulatorFlaky> flaky_perfect;
  std::shared_ptr<ConnectionSimulatorFlaky> flaky;

  const char *server_to_client = "abc123abc123abc123abc123abc123abc123abc123abc123";
  const char *client_to_server = "123ABC123ABC123ABC123ABC123ABC123ABC123ABC123ABC";

  std::string server_received;
  std::string client_received;

  void TestWithConnectionSimulators(std::shared_ptr<IConnectionSimulator> server_incoming,
                                    std::shared_ptr<IConnectionSimulator> server_outgoing,
                                    std::shared_ptr<IConnectionSimulator> client_incoming,
                                    std::shared_ptr<IConnectionSimulator> client_outgoing) {
    SocketByteServer byte_server;

    std::thread client_thread([this, &client_incoming, &client_outgoing] {
      SocketByteStream byte_stream = *SocketByteStream::CreateClientSide();

      byte_stream.SetConnectionSimulatorIncoming(client_incoming);
      byte_stream.SetConnectionSimulatorOutgoing(client_outgoing);

      byte_stream.SendString(client_to_server);
      client_received = byte_stream.ReadString();
    });

    std::shared_ptr<SocketByteStream> byte_stream;

    int counter = 1000;
    while (!byte_stream && counter > 0) {
      byte_stream = byte_server.GetNewClient();
      std::this_thread::sleep_for(std::chrono::microseconds(100));
      counter--;
    }

    ASSERT_NE(counter, 0);
    ASSERT_TRUE(byte_stream);

    byte_stream->SetConnectionSimulatorIncoming(std::move(server_incoming));
    byte_stream->SetConnectionSimulatorOutgoing(std::move(server_outgoing));

    byte_stream->SendString(server_to_client);
    server_received = byte_stream->ReadString();

    client_thread.join();
  }
};

TEST_F(ConnectionSimulator, AllPerfect) {
  TestWithConnectionSimulators(perfect, perfect, perfect, perfect);
  EXPECT_EQ(server_to_client, client_received);
  EXPECT_EQ(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, AllFlakyPerfect) {
  TestWithConnectionSimulators(flaky_perfect, flaky_perfect, flaky_perfect, flaky_perfect);
  EXPECT_EQ(server_to_client, client_received);
  EXPECT_EQ(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, FlakyServerIncoming) {
  TestWithConnectionSimulators(flaky, perfect, perfect, perfect);
  EXPECT_EQ(server_to_client, client_received);
  EXPECT_NE(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, FlakyServerOutgoing) {
  TestWithConnectionSimulators(perfect, flaky, perfect, perfect);
  EXPECT_NE(server_to_client, client_received);
  EXPECT_EQ(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, FlakyClientIncoming) {
  TestWithConnectionSimulators(perfect, perfect, flaky, perfect);
  EXPECT_NE(server_to_client, client_received);
  EXPECT_EQ(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, FlakyClientOutgoing) {
  TestWithConnectionSimulators(perfect, perfect, perfect, flaky);
  EXPECT_EQ(server_to_client, client_received);
  EXPECT_NE(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, AllFlaky) {
  TestWithConnectionSimulators(flaky, flaky, flaky, flaky);
  EXPECT_NE(server_to_client, client_received);
  EXPECT_NE(client_to_server, server_received);
}

TEST_F(ConnectionSimulator, Length) {
  std::vector<u_int8_t> input;
  input.reserve(100);
  for (int i = 0; i < 100; ++i) {
    input.push_back(i);
  }

  ConnectionSimulatorFlaky::Parameters parameters(3, 10, 2);
  std::vector<u_int8_t> expected_output(input);
  for (size_t i = 0; i < input.size(); ++i) {
    if ((i - parameters.first_error) % (parameters.error_interval) < parameters.error_length) {
      expected_output.at(i) ^= 0xffu;
    }
  }

  ConnectionSimulatorFlaky connection_simulator(parameters);

  std::vector<u_int8_t> actual_output;
  actual_output.reserve(input.size());

  for (const auto &value : input) {
    actual_output.push_back(connection_simulator.Filter(value));
  }

  EXPECT_EQ(actual_output, expected_output);
}
