#pragma once

#include <QObject>

// 持有桌面会话级 D-Bus 名称，用于保证 GUI 单实例运行。
// 后续启动的进程调用主实例的 Activate() 后即退出。
class SingleInstanceController final : public QObject {
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.linuxthermal.Control.Gui1")

public:
  explicit SingleInstanceController(QObject *parent = nullptr);
  ~SingleInstanceController() override;

  [[nodiscard]] bool isPrimary() const noexcept { return m_primary; }
  // 请求已注册的主实例显示窗口。
  [[nodiscard]] bool activatePrimary();
  // 取走在窗口完成连接前到达的激活请求。
  [[nodiscard]] bool takePendingActivation() noexcept;

public slots:
  // 记录或发出显示主窗口的请求。
  [[nodiscard]] bool Activate();

signals:
  void activationRequested();

private:
  bool m_primary = true;
  bool m_serviceRegistered = false;
  bool m_activationPending = false;
};
