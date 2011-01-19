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

#ifndef _INDI_HPP_
#define _INDI_HPP_

#include <QByteArray>
#include <QHash>
#include <QString>

//! Possible property permissions according to the INDI protocol specification.
//! Lights are assumed to be always read-only, Switches can be read-only or
//! read-write.
enum Permission {
	PermissionReadOnly,
	PermissionWriteOnly,
	PermissionReadWrite
};

//! Possible property states according to the INDI protocol specification.
enum State {
	StateIdle,
	StateOk,
	StateBusy,
	StateAlert
};

//! Rules governing the behaviour of vectors (arrays) of switches 
enum SwitchRule {
	SwitchOnlyOne,
	SwitchAtMostOne,
	SwitchAny
};

//In all element classes, put the required elements first.

class Element
{
public:
	Element(const QString& elementName, const QString& elementLabel = QString());

	const QString& getName() const;
	const QString& getLabel() const;

private:
	QString name;
	QString label;
};

//! Sub-property representing a single string.
class TextElement : public Element
{
	//! \param elementName is required.
	//! \param elementValue is required.
	//! \param elementLabel is not required. If it is not specified, the value
	//! of Element::name (i.e. #elementName) will be used as a value
	//! for Element::label.
	TextElement(const QString& elementName, const QString& elementValue, const QString& elementLabel = QString());

	const QString& getText() const;

private:
	QString value;
};

//! Sub-property representing a single number.
//! Despite being defined as a class, this is just a data structure.
class NumberElement : public Element
{
public:
	NumberElement(const QString& elementName,
	              const QString& initialValue,
	              const QString& format,
	              const QString& minimalValue,
	              const QString& maximalValue,
	              const QString& step,
	              const QString& label = QString());

	double getValue() const;
	QString getFormattedValue() const;

	void setValue(const QString& stringValue);

private:
	bool isSexagesimal;
	double value;
	double maxValue;
	double minValue;
	double step;
	QString formatString;

	double readDoubleFromString(const QString& string);
};

//! Sub-property representing a single switch/button.
//! Despite being defined as a class, this is just a data structure.
//! \todo Is there a point of this existing, or it can be replaced with a hash
//! of booleans?
class SwitchElement : public Element
{
public:
	SwitchElement(const QString& elementName,
				  const QString& initialValue,
				  const QString& label = QString());

	bool isOn();
	void setValue(const QString& stringValue);

private:
	//! State of the switch. True is "on", false is "off". :)
	bool state;
};

//! Sub-property representing a single indicator light.
//! Despite being defined as a class, this is just a data structure.
class LightElement
{
};

//! Sub-property representing a single BLOB (Binary Large OBject, e.g. a photo).
//! Despite being defined as a class, this is just a data structure.
class BlobElement
{
	QString name;
	QString label;
	QByteArray binaryData;
};

//! Base class of all property classes.
//TODO: Base class or template?
class Property
{
public:
	Property(const QString& propertyName,
			 State propertyState,
			 Permission accessPermission,
			 const QString& propertyLabel = QString(),
			 const QString& propertyGroup = QString());
	QString getName();
	QString getLabel();
	bool isReadable();
	bool isWritable();
	virtual int elementCount() const = 0;

private:
	//! Name used to identify the property internally.
	QString name;
	//! Human-readable label used to represent the property in the GUI.
	//! If not specified, the value of #name is used.
	QString label;
	//! Group name.
	QString group;
	//! Permission limiting client-side actions on this property.
	Permission permission;
	//! Current state of the property
	State state;
	//! Time in seconds before the next expected change in value.
	double timeout;
};

//! A vector/array of numeric values (NumberElement-s).
class NumberProperty : public Property
{
public:
	//! \todo Do I need a device name field?
	//! \todo More stuff may need to be added to the constructor's arguments.
	NumberProperty(const QString& propertyName,
				   State propertyState,
				   Permission accessPermission,
				   const QString& propertyLabel = QString(),
				   const QString& propertyGroup = QString());
	~NumberProperty();

	void addElement(NumberElement* element);
	void updateElement(const QString& name, const QString& newValue);
	NumberElement* getElement(const QString& name);
	int elementCount() const;

private:
	QHash<QString,NumberElement*> elements;
};

//! A vector/array of switches (boolean SwitchElement-s)
class SwitchProperty : public Property
{
public:
	SwitchProperty(const QString& propertyName,
	               State propertyState,
	               Permission accessPermission,
	               SwitchRule switchRule,
	               const QString& propertyLabel = QString(),
	               const QString& propertyGroup = QString());
	~SwitchProperty();

	SwitchRule getSwitchRule() const;

	void addElement(SwitchElement* element);
	//! Ignores the SwitchRule - everything is supposed to be checked on the
	//! device side.
	void updateElement(const QString& name, const QString& newValue);
	SwitchElement* getElement(const QString& name);
	int elementCount() const;

private:
	SwitchRule rule;
	QHash<QString,SwitchElement*> elements;
};

#endif//_INDI_HPP_
