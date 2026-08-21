#pragma once

#include <QString>
#include <QVariantList>

namespace FanCurveConfig {
// 返回管理员可编辑的系统级默认曲线路径。
[[nodiscard]] QString editableConfigPath();
// 返回软件包保留的只读默认曲线备份路径。
[[nodiscard]] QString backupConfigPath();
// 在缺少备份时创建默认曲线备份，且不覆盖已有文件。
[[nodiscard]] bool ensureDefaultBackup(QString *error = nullptr);
// 从 `path` 解析曲线；格式或约束错误时返回空列表并填写 `error`。
[[nodiscard]] QVariantList loadFile(const QString &path,
                                    QString *error = nullptr);
// 优先加载管理员配置，不可用时回退到软件包默认曲线。
[[nodiscard]] QVariantList loadDefault(QString *error = nullptr);
} // namespace FanCurveConfig
