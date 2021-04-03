#include "HeightMap.h"

using namespace game;
using namespace game::world;
using namespace game::world::block_types;

void HeightMap::SetType(int height, BlockType type) {
  auto const result = height_types.insert(std::pair<int, BlockType>(height, type));
  if (!result.second) {
    result.first->second = type;
  }
}

BlockType HeightMap::GetType(int height) {
  if (height_types.empty()) {
    return EMPTY_BLOCK;
  }

  auto upper_bound = height_types.upper_bound(height);
  if (upper_bound == height_types.begin()) {
    return EMPTY_BLOCK;
  } else {
    return (--upper_bound)->second;
  }
}
