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

double NumberElement::readDoubleFromString(const QString& string)
{
	if (string.isEmpty())
		return 0.0;

	bool isDecimal = false;
	double decimalNumber = string.toDouble(&isDecimal);
	if (isDecimal)
		return decimalNumber;

	//TODO: Implement conversion from sexagesimal
	return 0.0;
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

QString NumberElement::getFormattedValue() const
{
	//TODO: Add the other case
	//TODO: Use the standard function for C-formatted output?
	QString formattedValue;
	formattedValue.sprintf(formatString.toAscii(), value);
	return formattedValue;
}

double NumberElement::getValue() const
{
	return value;
}

void NumberElement::setValue(const QString& stringValue)
{
	//TODO: Checks for min/max/step
	value = readDoubleFromString(stringValue);
}

Property::Property(const QString& propertyName,
				   State propertyState,
				   Permission accessPermission,
				   const QString& propertyLabel,
				   const QString& propertyGroup)
{
	name = propertyName;
	if (propertyLabel.isEmpty())
		label = name;
	else
		label = propertyLabel;

	group = propertyGroup;
	permission = accessPermission;
	state = propertyState;
}

QString Property::getName()
{
	return name;
}

QString Property::getLabel()
{
	return label;
}

bool Property::isReadable()
{
	return (permission == PermissionReadOnly || permission == PermissionReadWrite);
}

bool Property::isWritable()
{
	return (permission == PermissionWriteOnly || permission == PermissionReadWrite);
}

NumberProperty::NumberProperty(const QString& propertyName,
							   State propertyState,
							   Permission accessPermission,
							   const QString& propertyLabel,
							   const QString& propertyGroup) :
	Property(propertyName,
			 propertyState,
			 accessPermission,
			 propertyLabel,
			 propertyGroup)
{
	//
}

NumberProperty::~NumberProperty()
{
	foreach (NumberElement* numberElementPtr, elements)
	{
		delete numberElementPtr;
	}
}

void NumberProperty::addElement(NumberElement* element)
{
	elements.insert(element->getName(), element);
}

void NumberProperty::updateElement(const QString& name, const QString& newValue)
{
	elements[name]->setValue(newValue);
}

NumberElement* NumberProperty::getElement(const QString& name)
{
	return elements.value(name);
}

int NumberProperty::elementCount() const
{
	return elements.count();
}