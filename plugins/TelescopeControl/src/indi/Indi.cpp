/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010 Bogdan Marinov
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

#include "Indi.hpp"

#include <cmath>
#include <QStringList>

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Element Methods
#endif
/* ********************************************************************* */
Element::Element(const QString &elementName, const QString &elementLabel)
{
	name = elementName;
	if (elementLabel.isEmpty())
		label = name;
	else
		label = elementLabel;
}

const QString& Element::getName() const
{
	return name;
}

const QString& Element::getLabel() const
{
	return label;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark NumberElement Methods
#endif
/* ********************************************************************* */
NumberElement::NumberElement(const QString& elementName,
									  const double initialValue,
									  const QString& format,
									  const double minimumValue,
									  const double maximumValue,
									  const double step,
									  const QString& label) :
Element(elementName, label),
value(initialValue),
maxValue(maximumValue),
minValue(minimumValue),
step(step)
{
	//TODO: Validation and other stuff
	formatString = format;
}

NumberElement::NumberElement(const QString& elementName,
                             const QString& initialValue,
                             const QString& format,
                             const QString& minimalValue,
                             const QString& maximalValue,
                             const QString& incrementStep,
                             const QString& elementLabel) :
Element(elementName, elementLabel)
{
	//TODO: Validation and other stuff
	formatString = format;
	
	minValue = readDoubleFromString(minimalValue);
	maxValue = readDoubleFromString(maximalValue);
	step = readDoubleFromString(incrementStep);
	value = readDoubleFromString(initialValue);
}

double NumberElement::readDoubleFromString(const QString& string)
{
	if (string.isEmpty())
		return 0.0;

	bool isDecimal = false;
	double decimalNumber = string.toDouble(&isDecimal);
	if (isDecimal)
		return decimalNumber;

	double degrees = 0.0, minutes = 0.0, seconds = 0.0;
	QRegExp separator("[\\ \\: \\;]");
	QStringList components = string.split(separator);
	if (components.count() < 2 || components.count() > 3)
		return 0.0;

	degrees = components.at(0).toDouble();
	minutes = components.at(1).toDouble();
	if (components.count() == 3)
		seconds = components.at(2).toDouble();

	//TODO: Smarter way of calculating a degree fraction.
	//TODO: And dealing with the sign.
	double sign = 1.0;
	if (degrees < 0)
	{
		sign = -1.0;
		degrees = -degrees;
	}
	degrees += (minutes/60.0);
	degrees += (seconds/3600.0);
	degrees *= sign;

	return degrees;
}

QString NumberElement::getFormattedValue() const
{
	//TODO: Optimize
	QString formattedValue;
	QRegExp indiFormat("^\\%(\\d+)\\.(\\d)\\m$");
	if (indiFormat.exactMatch(formatString))
	{
		int width = indiFormat.cap(1).toInt();
		int precision = indiFormat.cap(2).toInt();
		if (width < 1)
			return formattedValue;

		double seconds = value * 3600.0;
		int degrees = seconds / 3600;
		seconds = fabs(seconds);
		seconds = fmod(seconds, 3600);
		double minutes = seconds / 60.0;
		seconds = fmod(seconds, 60);

		switch (precision)
		{
		case 3:
			formattedValue = QString("%1:%2").arg(degrees).arg(minutes, 2, 'f', 0, '0');
			break;
		case 5:
			formattedValue = QString("%1:%2").arg(degrees).arg(minutes, 4, 'f', 1, '0');
			break;
		case 6:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 2, 'f', 0, '0');
			break;
		case 8:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 4, 'f', 1, '0');
			break;
		case 9:
		default:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 5, 'f', 2, '0');
		}
		formattedValue = formattedValue.rightJustified(width, ' ', true);
	}
	else
	{
		//TODO: Use the standard function for C-formatted output?
		formattedValue.sprintf(formatString.toAscii(), value);
	}
	return formattedValue;
}

double NumberElement::getValue() const
{
	return value;
}

void NumberElement::setValue(const QString& stringValue)
{
	double newValue = readDoubleFromString(stringValue);
	if (newValue < minValue)
		return;

	//If maxValue == minValue, it should be ignored per the INDI specification.
	if (maxValue > minValue && newValue > maxValue)
		return;

	//If step is 0, it should be ignored per the INDI specification.
	//TODO: Doesn't work very well for non-integers.
	//Alternative: (fmod((newValue-minValue), step) - step) > 0.001,
	//but that doesn't work very well, too.
	if (step > 0.0)
	{
		double remainder = fmod((newValue - minValue), step);
		if (qFuzzyCompare(remainder+1, 0.0+1))
			return;
	}

	value = newValue;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark SwitchElement Methods
#endif
/* ********************************************************************* */
SwitchElement::SwitchElement(const QString& elementName,
                             const QString& initialValue,
                             const QString& label) :
	Element(elementName, label)
{
	state = false;
	setValue(initialValue);
}

bool SwitchElement::isOn()
{
	return state;
}

void SwitchElement::setValue(const QString& string)
{
	if (string == "On")
		state = true;
	else if (string == "Off")
		state = false;
	else
		return;
	//TODO: Output?
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
