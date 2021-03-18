#ifndef CROSSING_CONSOLES_MOCK_TERMINAL_H
#define CROSSING_CONSOLES_MOCK_TERMINAL_H

#include <mutex>
#include <string>

#include "ITerminal.h"

namespace game::terminal {

/**
 * \brief Used for simulating input and capturing output during testing.
 */
class MockTerminal : public ITerminal {
 public:
  MockTerminal()
      : mutex() {
  }

  bool HasInput() override;
  int GetInput() override;

  void SetScreen(const std::wstring &content) override;

  void AddInput(char input);

  [[nodiscard]] std::wstring GetLastOutput();

 private:
  std::wstring last_output;
  std::wstring unused_inputs;

  /// Needed because this object can be used by multiple threads at the same time.
  std::mutex mutex;
};

}  // namespace game::terminal

#endif  // CROSSING_CONSOLES_MOCK_TERMINAL_H
