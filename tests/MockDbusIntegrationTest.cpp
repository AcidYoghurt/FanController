#include "Constants.h"
#include "DbusUtils.h"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

namespace {
QVariantList curve(int secondDuty = 50) {
  return {
      QVariantMap{{QStringLiteral("temperature"), 45},
                  {QStringLiteral("duty"), 40}},
      QVariantMap{{QStringLiteral("temperature"), 55},
                  {QStringLiteral("duty"), secondDuty}},
      QVariantMap{{QStringLiteral("temperature"), 65},
                  {QStringLiteral("duty"), 65}},
      QVariantMap{{QStringLiteral("temperature"), 75},
                  {QStringLiteral("duty"), 80}},
      QVariantMap{{QStringLiteral("temperature"), 85},
                  {QStringLiteral("duty"), 100}},
  };
}
} // namespace

class MockDbusIntegrationTest final : public QObject {
  Q_OBJECT

private:
  QDBusInterface interface() {
    return {
        QString::fromLatin1(LinuxThermal::Constants::ServiceName),
        QString::fromLatin1(LinuxThermal::Constants::ObjectPath),
        QString::fromLatin1(LinuxThermal::Constants::InterfaceName),
        QDBusConnection::systemBus(),
    };
  }

  QVariantMap status() {
    auto api = interface();
    const QDBusReply<QVariantMap> reply = api.call(QStringLiteral("GetStatus"));
    if (!reply.isValid()) {
      QTest::qFail(qPrintable(reply.error().message()), __FILE__, __LINE__);
      return {};
    }
    return LinuxThermal::Dbus::unwrapMap(reply.value());
  }

  void setMode(const QString &mode) {
    auto api = interface();
    const QDBusReply<bool> reply = api.call(QStringLiteral("SetMode"), mode);
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
    QVERIFY(reply.value());
    QCOMPARE(status().value(QStringLiteral("mode")).toString(), mode);
  }

private slots:
  void serviceVersion() {
    auto api = interface();
    const QDBusReply<QString> reply = api.call(QStringLiteral("GetVersion"));
    QVERIFY2(reply.isValid(), qPrintable(reply.error().message()));
    QCOMPARE(reply.value(), QString::fromLatin1(LinuxThermal::Constants::Version));
    QVERIFY(!status().value(QStringLiteral("sensors")).toList().isEmpty());
    const auto secureBoot =
        status().value(QStringLiteral("secureBoot")).toMap();
    QVERIFY(secureBoot.contains(QStringLiteral("state")));
    QVERIFY(secureBoot.contains(QStringLiteral("requirement")));
  }

  void allModes() {
    setMode(QStringLiteral("automatic"));
    setMode(QStringLiteral("full-speed"));
    const auto fans = status().value(QStringLiteral("fans")).toList();
    QVERIFY(!fans.isEmpty());
    int controllableFans = 0;
    for (const auto &value : fans) {
      const auto fan = value.toMap();
      if (!fan.value(QStringLiteral("controllable")).toBool())
        continue;
      ++controllableFans;
      QCOMPARE(fan.value(QStringLiteral("duty")).toInt(), 100);
    }
    QCOMPARE(controllableFans, 2);

    setMode(QStringLiteral("custom-curve"));
    const auto profiles =
        status().value(QStringLiteral("curveProfiles")).toList();
    QCOMPARE(profiles.size(), 2);
    const auto customFans = status().value(QStringLiteral("fans")).toList();
    for (const auto &value : customFans) {
      const auto fan = value.toMap();
      if (fan.value(QStringLiteral("controllable")).toBool())
        QVERIFY(fan.value(QStringLiteral("duty")).toInt() > 0);
    }
    setMode(QStringLiteral("automatic"));
  }

  void saveApplyAndResetCurve() {
    auto api = interface();
    const auto custom = curve(55);

    QDBusReply<bool> saveReply = api.call(QStringLiteral("SetCurve"), custom);
    QVERIFY2(saveReply.isValid(), qPrintable(saveReply.error().message()));
    QVERIFY(saveReply.value());

    QDBusReply<QVariantList> curveReply = api.call(QStringLiteral("GetCurve"));
    QVERIFY2(curveReply.isValid(), qPrintable(curveReply.error().message()));
    QCOMPARE(curveReply.value().size(), 5);

    QDBusReply<bool> applyReply =
        api.call(QStringLiteral("ApplyCurve"), custom);
    QVERIFY2(applyReply.isValid(), qPrintable(applyReply.error().message()));
    QVERIFY(applyReply.value());
    QCOMPARE(status().value(QStringLiteral("mode")).toString(),
             QStringLiteral("custom-curve"));

    QDBusReply<bool> resetReply = api.call(QStringLiteral("ResetCurve"));
    QVERIFY2(resetReply.isValid(), qPrintable(resetReply.error().message()));
    QVERIFY(resetReply.value());
  }

  void commandFailureIsReturned() {
    setMode(QStringLiteral("automatic"));
    auto api = interface();
    QDBusReply<bool> armReply = api.call(QStringLiteral("MockFailNextCommand"));
    QVERIFY2(armReply.isValid(), qPrintable(armReply.error().message()));
    QVERIFY(armReply.value());

    const QDBusReply<bool> modeReply =
        api.call(QStringLiteral("SetMode"), QStringLiteral("full-speed"));
    QVERIFY(!modeReply.isValid());
    QCOMPARE(modeReply.error().type(), QDBusError::Failed);
    QCOMPARE(status().value(QStringLiteral("mode")).toString(),
             QStringLiteral("automatic"));
  }

  void sensorFailureTriggersSafetyOverride() {
    setMode(QStringLiteral("custom-curve"));
    auto api = interface();
    QDBusReply<bool> failureReply =
        api.call(QStringLiteral("MockSetSensorFailure"), true);
    QVERIFY2(failureReply.isValid(),
             qPrintable(failureReply.error().message()));
    QVERIFY(failureReply.value());
    QTRY_VERIFY_WITH_TIMEOUT(
        status().value(QStringLiteral("safetyOverride")).toBool(), 2000);
    QCOMPARE(status().value(QStringLiteral("fans")).toList().constFirst().toMap().value(QStringLiteral("duty")).toInt(), 100);

    QDBusReply<bool> recoveryReply =
        api.call(QStringLiteral("MockSetSensorFailure"), false);
    QVERIFY2(recoveryReply.isValid(),
             qPrintable(recoveryReply.error().message()));
    QVERIFY(recoveryReply.value());
    QTRY_VERIFY_WITH_TIMEOUT(
        !status().value(QStringLiteral("safetyOverride")).toBool(), 2000);
    setMode(QStringLiteral("automatic"));
  }

  void unsafeCurveIsRejected() {
    auto api = interface();
    const auto unsafe = curve(20);
    const QDBusReply<bool> reply = api.call(QStringLiteral("SetCurve"), unsafe);
    QVERIFY(!reply.isValid());
    QCOMPARE(reply.error().type(), QDBusError::InvalidArgs);
  }
};

QTEST_GUILESS_MAIN(MockDbusIntegrationTest)
#include "MockDbusIntegrationTest.moc"
