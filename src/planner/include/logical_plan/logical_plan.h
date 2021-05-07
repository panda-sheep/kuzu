#pragma once

#include "src/planner/include/logical_plan/operator/logical_operator.h"
#include "src/planner/include/logical_plan/schema.h"

namespace graphflow {
namespace planner {

class LogicalPlan {

public:
    LogicalPlan() : cost{0} { schema = make_unique<Schema>(); }

    explicit LogicalPlan(unique_ptr<Schema> schema) : schema{move(schema)}, cost{0} {}

    const LogicalOperator& getLastOperator();

    void appendOperator(shared_ptr<LogicalOperator> op);

    unique_ptr<LogicalPlan> copy() const;

public:
    shared_ptr<LogicalOperator> lastOperator;
    unique_ptr<Schema> schema;
    uint64_t cost;
};

} // namespace planner
} // namespace graphflow
