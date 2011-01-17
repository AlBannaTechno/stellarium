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

#include "IndiClient.hpp"
#include <QDebug>
#include <QRegExp>

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


const char* IndiClient::T_DEF_NUMBER_VECTOR = "defNumberVector";
const char* IndiClient::T_SET_NUMBER_VECTOR = "setNumberVector";
const char* IndiClient::T_DEF_NUMBER = "defNumber";
const char* IndiClient::T_ONE_NUMBER = "oneNumber";

const char* IndiClient::A_DEVICE = "device";
const char* IndiClient::A_NAME = "name";
const char* IndiClient::A_LABEL = "label";
const char* IndiClient::A_GROUP = "group";
const char* IndiClient::A_STATE = "state";
const char* IndiClient::A_PERMISSION = "perm";
const char* IndiClient::A_TIMEOUT = "timeout";
const char* IndiClient::A_TIMESTAMP = "timestamp";
const char* IndiClient::A_MESSAGE = "message";
const char* IndiClient::A_FORMAT = "format";
const char* IndiClient::A_MINIMUM = "min";
const char* IndiClient::A_MAXIMUM = "max";
const char* IndiClient::A_STEP = "step";

const char* IndiClient::SP_CONNECTION = "CONNECTION";
const char* IndiClient::SP_J2000_COORDINATES = "EQUATORIAL_COORD";
const char* IndiClient::SP_JNOW_COORDINATES = "EQUATORIAL_EOD_COORD";
const char* IndiClient::SP_J2000_COORDINATES_REQUEST = "EQUATORIAL_COORD_REQUEST";
const char* IndiClient::SP_JNOW_COORDINATES_REQUEST = "EQUATORIAL_EOD_COORD_REQUEST";

IndiClient::IndiClient(QObject* parent)
	: QObject(parent),
	ioDevice(0),
	textStream(0)
{
	//Make the parser think it's parsing parts of a large document
	//(otherwise it thinks that the first tag in the message is the root one)
	//"Extra content at end of document."
	//TODO: Think of a better way?
	xmlReader.addData("<indi>");
}

IndiClient::~IndiClient()
{

	if (ioDevice)
	{
		disconnect(ioDevice, SIGNAL(readyRead()),
		           this, SLOT(handleIncomingCommands()));
	}

	if (textStream)
	{
		delete textStream;
	}
}

void IndiClient::addConnection(QIODevice* newIoDevice)
{
	if (newIoDevice == 0 ||
		!newIoDevice->isOpen() ||
		!newIoDevice->isReadable() ||
		!newIoDevice->isWritable())
		return;

	//TODO: For now, only one device stream is supported.
	if (ioDevice || textStream)
		return; //A device is already defined?

	ioDevice = newIoDevice;
	textStream = new QTextStream(ioDevice);

	connect(ioDevice, SIGNAL(readyRead()),
	        this, SLOT(handleIncomingCommands()));

	//TODO: Temporarily here. Find a better way!
	sendRawCommand("<getProperties version='1.7' />\n");
}

void IndiClient::sendRawCommand(const QString& command)
{
	if (textStream == 0)
		return;

	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isWritable())
		return;

	QTextStream outgoing(ioDevice);
	outgoing << command;
}

void IndiClient::handleIncomingCommands()
{
	if (textStream == 0)
		return;

	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isReadable() ||
		!ioDevice->isWritable())
		return;

	//Get rid of "XML declaration not at start of document." errors
	//(Damn INDI and badly formed code!)
	//TODO: Hack! Think of a better way!
	QString buffer = textStream->readAll();
	const QRegExp xmlDeclaration("<\\?[^>]+>");
	buffer.remove(xmlDeclaration);
	xmlReader.addData(buffer);

	while (!xmlReader.atEnd())
	{
		xmlReader.readNext();
		//TODO: Ugly. Must be rewritten.
		if (xmlReader.tokenType() == QXmlStreamReader::Invalid)
		{
			QXmlStreamReader::Error errorCode = xmlReader.error();
			if (errorCode == QXmlStreamReader::PrematureEndOfDocumentError)
			{
				//Happens when the end of the current "transmission" has been
				//reached.
				//TODO: Better way of handling this.
				//qDebug() << "Command segment read.";
				break;
			}
			else
			{
				qDebug() << errorCode << xmlReader.errorString();
				xmlReader.clear();//Is this necessary?
				break;
			}
		}
		else if (xmlReader.tokenType() == QXmlStreamReader::NoToken)
		{
			break;
		}
		else if (xmlReader.isWhitespace())
			continue;

		if (xmlReader.isStartElement())
		{
			if (xmlReader.name() == T_DEF_NUMBER_VECTOR)
			{
				readNumberPropertyDefinition();
			}
			else if (xmlReader.name() == T_SET_NUMBER_VECTOR)
			{
				readNumberProperty();
			}
			//TODO: To be continued...
		}
	}
}

