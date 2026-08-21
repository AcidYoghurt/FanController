#include "backends/NvidiaBackend.h"

#include <QLibrary>
#include <QTest>

class NvidiaBackendTest final : public QObject {
  Q_OBJECT

private slots:
  void discoversTelemetryAndPerFanControl() {
    const auto path = qEnvironmentVariable("THERMAL_CONTROL_NVML_LIBRARY");
    QVERIFY2(!path.isEmpty(), "fake NVML path not configured");
    QLibrary fake(path);
    QVERIFY(fake.load());
    const auto reset = reinterpret_cast<void (*)()>(fake.resolve("fakeNvmlReset"));
    const auto lastSpeed =
        reinterpret_cast<unsigned (*)()>(fake.resolve("fakeNvmlLastSpeed"));
    const auto defaultCalls =
        reinterpret_cast<unsigned (*)()>(fake.resolve("fakeNvmlDefaultCalls"));
    QVERIFY(reset && lastSpeed && defaultCalls);
    reset();

    NvidiaBackend backend;
    QVERIFY(backend.probe());
    QCOMPARE(backend.sensors().size(), 1);
    QCOMPARE(backend.sensors().constFirst().temperature, 62.0);
    QCOMPARE(backend.fans().size(), 2);
    QCOMPARE(backend.fans().at(0).rpm, 1450);
    QCOMPARE(backend.fans().at(1).rpm, 1570);
    QCOMPARE(backend.zones().size(), 2);
    QVERIFY(backend.info().capabilities.fullSpeed);
    QVERIFY(backend.info().capabilities.customCurve);

    const auto firstZone = backend.zones().constFirst().id;
    const auto secondZone = backend.zones().at(1).id;
    QVERIFY(backend.setDuty(firstZone, 50).success);
    QCOMPARE(lastSpeed(), 50u);
    QVERIFY(backend.setFullSpeed(secondZone).success);
    QCOMPARE(lastSpeed(), 1100u);
    QVERIFY(backend.setAutomatic(firstZone).success);
    QCOMPARE(defaultCalls(), 1u);
  }
};

QTEST_GUILESS_MAIN(NvidiaBackendTest)
#include "NvidiaBackendTest.moc"
