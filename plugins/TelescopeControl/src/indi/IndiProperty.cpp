/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IndiProperty.hpp"

#include <QDir>
#include <QStringList>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Property Methods
#endif
/* ********************************************************************* */
Property::Property(const QString& propertyName,
				   State propertyState,
				   Permission accessPermission,
				   const QString& propertyLabel,
				   const QString& propertyGroup,
				   const QDateTime& _timestamp)
{
	name = propertyName;
	if (propertyLabel.isEmpty())
		label = name;
	else
		label = propertyLabel;

	group = propertyGroup;
	permission = accessPermission;
	state = propertyState;
	setTimestamp(_timestamp);
}

Property::~Property()
{
}

Property::PropertyType Property::getType() const
{
	return type;
}

QString Property::getName()
{
	return name;
}

QString Property::getLabel()
{
	return label;
}

QString Property::getGroup()
{
	return group;
}

bool Property::isReadable()
{
	return (permission == PermissionReadOnly || permission == PermissionReadWrite);
}

bool Property::isWritable()
{
	return (permission == PermissionWriteOnly || permission == PermissionReadWrite);
}

Permission Property::getPermission() const
{
	return permission;
}

void Property::setState(State newState)
{
	state = newState;
}

State Property::getCurrentState() const
{
	return state;
}

QDateTime Property::getTimestamp() const
{
	return timestamp;
}

qint64 Property::getTimestampInMilliseconds() const
{
	return timestamp.toMSecsSinceEpoch();
}

void Property::setTimestamp(const QDateTime& newTimestamp)
{
	if (newTimestamp.isValid())
	{
		timestamp = newTimestamp;
		//TODO: Re-interpret or convert?
		if(newTimestamp.timeSpec() != Qt::UTC)
			timestamp.setTimeSpec(Qt::UTC);
	}
	else
	{
		timestamp = QDateTime::currentDateTimeUtc();
	}
}


TextProperty::TextProperty(const QString& propertyName,
                           State propertyState,
                           Permission accessPermission,
                           const QString& propertyLabel,
                           const QString& propertyGroup,
                           const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::TextProperty;
}

TextProperty::~TextProperty()
{
	qDeleteAll(elements);
}

void TextProperty::addElement(TextElement* element)
{
	elements.insert(element->getName(), element);
}

TextElement* TextProperty::getElement(const QString& name)
{
	return elements.value(name);
}

int TextProperty::elementCount() const
{
	return elements.count();
}

QStringList TextProperty::getElementNames() const
{
	return elements.keys();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark NumberProperty Methods
#endif
/* ********************************************************************* */
NumberProperty::NumberProperty(const QString& propertyName,
                               State propertyState,
                               Permission accessPermission,
                               const QString& propertyLabel,
                               const QString& propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::NumberProperty;
}

NumberProperty::~NumberProperty()
{
	qDeleteAll(elements);
}

void NumberProperty::addElement(NumberElement* element)
{
	elements.insert(element->getName(), element);
}

void NumberProperty::update(const QHash<QString, QString>& newValues,
                            const QDateTime& newTimestamp)
{
	QHashIterator<QString, QString> it(newValues);
	while(it.hasNext())
	{
		it.next();
		if (elements.contains(it.key()))
			elements[it.key()]->setValue(it.value());
	}
	setTimestamp(newTimestamp);
}

void NumberProperty::update(const QHash<QString, QString>& newValues,
                            const QDateTime &timestamp,
                            State newState)
{
	setState(newState);
	update(newValues, timestamp);
}

NumberElement* NumberProperty::getElement(const QString& name)
{
	return elements.value(name);
}

int NumberProperty::elementCount() const
{
	return elements.count();
}

QStringList NumberProperty::getElementNames() const
{
	return elements.keys();
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark SwitchProperty Methods
#endif
/* ********************************************************************* */
SwitchProperty::SwitchProperty(const QString &propertyName,
                               State propertyState,
                               Permission accessPermission,
                               SwitchRule switchRule,
                               const QString &propertyLabel,
                               const QString &propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp),
	rule(switchRule)
{
	type = Property::SwitchProperty;
}

SwitchProperty::~SwitchProperty()
{
	qDeleteAll(elements);
}

SwitchRule SwitchProperty::getSwitchRule() const
{
	return rule;
}

void SwitchProperty::addElement(SwitchElement* element)
{
	elements.insert(element->getName(), element);
}

void SwitchProperty::update(const QHash<QString, QString>& newValues,
                            const QDateTime& newTimestamp)
{
	QHashIterator<QString, QString> it(newValues);
	while(it.hasNext())
	{
		it.next();
		if (elements.contains(it.key()))
			elements[it.key()]->setValue(it.value());
	}
	setTimestamp(newTimestamp);
}

void SwitchProperty::update(const QHash<QString, QString>& newValues,
                            const QDateTime& newTimestamp,
                            State newState)
{
	setState(newState);
	update(newValues, newTimestamp);
}

SwitchElement* SwitchProperty::getElement(const QString& name)
{
	return elements.value(name);
}

int SwitchProperty::elementCount() const
{
	return elements.count();
}

QStringList SwitchProperty::getElementNames() const
{
	return elements.keys();
}


LightProperty::LightProperty(const QString& propertyName,
                               State propertyState,
                               const QString& propertyLabel,
                               const QString& propertyGroup,
                               const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         PermissionReadOnly,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::LightProperty;
}

LightProperty::~LightProperty()
{
	qDeleteAll(elements);
}

void LightProperty::addElement(LightElement* element)
{
	elements.insert(element->getName(), element);
}

LightElement* LightProperty::getElement(const QString& name)
{
	return elements.value(name);
}

int LightProperty::elementCount() const
{
	return elements.count();
}

QStringList LightProperty::getElementNames() const
{
	return elements.keys();
}


BlobProperty::BlobProperty(const QString& propertyName,
                           State propertyState,
                           Permission accessPermission,
                           const QString& propertyLabel,
                           const QString& propertyGroup,
                           const QDateTime& timestamp) :
	Property(propertyName,
	         propertyState,
	         accessPermission,
	         propertyLabel,
	         propertyGroup,
	         timestamp)
{
	type = Property::BlobProperty;
}

BlobProperty::~BlobProperty()
{
	qDeleteAll(elements);
}

int BlobProperty::elementCount() const
{
	return elements.count();
}

QStringList BlobProperty::getElementNames() const
{
	return elements.keys();
}

void BlobProperty::addElement(BlobElement* element)
{
	elements.insert(element->getName(), element);
}

BlobElement* BlobProperty::getElement(const QString& name)
{
	return elements[name];
}

void BlobProperty::update(const QDateTime& newTimestamp)
{
	setTimestamp(newTimestamp);
}

void BlobProperty::update(const QDateTime& newTimestamp, State newState)
{
	setState(newState);
	setTimestamp(newTimestamp);
}
