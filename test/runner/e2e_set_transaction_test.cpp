#include "test/test_utility/include/test_helper.h"

using namespace kuzu::testing;

// Test manual transaction. Auto transaction is tested in update test.
class BaseSetNodePropTransactionTest : public DBTest {
public:
    void SetUp() override {
        DBTest::SetUp();
        readConn = make_unique<Connection>(database.get());
    }

    static void readAndAssertNodeProperty(
        Connection* conn, uint64_t nodeOffset, string propertyName, vector<string> groundTruth) {
        auto readQuery =
            "MATCH (a:person) WHERE a.ID=" + to_string(nodeOffset) + " RETURN a." + propertyName;
        auto result = conn->query(readQuery);
        auto resultStr = TestHelper::convertResultToString(*result);
        checkResult(resultStr, groundTruth);
    }

private:
    static void checkResult(vector<string>& result, vector<string>& groundTruth) {
        ASSERT_EQ(result.size(), groundTruth.size());
        for (auto i = 0u; i < result.size(); ++i) {
            ASSERT_STREQ(result[i].c_str(), groundTruth[i].c_str());
        }
    }

protected:
    unique_ptr<Connection> readConn;
};

class SetNodeStructuredPropTransactionTest : public BaseSetNodePropTransactionTest {
public:
    string getInputCSVDir() override { return "dataset/tinysnb/"; }

    void insertLongStrings1000TimesAndVerify(Connection* connection) {
        int numWriteQueries = 1000;
        for (int i = 0; i < numWriteQueries; ++i) {
            connection->query("MATCH (a:person) WHERE a.ID < 100 SET a.fName = "
                              "concat('abcdefghijklmnopqrstuvwxyz', string(a.ID+" +
                              to_string(numWriteQueries) + "))");
        }
        auto result = connection->query("MATCH (a:person) WHERE a.ID=2 RETURN a.fName");
        ASSERT_EQ(result->getNext()->getResultValue(0)->getStringVal(),
            "abcdefghijklmnopqrstuvwxyz" + to_string(numWriteQueries + 2));
    }
};

TEST_F(SetNodeStructuredPropTransactionTest,
    SingleTransactionReadWriteToFixedLengthStructuredNodePropertyNonNullTest) {
    conn->beginWriteTransaction();
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = 70;");
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"70"});
}

TEST_F(SetNodeStructuredPropTransactionTest,
    SingleTransactionReadWriteToStringStructuredNodePropertyNonNullTest) {
    conn->beginWriteTransaction();
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.fName = 'abcdefghijklmnopqrstuvwxyz';");
    readAndAssertNodeProperty(
        conn.get(), 0 /* node offset */, "fName", vector<string>{"abcdefghijklmnopqrstuvwxyz"});
}

TEST_F(SetNodeStructuredPropTransactionTest,
    SingleTransactionReadWriteToFixedLengthStructuredNodePropertyNullTest) {
    conn->beginWriteTransaction();
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = null;");
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{""});
}

TEST_F(SetNodeStructuredPropTransactionTest,
    SingleTransactionReadWriteToStringStructuredNodePropertyNullTest) {
    conn->beginWriteTransaction();
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
    auto result = conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.fName = null;");
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "fName", vector<string>{""});
}

TEST_F(SetNodeStructuredPropTransactionTest,
    Concurrent1Write1ReadTransactionInTheMiddleOfTransaction) {
    conn->beginWriteTransaction();
    readConn->beginReadOnlyTransaction();
    // read before update
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    readAndAssertNodeProperty(readConn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
    readAndAssertNodeProperty(readConn.get(), 0 /* nodeoffset */, "fName", vector<string>{"Alice"});
    // update
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = 70;");
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.fName = 'abcdefghijklmnopqrstuvwxyz'");
    // read after update but before commit
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"70"});
    readAndAssertNodeProperty(readConn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    readAndAssertNodeProperty(
        conn.get(), 0 /* node offset */, "fName", vector<string>{"abcdefghijklmnopqrstuvwxyz"});
    readAndAssertNodeProperty(
        readConn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
}

TEST_F(SetNodeStructuredPropTransactionTest, Concurrent1Write1ReadTransactionCommitAndCheckpoint) {
    conn->beginWriteTransaction();
    readConn->beginReadOnlyTransaction();
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = 70;");
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.fName = 'abcdefghijklmnopqrstuvwxyz'");
    readConn->commit();
    conn->commit();
    // read after commit
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"70"});
    readAndAssertNodeProperty(readConn.get(), 0 /* node offset */, "age", vector<string>{"70"});
    readAndAssertNodeProperty(
        conn.get(), 0 /* node offset */, "fName", vector<string>{"abcdefghijklmnopqrstuvwxyz"});
    readAndAssertNodeProperty(
        readConn.get(), 0 /* node offset */, "fName", vector<string>{"abcdefghijklmnopqrstuvwxyz"});
}

