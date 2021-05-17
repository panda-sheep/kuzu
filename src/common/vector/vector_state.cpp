#include "src/common/include/vector/vector_state.h"

namespace graphflow {
namespace common {

VectorState::VectorState(bool initializeSelectedValuesPos, uint64_t capacity)
    : size{0}, currPos{-1}, numSelectedValues{0} {
    valuesPos = make_unique<uint64_t[]>(capacity);
    selectedValuesPos = valuesPos.get();
    if (initializeSelectedValuesPos) {
        // If the dataChunk won't be filtered, we initialize selectedValuesPos such as values at
        // all positions are selected.
        for (auto i = 0u; i < capacity; i++) {
            selectedValuesPos[i] = i;
        }
    }
}

shared_ptr<VectorState> VectorState::getSingleValueDataChunkState() {
    auto state = make_shared<VectorState>(true /* init SelectedValuesPos */, 1);
    state->size = 1;
    state->currPos = 0;
    return state;
}

shared_ptr<VectorState> VectorState::clone() {
    auto capacity = sizeof(valuesPos.get()) / sizeof(uint64_t);
    auto newState = make_shared<VectorState>(false /*initializeSelectedValuesPos*/, capacity);
    newState->size = size;
    newState->currPos = currPos;
    newState->numSelectedValues = numSelectedValues;
    memcpy(newState->valuesPos.get(), valuesPos.get(), capacity * sizeof(uint64_t));
    return newState;
}

} // namespace common
} // namespace graphflow