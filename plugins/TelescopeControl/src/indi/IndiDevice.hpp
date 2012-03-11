/* 
Qt-based INDI protocol client
Copyright (C) 2012  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef INDIDEVICE_HPP
#define INDIDEVICE_HPP

#include <QObject>
#include <QSharedPointer>

#include <IndiProperty.hpp>

//! Container for the Property objects of a single device.
//! \todo Handle BLOB policy.
class Device : public QObject
{
	Q_OBJECT
public:
	Device(const QString& newDeviceName, QObject *parent = 0);
	~Device();
	
	QString getName() const;
	bool addProperty(const PropertyP& property);
	void removeProperty(const QString& propertyName);
	void removeAllProperties();
	PropertyP getProperty(const QString& propertyName);
	bool hasProperty(const QString& propertyName) const;
	
signals:
	//! Emitted when a new property has been defined for this device.
	//! (I.e. when a \b def[type]Vector element has been parsed by IndiClient.)
	void propertyDefined(const PropertyP& property);
	//! Emitted when a property has been removed from this device.
	//! (I.e. when a \b delProperty element has been parsed by IndiClient.)
	void propertyRemoved(const QString& propertyName);
	
public slots:
	
private:
	QString name;
	
	QHash<QString,PropertyP> properties;
};

typedef QSharedPointer<Device> DeviceP;

#endif // INDIDEVICE_HPP
