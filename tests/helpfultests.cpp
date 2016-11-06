#include "gtest/gtest.h"
#include "qzebradev/helpful.h"
#include "qzebradev/gtesthelpful.h"

using namespace QZebraDev;


class HelpfulTests : public ::testing::Test
{

};

struct Class {
    static QString className() { return Helpful::className(Q_FUNC_INFO); }
    static QString methodName() { return Helpful::methodName(Q_FUNC_INFO); }
};

TEST_F(HelpfulTests, ClassName)
{
    EXPECT_EQ_STR(Class::className(), "Class");
    EXPECT_EQ_STR(Helpful::className(Q_FUNC_INFO), "HelpfulTests_ClassName_Test");
}

TEST_F(HelpfulTests, MethodName)
{
    EXPECT_EQ_STR(Class::methodName(), "methodName()");
    EXPECT_EQ_STR(Helpful::methodName(Q_FUNC_INFO), "TestBody()");
}
