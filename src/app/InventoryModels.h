#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

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
  void load(const QVariantList &points, int revision = 0);
  Q_INVOKABLE void setPoint(int row, int temperature, int duty);
  Q_INVOKABLE QVariantMap pointAt(int row) const;
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
