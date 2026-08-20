#pragma once

#include <QObject>

class SingleInstanceController final : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.linuxthermal.Control.Gui1")

public:
  explicit SingleInstanceController(QObject *parent = nullptr);
  ~SingleInstanceController() override;

  [[nodiscard]] bool isPrimary() const noexcept { return m_primary; }
  [[nodiscard]] bool activatePrimary();
  [[nodiscard]] bool takePendingActivation() noexcept;

public slots:
  [[nodiscard]] bool Activate();

signals:
  void activationRequested();

private:
  bool m_primary = true;
  bool m_serviceRegistered = false;
  bool m_activationPending = false;
};
