#pragma once

#include <QString>
#include <QVariantList>

namespace FanCurveConfig {
[[nodiscard]] QString editableConfigPath();
[[nodiscard]] QString backupConfigPath();
[[nodiscard]] bool ensureDefaultBackup(QString *error = nullptr);
[[nodiscard]] QVariantList loadFile(const QString &path,
                                    QString *error = nullptr);
[[nodiscard]] QVariantList loadDefault(QString *error = nullptr);
} // namespace FanCurveConfig
