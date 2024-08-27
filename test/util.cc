#include "util.h"

namespace platanus {
const std::vector<std::size_t>& GenerateNumbers(std::size_t n) {
  std::random_device                         seed_gen;
  std::mt19937_64                            engine{seed_gen()};
  std::uniform_int_distribution<std::size_t> dist{0, std::numeric_limits<std::int32_t>::max()};

  static std::vector<std::size_t> values;
  static std::set<std::size_t>    unique_values;

  while (values.size() < n) {
    std::size_t i;
    do {
      i = dist(engine);
    } while (unique_values.find(i) != unique_values.end());

    values.push_back(i);
    unique_values.insert(i);
  }

  return values;
}
}  // namespace platanus