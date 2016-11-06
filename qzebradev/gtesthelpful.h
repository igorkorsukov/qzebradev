#ifndef GTESTHELPFUL_H
#define GTESTHELPFUL_H

#include <QDir>
#include <QFileInfo>

#define EXPECT_EQ_STR(f, s) EXPECT_EQ(QString(f).toStdString(), QString(s).toStdString())
#define EXPECT_EQ_DIRPATH(f, s) EXPECT_EQ(QDir(f), QDir(s))
#define EXPECT_EQ_FILEPATH(f, s) EXPECT_EQ(QFileInfo(f), QFileInfo(s))

#define ASSERT_EQ_STR(f, s) ASSERT_EQ(QString(f).toStdString(), QString(s).toStdString())

#endif // GTESTHELPFUL_H
