#ifndef CROSSING_CONSOLES_GAMESERVER_H
#define CROSSING_CONSOLES_GAMESERVER_H

#include <memory>

#include "../communication/connection_layer/connection/ServerSideConnectionManager.h"
#include "networking/Change.h"
#include "world/World.h"

namespace game {

/**
 * \brief Server to handle game logic and connections to GameClient.
 */
class GameServer {
 public:
  explicit GameServer(
      const common::coordinate_size_t &world_size,
      communication::ProtocolDefinition::timeout_t communication_timeout = communication::ProtocolDefinition::timeout);

  /**
   * \brief Handle connections to `GameClient`s. Should be called in a loop.
   */
  void RunIteration();

  [[nodiscard]] const world::World &GetWorld() const;

 private:
  /**
   * \brief Handle an `Event` caused by a `GameClient`.
   */
  void HandleEvent(const std::shared_ptr<communication::connection_layer::Event> &event);

  std::shared_ptr<world::World> world;

  std::shared_ptr<communication::connection_layer::ServerSideConnectionManager> server_manager;

  /// Used to send world updates at a constant frequency.
  std::chrono::time_point<std::chrono::steady_clock> last_world_sent;
  static constexpr auto send_world_interval = std::chrono::milliseconds(10);
};

}  // namespace game

#endif  // CROSSING_CONSOLES_GAMESERVER_H
