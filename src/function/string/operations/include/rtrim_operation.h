#pragma once

#include <cassert>
#include <cstring>

#include "base_str_operation.h"

#include "src/common/types/include/ku_string.h"

using namespace std;
using namespace kuzu::common;

namespace kuzu {
namespace function {
namespace operation {

struct Rtrim {
    static inline void operation(
        ku_string_t& input, ku_string_t& result, ValueVector& resultValueVector) {
        BaseStrOperation::operation(input, result, resultValueVector, rtrim);
    }

    static uint32_t rtrim(char* data, uint32_t len) {
        auto counter = len - 1;
        for (; counter >= 0; counter--) {
            if (!isspace(data[counter])) {
                break;
            }
        }
        return counter + 1;
    }
};

} // namespace operation
} // namespace function
} // namespace kuzu
