// SPDX-License-Identifier: GPL-2.0
#ifndef MAPLOCATIONMODEL_H
#define MAPLOCATIONMODEL_H

#include <QObject>
#include <QVector>
#include <QHash>
#include <QByteArray>
#include <QAbstractListModel>
#include <QGeoCoordinate>

class MapLocation : public QObject
{
	Q_OBJECT
	Q_PROPERTY(quint32 uuid MEMBER m_uuid)
	Q_PROPERTY(QGeoCoordinate coordinate MEMBER m_coordinate)

public:
	static const char *PROPERTY_NAME_COORDINATE;
	static const char *PROPERTY_NAME_UUID;

	explicit MapLocation();
	explicit MapLocation(quint32 uuid, QGeoCoordinate coord);

	QVariant getRole(int role) const;

	enum Roles {
		RoleUuid = Qt::UserRole + 1,
		RoleCoordinate
	};

private:
	quint32 m_uuid;
	QGeoCoordinate m_coordinate;
};

class MapLocationModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
	MapLocationModel(QObject *parent = NULL);
	~MapLocationModel();

	Q_INVOKABLE MapLocation *get(int row);

	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;
	int count();
	void add(MapLocation *);
	void addList(QVector<MapLocation *>);
	void clear();

protected:
	QHash<int, QByteArray> roleNames() const;

private:
	QVector<MapLocation *> m_mapLocations;
	QHash<int, QByteArray> m_roles;

signals:
	void countChanged(int c);

};

#endif