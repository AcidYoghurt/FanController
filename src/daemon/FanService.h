#pragma once

#include <QDBusContext>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class FanController;

class FanService final : public QObject, protected QDBusContext {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.linuxthermal.Control1")

public:
  explicit FanService(FanController *controller, QObject *parent = nullptr);

public slots:
  [[nodiscard]] QVariantMap GetStatus() const;
  [[nodiscard]] QVariantList GetCurve() const;
  [[nodiscard]] QVariantList GetCurveProfiles() const;
  [[nodiscard]] QVariantList GetZoneCurve(const QString &zoneId) const;
  [[nodiscard]] QString GetVersion() const;
  [[nodiscard]] bool SetMode(const QString &mode);
  [[nodiscard]] bool SetCurve(const QVariantList &curve);
  [[nodiscard]] bool SetZoneCurve(const QString &zoneId,
                                  const QVariantList &curve);
  [[nodiscard]] bool ApplyCurve(const QVariantList &curve);
  [[nodiscard]] bool ApplyZoneCurve(const QString &zoneId,
                                    const QVariantList &curve);
  [[nodiscard]] bool ResetCurve();
  [[nodiscard]] bool ResetZoneCurve(const QString &zoneId);
  [[nodiscard]] bool MockSetSensorFailure(bool enabled);
  [[nodiscard]] bool MockFailNextCommand();

signals:
  void StatusChanged(const QVariantMap &status);
  void ModeChanged(const QString &mode);
  void CurveChanged(const QVariantList &curve);
  void CurvesChanged(const QVariantList &profiles);

private:
  [[nodiscard]] bool authorizeCaller(QString *error) const;

  FanController *m_controller;
};
