#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// 通过一组固定的 QML 角色公开 QVariantMap 记录列表。
// `identityRole` 用于判断 replace() 能否原位更新行，从而避免重置模型并
// 使现有委托失效。
class MapListModel : public QAbstractListModel {
  Q_OBJECT
  QML_ANONYMOUS
  Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
  explicit MapListModel(QList<QByteArray> roles, QByteArray identityRole,
                        QObject *parent = nullptr);
  [[nodiscard]] int count() const noexcept { return static_cast<int>(m_items.size()); }
  [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  // 替换数据快照；结构稳定时保留各行的对象标识。
  void replace(const QVariantList &items);
  [[nodiscard]] QVariantMap itemAt(int index) const;

signals:
  void countChanged();

private:
  QHash<int, QByteArray> m_roles;
  QVariantList m_items;
  QByteArray m_identityRole;
};

class SensorListModel : public MapListModel {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit SensorListModel(QObject *parent = nullptr);
};

class FanListModel : public MapListModel {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit FanListModel(QObject *parent = nullptr);
};

class ZoneListModel : public MapListModel {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit ZoneListModel(QObject *parent = nullptr);
};

class BackendListModel : public MapListModel {
  Q_OBJECT
  QML_ANONYMOUS
public:
  explicit BackendListModel(QObject *parent = nullptr);
};

// 持有一条风扇曲线的可编辑副本，并跟踪服务端修订号。
// 最后一个点是强制安全端点，必须保持最大温度和最大占空比，因此以固定点
// 的形式公开。
class CurvePointModel : public QAbstractListModel {
  Q_OBJECT
  QML_ANONYMOUS
  Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged FINAL)
  Q_PROPERTY(int revision READ revision NOTIFY revisionChanged FINAL)
  Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
  Q_PROPERTY(QVariantList points READ toVariantList NOTIFY pointsChanged FINAL)

public:
  enum Roles { TemperatureRole = Qt::UserRole + 1, DutyRole, FixedRole };
  Q_ENUM(Roles)

  explicit CurvePointModel(QObject *parent = nullptr);
  [[nodiscard]] int count() const noexcept { return static_cast<int>(m_points.size()); }
  [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  [[nodiscard]] bool dirty() const noexcept { return m_dirty; }
  [[nodiscard]] int revision() const noexcept { return m_revision; }
  [[nodiscard]] QVariantList toVariantList() const;
  // 加载干净的服务端快照，并清除本地修改状态。
  void load(const QVariantList &points, int revision = 0);
  // 替换一个可编辑点，并将模型标记为已修改。
  Q_INVOKABLE void setPoint(int row, int temperature, int duty);
  Q_INVOKABLE QVariantMap pointAt(int row) const;
  // 将当前值标记为已获服务端确认。
  void markClean();

signals:
  void dirtyChanged();
  void revisionChanged();
  void countChanged();
  void pointsChanged();

private:
  QVariantList m_points;
  bool m_dirty = false;
  int m_revision = 0;
};
