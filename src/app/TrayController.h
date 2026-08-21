#pragma once

#include <QIcon>
#include <QObject>
#include <functional>

class QAction;
class QApplication;
class QEvent;
class QMenu;
class QSystemTrayIcon;
class QWindow;

// 协调窗口可见性与系统托盘菜单。托盘可用时，关闭窗口只会隐藏窗口；
// 只有 quitApplication() 会终止 GUI。
class TrayController final : public QObject {
  Q_OBJECT

public:
  explicit TrayController(QApplication *application, QIcon icon,
                          bool forceAvailableForTests = false,
                          QObject *parent = nullptr);
  ~TrayController() override;

  void setWindow(QWindow *window);
  // 设置托盘退出前必须成功执行的同步安全操作。
  // 回调返回 false 时取消退出，并重新启用“退出”操作。
  void setQuitPreparation(std::function<bool()> preparation);
  [[nodiscard]] bool trayAvailable() const noexcept { return m_trayAvailable; }
  [[nodiscard]] bool quitRequested() const noexcept { return m_quitRequested; }

public slots:
  void showWindow();
  void hideWindow();
  void quitApplication();

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void updateActions();

  QApplication *m_application;  // 不持有；应用对象的生命周期更长。
  QWindow *m_window = nullptr;
  QSystemTrayIcon *m_trayIcon;
  QMenu *m_menu;  // QSystemTrayIcon 不接管菜单，由析构函数释放。
  QAction *m_showAction;
  QAction *m_hideAction;
  QAction *m_quitAction;
  bool m_trayAvailable = false;
  bool m_quitRequested = false;
  std::function<bool()> m_quitPreparation;
};
