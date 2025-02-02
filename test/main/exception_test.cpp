#include "include/main_test_helper.h"

using namespace kuzu::testing;

TEST_F(ApiTest, Exception) {
    unique_ptr<QueryResult> result;
    unique_ptr<PreparedStatement> preparedStatement;

    auto parser_error_query = "MATCH (a:person)";
    auto parser_error = "Parser exception: Query must conclude with RETURN clause (line: 1, "
                        "offset: 0)\n\"MATCH (a:person)\"\n ^^^^^";
    result = conn->query(parser_error_query);
    ASSERT_FALSE(result->isSuccess());
    ASSERT_STREQ(result->getErrorMessage().c_str(), parser_error);
    preparedStatement = conn->prepare(parser_error_query);
    ASSERT_FALSE(preparedStatement->isSuccess());
    ASSERT_STREQ(preparedStatement->getErrorMessage().c_str(), parser_error);

    auto binder_error_query = "MATCH (a:person) RETURN b";
    auto binder_error = "Binder exception: Variable b is not in scope.";
    result = conn->query(binder_error_query);
    ASSERT_FALSE(result->isSuccess());
    ASSERT_STREQ(result->getErrorMessage().c_str(), binder_error);
    preparedStatement = conn->prepare(binder_error_query);
    ASSERT_FALSE(preparedStatement->isSuccess());
    ASSERT_STREQ(preparedStatement->getErrorMessage().c_str(), binder_error);

    auto catalog_error_query = "MATCH (a:person) RETURN dummy(n)";
    auto catalog_error = "Catalog exception: DUMMY function does not exist.";
    result = conn->query(catalog_error_query);
    ASSERT_FALSE(result->isSuccess());
    ASSERT_STREQ(result->getErrorMessage().c_str(), catalog_error);
    preparedStatement = conn->prepare(catalog_error_query);
    ASSERT_FALSE(preparedStatement->isSuccess());
    ASSERT_STREQ(preparedStatement->getErrorMessage().c_str(), catalog_error);

    auto function_error_query = "MATCH (a:person) RETURN a.age + 'hh'";
    auto function_error =
        "Binder exception: Cannot match a built-in function for given function +(INT64,STRING). "
        "Supported inputs are\n(INT64,INT64) -> INT64\n(INT64,DOUBLE) -> DOUBLE\n(DOUBLE,INT64) "
        "-> DOUBLE\n(DOUBLE,DOUBLE) -> DOUBLE\n(UNSTRUCTURED,UNSTRUCTURED) -> "
        "UNSTRUCTURED\n(DATE,INT64) -> DATE\n(INT64,DATE) -> DATE\n(DATE,INTERVAL) -> "
        "DATE\n(INTERVAL,DATE) -> DATE\n(TIMESTAMP,INTERVAL) -> "
        "TIMESTAMP\n(INTERVAL,TIMESTAMP) -> TIMESTAMP\n(INTERVAL,INTERVAL) -> INTERVAL\n";
    result = conn->query(function_error_query);
    ASSERT_FALSE(result->isSuccess());
    ASSERT_STREQ(result->getErrorMessage().c_str(), function_error);
    preparedStatement = conn->prepare(function_error_query);
    ASSERT_FALSE(preparedStatement->isSuccess());
    ASSERT_STREQ(preparedStatement->getErrorMessage().c_str(), function_error);

    // TODO(Semih): Uncomment when enabling ad-hoc properties
    //    auto runtime_error_query = "MATCH (a:person) RETURN a.unstrDateProp + 'hh'";
    //    auto runtime_error = "Runtime exception: Cannot add `DATE` and `STRING`";
    //    result = conn->query(runtime_error_query);
    //    ASSERT_FALSE(result->isSuccess());
    //    ASSERT_STREQ(result->getErrorMessage().c_str(), runtime_error);
    //
    //    // test fetching result when query fails
    //    try {
    //        result->hasNext();
    //        FAIL();
    //    } catch (Exception& exception) {
    //        ASSERT_STREQ("Runtime exception: Cannot add `DATE` and `STRING`", exception.what());
    //    } catch (std::exception& exception) { FAIL(); }
}
