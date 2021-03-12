#ifndef CROSSING_CONSOLES_POSITION_H
#define CROSSING_CONSOLES_POSITION_H

#include <cstdint>
#include <iterator>
#include <vector>

#include "networking/ISerializable.h"

typedef int coordinate_t;

class Position : public ISerializable {
 public:
  coordinate_t x;
  coordinate_t y;

  Position(int x, int y);

  void Set(int x_new, int y_new);

  void Serialize(std::vector<uint8_t>& into) const override;

  static Position Deserialize(std::vector<uint8_t>::iterator& from);

  Position operator+(const Position& other_position) const;
  Position operator-(const Position& other_position) const;
  Position operator*(const Position& other_position) const;
  Position operator/(const Position& other_position) const;

  bool operator==(const Position& other_position) const;
  bool operator!=(const Position& other_position) const;
  bool operator<(const Position& other_position) const;
  bool operator<=(const Position& other_position) const;
  bool operator>(const Position& other_position) const;
  bool operator>=(const Position& other_position) const;
};

typedef Position coordinate_size_t;
typedef Position coordinate_distance_t;
typedef Position coordinate_factor_t;
#endif  // CROSSING_CONSOLES_POSITION_H
