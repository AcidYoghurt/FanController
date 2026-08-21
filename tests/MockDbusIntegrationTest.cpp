#include "Constants.h"
#include "DbusUtils.h"

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>

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
QVariantList plateauCurve(int duty) {
  return {
      QVariantMap{{QStringLiteral("temperature"), 30},
                  {QStringLiteral("duty"), duty}},
      QVariantMap{{QStringLiteral("temperature"), 84},
                  {QStringLiteral("duty"), duty}},
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

  void curveCommandsRoundTripThroughDbus() {
    auto api = interface();
    const QString zoneId = QStringLiteral("mock:cpu:zone");
    const QString fanId = QStringLiteral("mock:cpu:fan0");
    const QDBusReply<QVariantList> originalCurveReply =
        api.call(QStringLiteral("GetZoneCurve"), zoneId);
    QVERIFY2(originalCurveReply.isValid(),
             qPrintable(originalCurveReply.error().message()));

    const QList<int> requestedDuties{40, 43, 48, 55, 65, 75, 85, 95, 100};
    for (const int requestedDuty : requestedDuties) {
      const QDBusReply<bool> applyReply =
          api.call(QStringLiteral("ApplyZoneCurve"), zoneId,
                   plateauCurve(requestedDuty));
      QVERIFY2(applyReply.isValid(), qPrintable(applyReply.error().message()));
      QVERIFY(applyReply.value());

      const auto feedback = status();
      QCOMPARE(feedback.value(QStringLiteral("mode")).toString(),
               QStringLiteral("custom-curve"));
      const auto zones = feedback.value(QStringLiteral("zones")).toList();
      const auto fans = feedback.value(QStringLiteral("fans")).toList();
      const auto zone = std::find_if(
          zones.cbegin(), zones.cend(), [&zoneId](const auto &value) {
            return value.toMap().value(QStringLiteral("zoneId")).toString() ==
                   zoneId;
          });
      const auto fan = std::find_if(
          fans.cbegin(), fans.cend(), [&fanId](const auto &value) {
            return value.toMap().value(QStringLiteral("fanId")).toString() ==
                   fanId;
          });
      QVERIFY(zone != zones.cend());
      QVERIFY(fan != fans.cend());
      QCOMPARE(zone->toMap().value(QStringLiteral("currentDuty")).toInt(),
               requestedDuty);
      QCOMPARE(fan->toMap().value(QStringLiteral("duty")).toInt(),
               requestedDuty);
      QCOMPARE(fan->toMap().value(QStringLiteral("rpm")).toInt(),
               700 + requestedDuty * 44);
    }

    const QDBusReply<bool> restoreCurveReply =
        api.call(QStringLiteral("SetZoneCurve"), zoneId,
                 originalCurveReply.value());
    QVERIFY2(restoreCurveReply.isValid(),
             qPrintable(restoreCurveReply.error().message()));
    QVERIFY(restoreCurveReply.value());
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
