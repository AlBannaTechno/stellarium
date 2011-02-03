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
#include <QDateTime>
#include <QHash>
#include <QString>

#include <limits>
const double DOUBLE_MAX = std::numeric_limits<double>::max();
const double DOUBLE_MIN = std::numeric_limits<double>::min();

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
public:
	TextElement(const QString& elementName,
	            const QString& initialValue,
	            const QString& label = QString());

	QString getValue() const;
	void setValue(const QString& stringValue);

private:
	QString value;
};

//! Sub-property representing a single number.
class NumberElement : public Element
{
public:
	NumberElement(const QString& elementName,
	              const double initialValue = 0.0,
	              const QString& format = "%d",
	              const double minimumValue = DOUBLE_MIN,
	              const double maximumValue = DOUBLE_MAX,
	              const double step = 0,
	              const QString& label = QString());
	
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

	static double readDoubleFromString(const QString& string);

private:
	// For unit tests
	friend class TestNumberElement;
	
	bool isSexagesimal;
	double value;
	double maxValue;
	double minValue;
	double step;
	QString formatString;
};

//! Sub-property representing a single switch/button.
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
class LightElement : public Element
{
public:
	LightElement(const QString& elementName,
	             const QString& initialValue,
	             const QString& label = QString());

	State getValue() const;
	//! \todo Decide what to do with the duplication with
	//! IndiClient::readStateFromString().
	//! \todo Decide how to handle wrong values. (At the moment if the parameter
	//! is not recognised it ignores it).
	void setValue(const QString& stringValue);

private:
	State state;
};

//! Sub-property representing a single BLOB (Binary Large OBject).
//! \todo Think what to make of this - it may be massive.
//! \todo Is there a point of it existing? It is not handled as the rest of the
//! elements - it has no initial value.
class BlobElement : public Element
{
	BlobElement(const QString& elementName,
	            const QString& label = QString());

	//! Decodes the string to a QByteArray?
	//! And saves it to disk? Emits a signal to the previewer?
	void setValue(const QString& blobLength,
	              const QString& blobFormat,
	              const QString& blobData);

	//! Example: Returns the decoded data?
	QByteArray getValue() const;

	//! Example
	QString getFormat() const;
	//! Example
	//! \todo Isn't int too small?
	int getSize() const;

private:
	QByteArray binaryData;
	QString format;
};

//! Base class of all property classes.
//TODO: Base class or template?
class Property
{
public:
	enum PropertyType {
		TextProperty,
		NumberProperty,
		SwitchProperty,
		LightProperty,
		BlobProperty
	};

	Property(const QString& propertyName,
	         State propertyState,
	         Permission accessPermission,
	         const QString& propertyLabel = QString(),
	         const QString& propertyGroup = QString(),
	         const QDateTime& timestamp = QDateTime());
	virtual ~Property();
	PropertyType getType() const;
	QString getName();
	QString getLabel();
	QString getGroup();
	bool isReadable();
	bool isWritable();
	Permission getPermission() const;
	void setState(State newState);
	State getCurrentState() const;
	QDateTime getTimestamp() const;
	qint64 getTimestampInMilliseconds() const;
	virtual int elementCount() const = 0;
	virtual QStringList getElementNames() const = 0;

protected:
	//! Sets the timestamp value. If necessary, reinterprets the data as UTC.
	//! If the argument is not a valid QDateTime, uses the current moment.
	void setTimestamp(const QDateTime& timestamp);
	//! Property type
	PropertyType type;

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
	//! Time of the last change (UTC)
	QDateTime timestamp;
};

//! A vector/array of string values (TextElement-s).
class TextProperty : public Property
{
public:
	TextProperty(const QString& propertyName,
	             State propertyState,
	             Permission accessPermission,
	             const QString& propertyLabel = QString(),
	             const QString& propertyGroup = QString(),
	             const QDateTime& timestamp = QDateTime());
	~TextProperty();

	//TODO
	TextElement* getElement(const QString& name);

	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,TextElement*> elements;
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
	               const QString& propertyGroup = QString(),
	               const QDateTime& timestamp = QDateTime());
	virtual ~NumberProperty();

	void addElement(NumberElement* element);
	NumberElement* getElement(const QString& name);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp,
	            State newState);
	int elementCount() const;
	QStringList getElementNames() const;

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
	               const QString& propertyGroup = QString(),
	               const QDateTime& timestamp = QDateTime());
	virtual ~SwitchProperty();

	SwitchRule getSwitchRule() const;

	void addElement(SwitchElement* element);
	//! Ignores the SwitchRule - everything is supposed to be checked on the
	//! device side.
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp);
	void update(const QHash<QString,QString>& newValues,
	            const QDateTime& timestamp,
	            State newState);
	SwitchElement* getElement(const QString& name);
	int elementCount() const;
	QStringList getElementNames() const;

private:
	SwitchRule rule;
	QHash<QString,SwitchElement*> elements;
};

//! A vector/array of lights (LightElement-s).
class LightProperty : public Property
{
public:
	//! Lights are always read-only
	LightProperty(const QString& propertyName,
	              State propertyState,
	              const QString& propertyLabel = QString(),
	              const QString& propertyGroup = QString(),
	              const QDateTime& timestamp = QDateTime());
	~LightProperty();

	LightElement* getElement(const QString& name);
	//TODO:

	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,LightElement*> elements;
};

//! A vector/array of BLOBs (BlobElement-s).
class BlobProperty : public Property
{
public:
	BlobProperty(const QString& propertyName,
	             State propertyState,
	             Permission accessPermission,
	             const QString& propertyLabel = QString(),
	             const QString& propertyGroup = QString(),
	             const QDateTime& timestamp = QDateTime());
	~BlobProperty();

	//TODO

	int elementCount() const;
	QStringList getElementNames() const;

private:
	QHash<QString,BlobElement*> elements;
};

#endif//_INDI_HPP_
