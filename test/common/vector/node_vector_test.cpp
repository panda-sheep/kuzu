#include "gtest/gtest.h"

#include "src/common/include/vector/node_vector.h"

TEST(ListTests, CreateSequenceNodeVectorTest) {
    auto vector = NodeIDSequenceVector();
    vector.setStartOffset(100);
    nodeID_t node;
    for (uint64_t i = 0; i < NODE_SEQUENCE_VECTOR_SIZE; i++) {
        vector.get(i, node);
        ASSERT_EQ(node.offset, 100 + i);
    }
}