TEST_F(SetNodeStructuredPropTransactionTest, Concurrent1Write1ReadTransactionRollback) {
    conn->beginWriteTransaction();
    readConn->beginReadOnlyTransaction();
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = 70;");
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.fName = 'abcdefghijklmnopqrstuvwxyz'");
    readConn->commit();
    conn->rollback();
    // read after rollback
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    readAndAssertNodeProperty(readConn.get(), 0 /* node offset */, "age", vector<string>{"35"});
    readAndAssertNodeProperty(conn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
    readAndAssertNodeProperty(
        readConn.get(), 0 /* node offset */, "fName", vector<string>{"Alice"});
}

TEST_F(SetNodeStructuredPropTransactionTest,
    OpenReadOnlyTransactionTriggersTimeoutErrorForWriteTransaction) {
    getTransactionManager(*database)->setCheckPointWaitTimeoutForTransactionsToLeaveInMicros(
        10000 /* 10ms */);
    readConn->beginReadOnlyTransaction();
    conn->beginWriteTransaction();
    conn->query("MATCH (a:person) WHERE a.ID = 0 SET a.age = 70;");
    try {
        conn->commit();
        FAIL();
    } catch (TransactionManagerException& e) {
    } catch (Exception& e) { FAIL(); }
    readAndAssertNodeProperty(readConn.get(), 0 /* node offset */, "age", vector<string>{"35"});
}

TEST_F(SetNodeStructuredPropTransactionTest, SetNodeLongStringPropRollbackTest) {
    conn->beginWriteTransaction();
    conn->query("MATCH (a:person) WHERE a.ID=0 SET a.fName='abcdefghijklmnopqrstuvwxyz'");
    conn->rollback();
    auto result = conn->query("MATCH (a:person) WHERE a.ID=0 RETURN a.fName");
    ASSERT_EQ(result->getNext()->getResultValue(0)->getStringVal(), "Alice");
}

TEST_F(SetNodeStructuredPropTransactionTest, SetVeryLongStringErrorsTest) {
    conn->beginWriteTransaction();
    string veryLongStr = "";
    for (auto i = 0u; i < DEFAULT_PAGE_SIZE + 1; ++i) {
        veryLongStr += "a";
    }
    auto result = conn->query("MATCH (a:person) WHERE a.ID=0 SET a.fName='" + veryLongStr + "'");
    ASSERT_FALSE(result->isSuccess());
}

TEST_F(SetNodeStructuredPropTransactionTest, SetManyNodeLongStringPropCommitTest) {
    conn->beginWriteTransaction();
    insertLongStrings1000TimesAndVerify(conn.get());
    conn->commit();
    auto result = conn->query("MATCH (a:person) WHERE a.ID=0 RETURN a.fName");
    ASSERT_EQ(result->getNext()->getResultValue(0)->getStringVal(),
        "abcdefghijklmnopqrstuvwxyz" + to_string(1000));
}

TEST_F(SetNodeStructuredPropTransactionTest, SetManyNodeLongStringPropRollbackTest) {
    conn->beginWriteTransaction();
    insertLongStrings1000TimesAndVerify(conn.get());
    conn->rollback();
    auto result = conn->query("MATCH (a:person) WHERE a.ID=0 RETURN a.fName");
    ASSERT_EQ(result->getNext()->getResultValue(0)->getStringVal(), "Alice");
}

class SetNodeUnstrPropTransactionTest : public BaseSetNodePropTransactionTest {
public:
    string getInputCSVDir() override {
        return "dataset/unstructured-property-lists-updates-tests/";
    }

public:
    string existingIntVal = "123456";
    string existingStrVal = "abcdefghijklmn";
    string intVal = "677121";
    string sStrVal = "short";
    string lStrVal = "new-long-string";
};

// TODO(Semih): Uncomment when enabling ad-hoc properties
// TEST_F(SetNodeUnstrPropTransactionTest, FixedLenPropertyShortStringInTrx) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us123='" + sStrVal + "'");
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{intVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{sStrVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, FixedLenPropertyShortStringCommitNormalExecution) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us123='" + sStrVal + "'");
//    conn->commit();
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{intVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{sStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, FixedLenPropertyShortStringRollbackNormalExecution) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us123='" + sStrVal + "'");
//    conn->rollback();
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, FixedLenPropertyShortStringCommitRecovery) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us123='" + sStrVal + "'");
//    conn->commitButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{intVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{sStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, FixedLenPropertyShortStringRollbackRecovery) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us123='" + sStrVal + "'");
//    conn->rollbackButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, LongStringPropTestInTrx) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.us123='" + lStrVal + "'");
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{lStrVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, LongStringPropTestCommitNormalExecution) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.us123='" + lStrVal + "'");
//    conn->commit();
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{lStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, LongStringPropTestRollbackNormalExecution) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.us123='" + lStrVal + "'");
//    conn->rollback();
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, LongStringPropTestCommitRecovery) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.us123='" + lStrVal + "'");
//    conn->commitButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{lStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, LongStringPropTestRollbackRecovery) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.us123='" + lStrVal + "'");
//    conn->rollbackButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, InsertNonExistingProps) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui124=" + intVal + ",a.us125='" + lStrVal + "'");
//    readAndAssertNodeProperty(conn.get(), 123, "ui124", vector<string>{intVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us125", vector<string>{lStrVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "ui124", vector<string>{""});
//    readAndAssertNodeProperty(readConn.get(), 123, "us125", vector<string>{""});
//    conn->rollbackButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "ui124", vector<string>{""});
//    readAndAssertNodeProperty(conn.get(), 123, "us125", vector<string>{""});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, RemoveExistingProperties) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.ui123=null,a.us123=null");
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{""});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{""});
//    readAndAssertNodeProperty(readConn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us123", vector<string>{existingStrVal});
//    conn->commit();
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{""});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{""});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, RemoveNonExistingProperties) {
//    conn->beginWriteTransaction();
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.ui124=null,a.us125=null");
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us123", vector<string>{existingStrVal});
//    conn->rollback();
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, RemoveNewlyAddedProperties) {
//    conn->beginWriteTransaction();
//    conn->query(
//        "MATCH (a:person) WHERE a.ID=123 SET a.ui123=" + intVal + ",a.us125='" + lStrVal + "'");
//    conn->query("MATCH (a:person) WHERE a.ID=123 SET a.ui123=null,a.us125=null");
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{""});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us125", vector<string>{""});
//    readAndAssertNodeProperty(readConn.get(), 123, "ui123", vector<string>{existingIntVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us123", vector<string>{existingStrVal});
//    readAndAssertNodeProperty(readConn.get(), 123, "us125", vector<string>{""});
//    conn->commitButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    readAndAssertNodeProperty(conn.get(), 123, "ui123", vector<string>{""});
//    readAndAssertNodeProperty(conn.get(), 123, "us123", vector<string>{existingStrVal});
//    readAndAssertNodeProperty(conn.get(), 123, "us125", vector<string>{""});
//}

