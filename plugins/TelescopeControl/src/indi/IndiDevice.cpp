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

#include "IndiDevice.hpp"

Device::Device(const QString& newDeviceName, QObject *parent) :
    QObject(parent),
    name(newDeviceName)
{
}

Device::~Device()
{
	properties.clear();
}

QString Device::getName() const
{
	return name;
}

bool Device::addProperty(const PropertyP& property)
{
	QString propertyName = property->getName();
	if (property.isNull() || properties.contains(propertyName))
	{
		// TODO: Debug?
		return false;
	}
	properties.insert(propertyName, property);
	emit propertyDefined(property);
	return true;
}

void Device::removeProperty(const QString& propertyName)
{
	if (properties.remove(propertyName) > 0)
	{
		emit propertyRemoved(propertyName);
	}
}

void Device::removeAllProperties()
{
	if (properties.isEmpty())
		return;
	
	QHash<QString,PropertyP>::iterator i = properties.begin();
	while (i != properties.end())
	{
		emit propertyRemoved(i.value()->getName());
		i.value().clear();
		properties.erase(i);
	}
}

PropertyP Device::getProperty(const QString& propertyName)
{
	return properties.value(propertyName, PropertyP());
}

bool Device::hasProperty(const QString& propertyName) const
{
	return properties.contains(propertyName);
}
