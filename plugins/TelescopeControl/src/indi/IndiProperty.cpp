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
#include <QDebug>


const char* TagAttributes::VERSION = "version";
const char* TagAttributes::DEVICE = "device";
const char* TagAttributes::NAME = "name";
const char* TagAttributes::LABEL = "label";
const char* TagAttributes::GROUP = "group";
const char* TagAttributes::STATE = "state";
const char* TagAttributes::PERMISSION = "perm";
const char* TagAttributes::TIMEOUT = "timeout";
const char* TagAttributes::TIMESTAMP = "timestamp";
const char* TagAttributes::MESSAGE = "message";
const char* TagAttributes::RULE = "rule";

TagAttributes::TagAttributes(const QXmlStreamReader& xmlReader) : 
    areValid(false)
{
	attributes = xmlReader.attributes();
	
	// Required attributes
	device = attributes.value(DEVICE).toString();
	name = attributes.value(NAME).toString();
	if (device.isEmpty() || name.isEmpty())
	{
		qDebug() << "A required attribute is missing (device, name):"
		         << device << name;
		return;
	}
	
	areValid = true;
	
	timeoutString = attributes.value(TIMEOUT).toString();
	timestamp = readTimestampAttribute(attributes);
	message = attributes.value(MESSAGE).toString();
}

QDateTime TagAttributes::readTimestampAttribute(const QXmlStreamAttributes& attributes)
{
	QString timestampString = attributes.value(TIMESTAMP).toString();
	QDateTime timestamp;
	if (!timestampString.isEmpty())
	{
		timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
		timestamp.setTimeSpec(Qt::UTC);
	}
	return timestamp;
}

BasicDefTagAttributes::BasicDefTagAttributes
(const QXmlStreamReader& xmlReader) : 
    TagAttributes(xmlReader)
{
	if (!areValid)
		return;
	
	QString stateString = attributes.value(STATE).toString();
	if (stateString == "Idle")
		state = StateIdle;
	else if (stateString == "Ok")
		state = StateOk;
	else if (stateString == "Busy")
		state = StateBusy;
	else if (stateString == "Alert")
		state = StateAlert;
	else
	{
		qDebug() << "Invalid value for required state attribute:"
		         << stateString;
		areValid = false;
		return;
	}
	
	// Other common attributes
	label = attributes.value(LABEL).toString();
	group = attributes.value(GROUP).toString();
}

StandardDefTagAttributes::StandardDefTagAttributes(const QXmlStreamReader& xmlReader) :
    BasicDefTagAttributes(xmlReader)
{
	// The basic validity should have been evaluated by now
	if (!areValid)
		return;
	
	QString permissionString = attributes.value(PERMISSION).toString();
	if (permissionString == "rw")
		permission = PermissionReadWrite;
	else if (permissionString == "wo")
		permission = PermissionWriteOnly;
	else if (permissionString == "ro")
		permission = PermissionReadOnly;
	else
	{
		qDebug() << "Invalid value for required permission attribute:"
		         << permissionString;
		areValid = false;
	}
}

DefSwitchTagAttributes::DefSwitchTagAttributes(const QXmlStreamReader& xmlReader) :
    StandardDefTagAttributes(xmlReader)
{
	// The basic validity should have been evaluated by now
	if (!areValid)
		return;
	
	QString ruleString  = attributes.value(RULE).toString();
	if (ruleString == "OneOfMany")
		rule = SwitchOnlyOne;
	else if (ruleString == "AtMostOne")
		rule = SwitchAtMostOne;
	else if (ruleString == "AnyOfMany")
		rule = SwitchAny;
	else
	{
		qDebug() << "Invalid value for rule attribute:"
		         << ruleString;
		areValid = false;
	}
}


SetTagAttributes::SetTagAttributes(const QXmlStreamReader& xmlReader) :
    TagAttributes(xmlReader),
    stateChanged(true)
{
	if (!areValid)
		return;
	
	QString stateString = attributes.value(STATE).toString();
	if (stateString == "Idle")
		state = StateIdle;
	else if (stateString == "Ok")
		state = StateOk;
	else if (stateString == "Busy")
		state = StateBusy;
	else if (stateString == "Alert")
		state = StateAlert;
	else
		stateChanged = false;
}

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
				   const QDateTime& firstTimestamp)
{
	name = propertyName;
	if (propertyLabel.isEmpty())
		label = name;
	else
		label = propertyLabel;

	group = propertyGroup;
	permission = accessPermission;
	state = propertyState;
	setTimestamp(firstTimestamp);
}

Property::Property(const BasicDefTagAttributes& attributes)
{
	name = attributes.name;
	if (attributes.label.isEmpty())
		label = name;
	else
		label = attributes.label;
	
	group = attributes.group;
	permission = PermissionReadOnly;
	state = attributes.state;
	setTimestamp(attributes.timestamp);
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

TextProperty::TextProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
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

NumberProperty::NumberProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
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

SwitchProperty::SwitchProperty(const DefSwitchTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
	rule = attributes.rule;
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

LightProperty::LightProperty(const BasicDefTagAttributes& attributes) :
    Property(attributes)
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

BlobProperty::BlobProperty(const StandardDefTagAttributes& attributes) :
    Property(attributes)
{
	permission = attributes.permission;
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
