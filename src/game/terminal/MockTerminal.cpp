#include "MockTerminal.h"

using namespace game;
using namespace game::terminal;

bool MockTerminal::HasInput() {
  std::lock_guard<std::mutex> lock_guard(mutex);

  return !unused_inputs.empty();
}

int MockTerminal::GetInput() {
  std::lock_guard<std::mutex> lock_guard(mutex);

  int input = (unsigned char)unused_inputs.at(0);
  unused_inputs = unused_inputs.substr(1);
  return input;
}

void MockTerminal::SetScreen(const std::wstring& content) {
  std::lock_guard<std::mutex> lock_guard(mutex);

  last_output = content;
}

void MockTerminal::AddInput(char input) {
  std::lock_guard<std::mutex> lock_guard(mutex);

  unused_inputs += input;
}
std::wstring MockTerminal::GetLastOutput() {
  std::lock_guard<std::mutex> lock_guard(mutex);

  return last_output;
}
