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

#ifndef _INDI_CLIENT_HPP_
#define _INDI_CLIENT_HPP_

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QXmlStreamReader>

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
class Switch
{
};

//! Sub-property representing a single indicator light.
//! Despite being defined as a class, this is just a data structure.
class Light
{
};

//! Sub-property representing a single BLOB (Binary Large OBject, e.g. a photo).
//! Despite being defined as a class, this is just a data structure.
class Blob
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

//! Class implementing a client for the INDI wire protocol.
//! Properties are stored internally. Qt signals are emitted when a property
//! is changed or defined.
class IndiClient : public QObject
{
	Q_OBJECT

public:
	IndiClient(QObject* parent = 0);
	~IndiClient();

	//! Error handling on the device level should be done by the caller.
	//! IndiClient only handles the command streams.
	void addConnection(QIODevice* ioDevice);

	//INDI XML tags
	static const char* T_DEF_NUMBER_VECTOR;
	static const char* T_SET_NUMBER_VECTOR;
	static const char* T_DEF_NUMBER;
	static const char* T_ONE_NUMBER;

	//INDI XML attributes
	static const char* A_DEVICE;
	static const char* A_NAME;
	static const char* A_LABEL;
	static const char* A_GROUP;
	static const char* A_STATE;
	static const char* A_PERMISSION;
	static const char* A_TIMEOUT;
	static const char* A_TIMESTAMP;
	static const char* A_MESSAGE;
	static const char* A_FORMAT;
	static const char* A_MINIMUM;
	static const char* A_MAXIMUM;
	static const char* A_STEP;

	//INDI standard properties
	static const char* SP_JNOW_COORD;

public slots:

signals:
	void propertyDefined(QString deviceName, Property* property);
	void propertyUpdated(QString deviceName, Property* property);

private slots:
	void handleIncomingCommands();

private:
	//!
	Permission readPermissionFromString(const QString& string);
	//!
	State readStateFromString(const QString& string);

	//!
	void readNumberPropertyDefinition();
	//!
	void readNumberElementDefinition(NumberProperty* numberProperty);
	//!
	void readNumberProperty();
	//!
	void readNumberElement(NumberProperty* numberProperty);

	//! May be a QProcess or a QTcpSocket.
	QIODevice* ioDevice;

	QTextStream* textStream;

	//! \todo Better name...
	QXmlStreamReader xmlReader;

	//! Temporary number properties only.
	//! \todo Must support named devices and all kinds of properties
	QHash<QString,NumberProperty*> numberProperties;
};

#endif //_INDI_CLIENT_HPP_
