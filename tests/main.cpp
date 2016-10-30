
#include <QCoreApplication>
#include "gtest/include/gtest/gtest.h"


int main(int argc, char **argv)
{

    volatile QCoreApplication *qapp = new QCoreApplication(argc, argv);
    Q_UNUSED(qapp);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
