// Copyright 2013 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2024 Yuya Asano <my_favorite_theory@yahoo.co.jp>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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