#include <QString>
#include <QtTest>

#include "cryptointerface.h"

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
    CryptoInterface *crypto = CryptoInterfaceSingleton::getCryptoInterface();

    QString privateKey;
    QString publicKey;
    QString certificate;
    WP::err error = crypto->generateKeyPair(certificate, publicKey, privateKey, "");
    QVERIFY2(error == WP::kOk, "key pair generation");

    const char *kTestString = "Fejoa encryption test string.";
    QByteArray input = kTestString;
    QByteArray encrypted;
    error = crypto->encyrptAsymmetric(input, encrypted, certificate);
    QVERIFY2(error == WP::kOk, "asymmetric encryption");

    QByteArray plain;
    error = crypto->decryptAsymmetric(encrypted, plain, privateKey, "", certificate);
    QVERIFY2(error == WP::kOk, "asymmetric decryption");

    QVERIFY2(plain == kTestString, "asymmetric decryption");
}

QTEST_APPLESS_MAIN(FejoaTest)

#include "fejoatest.moc"