Permission IndiClient::readPermissionFromString(const QString& string)
{
	if (string == "rw")
		return PermissionReadWrite;
	else if (string == "wo")
		return PermissionWriteOnly;
	else
		return PermissionReadOnly;
}

State IndiClient::readStateFromString(const QString& string)
{
	if (string == "Idle")
		return StateIdle;
	else if (string == "Ok")
		return StateOk;
	else if (string == "Busy")
		return StateBusy;
	else
		return StateAlert;
}

void IndiClient::readNumberPropertyDefinition()
{
	//TODO: Reuse code!
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString device = attributes.value(A_DEVICE).toString();
	if (device.isEmpty())
	{
		qDebug() << "A 'device' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	QString name = attributes.value(A_NAME).toString();
	if (name.isEmpty())
	{
		qDebug() << "A 'name' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	QString label = attributes.value(A_LABEL).toString();
	QString group = attributes.value(A_GROUP).toString();
	QString stateString = attributes.value(A_STATE).toString();
	if (stateString.isEmpty())
	{
		qDebug() << "A 'state' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	State initialState = readStateFromString(stateString);
	QString permissionString = attributes.value(A_PERMISSION).toString();
	if (permissionString.isEmpty())
	{
		qDebug() << "A 'perm' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	Permission permission = readPermissionFromString(permissionString);
	QString timeoutString = attributes.value(A_TIMEOUT).toString();
	QString timestampString = attributes.value(A_TIMESTAMP).toString();
	QString message = attributes.value(A_MESSAGE).toString();

	NumberProperty* numberProperty = new NumberProperty(name, initialState, permission, label, group);
	while (true)
	{
		xmlReader.readNext();
		if (xmlReader.name() == T_DEF_NUMBER_VECTOR && xmlReader.isEndElement())
			break;
		else if (xmlReader.name() == T_DEF_NUMBER)
		{
			//TODO: Add some mechanism for detecting errors
			readNumberElementDefinition(numberProperty);
		}
	}

	if (numberProperty->elementCount() > 0)
	{
		numberProperties.insert(name, numberProperty);
		emit propertyDefined(device, numberProperty);
	}
	//TODO: Later, allow for device in the data structure

	//TODO: Emit timestamp/message, or only message is no timestamp is available
}

void IndiClient::readNumberElementDefinition(NumberProperty *numberProperty)
{
	//TODO: Reuse code?
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString name = attributes.value(A_NAME).toString();
	if (name.isEmpty())
	{
		qDebug() << "defNumber" << "A 'name' attrbute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	QString label = attributes.value(A_LABEL).toString();
	QString format = attributes.value(A_FORMAT).toString();
	if (format.isEmpty())
	{
		qDebug() << "defNumber" << "A 'format' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	QString min = attributes.value(A_MINIMUM).toString();
	if (min.isEmpty())
	{
		qDebug() << "defNumber" << "A 'min' attribute is required.";
		xmlReader.skipCurrentElement();
		return;
	}
	QString max = attributes.value(A_MAXIMUM).toString();
	if (max.isEmpty())
	{
		qDebug() << "'max'";
		xmlReader.skipCurrentElement();
		return;
	}
	QString step = attributes.value(A_STEP).toString();
	//TODO: Validation

	QString value;
	while (!(xmlReader.name() == T_DEF_NUMBER && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
		{
			value = xmlReader.text().toString().trimmed();
			//TODO: break?
		}
		xmlReader.readNext();
	}
	if (value.isEmpty())
	{
		qDebug() << "defNumber element is empty?";
		return;
	}

	NumberElement* numberElement = new NumberElement(name, value, format, min, max, step, label);
	numberProperty->addElement(numberElement);
}

void IndiClient::readNumberProperty()
{
	//TODO: Reuse code?
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString device = attributes.value(A_DEVICE).toString();
	QString name = attributes.value(A_NAME).toString();
	//TODO: read state, timeout, timestamp, etc.

	//TODO: handle device names
	if (!numberProperties.contains(name))
	{
		qDebug() << "Unknown property name:" << name;
		xmlReader.skipCurrentElement();
		return;
	}
	NumberProperty* numberProperty = numberProperties.value(name);

	while (!(xmlReader.name() == T_SET_NUMBER_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_NUMBER)
		{
			readNumberElement(numberProperty);
		}
		xmlReader.readNext();
	}
	//TODO: Add check if anything has been updated at all?
	emit propertyUpdated(device, numberProperty);
}

void IndiClient::readNumberElement(NumberProperty* numberProperty)
{
	QString name = xmlReader.attributes().value(A_NAME).toString();
	//TODO: Validation
	QString value;
	while (!(xmlReader.name() == T_ONE_NUMBER && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
			value = xmlReader.text().toString().trimmed();
		xmlReader.readNext();
	}
	if (value.isEmpty())
	{
		//TODO
		qDebug() << "oneNumber element is empty?";
		return;
	}
	numberProperty->updateElement(name, value);
}
