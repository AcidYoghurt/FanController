#pragma once

#include <QIcon>
#include <QObject>

class QAction;
class QApplication;
class QEvent;
class QMenu;
class QSystemTrayIcon;
class QWindow;

class TrayController final : public QObject {
  Q_OBJECT

public:
  explicit TrayController(QApplication *application, QIcon icon,
                          bool forceAvailableForTests = false,
                          QObject *parent = nullptr);
  ~TrayController() override;

  void setWindow(QWindow *window);
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

  QApplication *m_application;
  QWindow *m_window = nullptr;
  QSystemTrayIcon *m_trayIcon;
  QMenu *m_menu;
  QAction *m_showAction;
  QAction *m_hideAction;
  QAction *m_quitAction;
  bool m_trayAvailable = false;
  bool m_quitRequested = false;
};
