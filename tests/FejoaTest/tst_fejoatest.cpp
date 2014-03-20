#include <QString>
#include <QtTest>

class FejoaTest : public QObject
{
    Q_OBJECT

public:
    FejoaTest();

private Q_SLOTS:
    void testCyrptoInterface();
};

FejoaTest::FejoaTest()
{
}

void FejoaTest::testCyrptoInterface()
{
    QVERIFY2(true, "Failure");
}

QTEST_APPLESS_MAIN(FejoaTest)

#include "tst_fejoatesttest.moc"
