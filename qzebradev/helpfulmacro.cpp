#include "helpfulmacro.h"

static const QString ClassSep("::");
static const QChar ArgBegin('(');
static const QChar Space(' ');

QString HelpfulMacro::className(const QString &funcInfo)
{
    int inx = funcInfo.lastIndexOf(ClassSep, funcInfo.indexOf(ArgBegin));
    int binx = funcInfo.lastIndexOf(Space, inx);
    return funcInfo.mid(binx, inx - binx);
}

QString HelpfulMacro::methodName(const QString &funcInfo)
{
    int inx = funcInfo.lastIndexOf(ClassSep, funcInfo.indexOf(ArgBegin));
    return inx > -1 ? funcInfo.mid(inx + 2, funcInfo.length() - inx) : funcInfo;
}
