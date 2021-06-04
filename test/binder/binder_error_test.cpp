#include "gtest/gtest.h"
#include "test/mock/mock_catalog.h"

#include "src/binder/include/query_binder.h"
#include "src/parser/include/parser.h"

using namespace graphflow::parser;
using namespace graphflow::binder;

using ::testing::NiceMock;
using ::testing::Test;

class BinderErrorTest : public Test {

public:
    void SetUp() override { catalog.setUp(); }

    string getBindingError(const string& input) {
        try {
            auto parsedQuery = Parser::parseQuery(input);
            QueryBinder(catalog).bind(*parsedQuery);
        } catch (const invalid_argument& exception) { return exception.what(); }
        return string();
    }

private:
    NiceMock<TinySnbCatalog> catalog;
};

TEST_F(BinderErrorTest, DisconnectedGraph1) {
    string expectedException = "Disconnect query graph is not supported.";
    auto input = "MATCH (a:person), (b:person) RETURN COUNT(*);";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, DisconnectedGraph2) {
    string expectedException = "Disconnect query graph is not supported.";
    auto input = "MATCH (a:person) WITH * MATCH (b:person) RETURN COUNT(*);";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, NodeLabelNotExist) {
    string expectedException = "Node label PERSON does not exist.";
    auto input = "MATCH (a:PERSON) RETURN COUNT(*);";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, NodeRelNotConnect) {
    string expectedException = "Node b doesn't connect to edge with same type as e1.";
    auto input = "MATCH (a:person)-[e1:workAt]->(b:person) RETURN COUNT(*);";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, RepeatedRelName) {
    string expectedException =
        "Bind relationship e1 to relationship with same name is not supported.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person)<-[e1:knows]-(:person) RETURN COUNT(*);";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, RepeatedReturnColumnName1) {
    string expectedException = "Multiple result column with the same name b are not supported.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) RETURN a.age AS b, b;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, RepeatedReturnColumnName2) {
    string expectedException = "Multiple result column with the same name e1 are not supported.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) RETURN *, e1;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, WITHExpressionAliased) {
    string expectedException = "Expression in WITH multi be aliased (use AS).";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) WITH a.age RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindToDifferentVariableType1) {
    string expectedException = "a defined with conflicting type REL (expect NODE).";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) WITH e1 AS a MATCH (a) RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindToDifferentVariableType2) {
    string expectedException = "a defined with conflicting type INT32 (expect NODE).";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) WITH a.age + 1 AS a MATCH (a) RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindEmptyStar) {
    string expectedException =
        "RETURN or WITH * is not allowed when there are no variables in scope.";
    auto input = "RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindVariableNotInScope1) {
    string expectedException = "Variable a not defined.";
    auto input = "WITH a MATCH (a:person)-[e1:knows]->(b:person) RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindVariableNotInScope2) {
    string expectedException = "Variable foo not defined.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) WHERE a.age > foo RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindPropertyLookUpOnExpression) {
    string expectedException = "Type mismatch: expect NODE or REL, but a.age + 2 was INT32.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) RETURN (a.age + 2).age;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindPropertyNotExist) {
    string expectedException = "Node a does not have property foo.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) RETURN a.foo;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}

TEST_F(BinderErrorTest, BindIDArithmetic) {
    string expectedException = "id(a) has data type NODE_ID. A numerical data type was expected.";
    auto input = "MATCH (a:person)-[e1:knows]->(b:person) WHERE id(a) + 1 < id(b) RETURN *;";
    ASSERT_STREQ(expectedException.c_str(), getBindingError(input).c_str());
}