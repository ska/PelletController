#include <QCoreApplication>
#include <QTest>
#include <cstdio>

int runSerialProtoTests(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int failed = 0;
    failed += runSerialProtoTests(argc, argv);

    fprintf(stderr, "\n%s: %d failed test function(s)\n", failed ? "FAIL" : "OK", failed);
    return failed ? 1 : 0;
}