// static void insertALargeNumberOfProperties(
//    Connection* conn, const string& intParam, const string& strParam) {
//    for (auto i = 0u; i <= 200; ++i) {
//        conn->query("MATCH (a:person) WHERE a.ID=123 SET a.ui" + to_string(i) + "=" + intParam +
//                    ",a.us" + to_string(i) + "='" + strParam + "'");
//    }
//}

// static void validateInsertALargeNumberOfPropertiesSucceeds(
//    Connection* conn, const string& intParam, const string& strParam) {
//    for (auto i = 0u; i <= 200; ++i) {
//        BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//            conn, 123, "ui" + to_string(i), vector<string>{intParam});
//        BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//            conn, 123, "us" + to_string(i), vector<string>{strParam});
//    }
//}

// static void validateInsertALargeNumberOfPropertiesFails(
//    Connection* conn, const string& intParam, const string& strParam) {
//    for (auto i = 0u; i <= 200; ++i) {
//        if (i == 123) {
//            BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//                conn, 123, "ui123", vector<string>{intParam});
//            BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//                conn, 123, "us123", vector<string>{strParam});
//        } else {
//            BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//                conn, 123, "ui" + to_string(i), vector<string>{""});
//            BaseSetNodePropTransactionTest::readAndAssertNodeProperty(
//                conn, 123, "us" + to_string(i), vector<string>{""});
//        }
//    }
//}

// TODO(Semih): Uncomment when enabling ad-hoc properties
// TEST_F(SetNodeUnstrPropTransactionTest, InsertALargeNumberOfPropertiesInTrx) {
//    conn->beginWriteTransaction();
//    insertALargeNumberOfProperties(conn.get(), intVal, lStrVal);
//    validateInsertALargeNumberOfPropertiesSucceeds(conn.get(), intVal, lStrVal);
//    validateInsertALargeNumberOfPropertiesFails(readConn.get(), existingIntVal, existingStrVal);
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, InsertALargeNumberOfPropertiesCommitNormalExecution) {
//    conn->beginWriteTransaction();
//    insertALargeNumberOfProperties(conn.get(), intVal, lStrVal);
//    conn->commit();
//    validateInsertALargeNumberOfPropertiesSucceeds(conn.get(), intVal, lStrVal);
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, InsertALargeNumberOfPropertiesRollbackNormalExecution) {
//    conn->beginWriteTransaction();
//    insertALargeNumberOfProperties(conn.get(), intVal, lStrVal);
//    conn->rollback();
//    validateInsertALargeNumberOfPropertiesFails(conn.get(), existingIntVal, existingStrVal);
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, InsertALargeNumberOfPropertiesCommitRecovery) {
//    conn->beginWriteTransaction();
//    insertALargeNumberOfProperties(conn.get(), intVal, lStrVal);
//    conn->commitButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    validateInsertALargeNumberOfPropertiesSucceeds(conn.get(), intVal, lStrVal);
//}
//
// TEST_F(SetNodeUnstrPropTransactionTest, InsertALargeNumberOfPropertiesRollbackRecovery) {
//    conn->beginWriteTransaction();
//    insertALargeNumberOfProperties(conn.get(), intVal, lStrVal);
//    conn->rollbackButSkipCheckpointingForTestingRecovery();
//    createDBAndConn(); // run recovery
//    validateInsertALargeNumberOfPropertiesFails(conn.get(), existingIntVal, existingStrVal);
//}
