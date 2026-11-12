#pragma once

class QApplication;
class QCommandLineParser;
class QWindow;
class TrayController;
class SingleInstanceController;

namespace GuiSmokeChecks {
// 仅在启用测试的构建中安装 GUI 冒烟检查，不参与正常界面逻辑。
void addOptions(QCommandLineParser &parser);
void prepareEnvironment(const QCommandLineParser &parser);
int install(const QCommandLineParser &parser, QApplication &app,
            QWindow *window, TrayController &trayController,
            SingleInstanceController &singleInstance, bool forceTrayForTests);
} // namespace GuiSmokeChecks
