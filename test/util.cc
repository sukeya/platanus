#include "util.h"

namespace platanus {
char* GenerateDigits(char buf[16], int val, int maxval) {
  EXPECT_LE(val, maxval);
  int p    = 15;
  buf[p--] = 0;
  while (maxval > 0) {
    buf[p--] = '0' + (val % 10);
    val /= 10;
    maxval /= 10;
  }
  return buf + p + 1;
}

const std::vector<int>& GenerateNumbers(int n, int maxval) {
  static std::vector<int> values;
  static std::set<int>    unique_values;

  if (values.size() < n) {
    for (int i = values.size(); i < n; i++) {
      int value;
      do {
        value = rand() % (maxval + 1);
      } while (unique_values.find(value) != unique_values.end());

      values.push_back(value);
      unique_values.insert(value);
    }
  }

  return values;
}
}