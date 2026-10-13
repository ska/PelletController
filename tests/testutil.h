#ifndef TESTUTIL_H
#define TESTUTIL_H

#include <QByteArray>
#include <QMetaObject>
#include <QTest>
#include <QVector>

/* QTest::qExec for one test class of the runner: the options go to every
 * class, a function name only to the class that has it (an unknown name
 * makes QtTest exit), a class with none of the names given is skipped. */
inline int runTestClass(QObject *test, int argc, char *argv[])
{
    QVector<char *> args { argv[0] };
    bool named = false;
    bool mine = false;
    for(int i = 1; i < argc; i++)
    {
        const QByteArray a(argv[i]);
        if(a.startsWith('-'))
        {
            args.append(argv[i]);
            continue;
        }
        named = true;
        const QByteArray name = a.split(':').first() + "()";
        if(test->metaObject()->indexOfMethod(name.constData()) >= 0)
        {
            args.append(argv[i]);
            mine = true;
        }
    }
    if(named && !mine)
        return 0;
    return QTest::qExec(test, args.size(), args.data());
}

#endif // TESTUTIL_H
