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
#include <QFile>
#include <QRegExp>
#include <QStringList>
#include <QXmlStreamWriter>

const char* IndiClient::T_DEF_NUMBER_VECTOR = "defNumberVector";
const char* IndiClient::T_DEF_SWITCH_VECTOR = "defSwitchVector";
const char* IndiClient::T_SET_NUMBER_VECTOR = "setNumberVector";
const char* IndiClient::T_SET_SWITCH_VECTOR = "setSwitchVector";
const char* IndiClient::T_NEW_NUMBER_VECTOR = "newNumberVector";
const char* IndiClient::T_NEW_SWITCH_VECTOR = "newSwitchVector";
const char* IndiClient::T_DEF_NUMBER = "defNumber";
const char* IndiClient::T_DEF_SWITCH = "defSwitch";
const char* IndiClient::T_ONE_NUMBER = "oneNumber";
const char* IndiClient::T_ONE_SWITCH = "oneSwitch";

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
const char* IndiClient::A_RULE = "rule";

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

QHash<QString,QString> IndiClient::loadDeviceDescriptions()
{
	QHash<QString,QString> result;

	//TODO: It should allow the file path to be set somewhere
	QFile indiDriversXmlFile("/usr/share/indi/drivers.xml");
	if (indiDriversXmlFile.open(QFile::ReadOnly | QFile::Text))
	{
		QXmlStreamReader xmlReader(&indiDriversXmlFile);

		QString deviceName;
		QString driverName;
		while (!xmlReader.atEnd())
		{
			if (xmlReader.hasError())
			{
				qDebug() << "Error parsing drivers.xml:"
				         << xmlReader.errorString();
				break;
			}

			if (xmlReader.isEndDocument())
				break;

			if (xmlReader.isStartElement())
			{
				if (xmlReader.name() == "devGroup")
				{
					if (xmlReader.attributes().value("group").toString() != "Telescopes")
						xmlReader.skipCurrentElement();
				}
				else if (xmlReader.name() == "device")
				{
					deviceName = xmlReader.attributes().value("label").toString();
					if (deviceName.isEmpty())
						xmlReader.skipCurrentElement();
				}
				else if (xmlReader.name() == "driver")
				{
					if (deviceName.isEmpty())
						xmlReader.skipCurrentElement();
					driverName = xmlReader.readElementText(QXmlStreamReader::SkipChildElements).trimmed();
					if (driverName.isEmpty())
						xmlReader.skipCurrentElement();
					result.insert(deviceName, driverName);
				}
			}

			xmlReader.readNext();
		}

		indiDriversXmlFile.close();
	}
	else
	{
		qDebug() << "Unable to open drivers.xml.";
	}

	return result;
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
			else if (xmlReader.name() == T_DEF_SWITCH_VECTOR)
			{
				readSwitchPropertyDefinition();
			}
			else if (xmlReader.name() == T_SET_NUMBER_VECTOR)
			{
				readNumberProperty();
			}
			else if (xmlReader.name() == T_SET_SWITCH_VECTOR)
			{
				readSwitchProperty();
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

SwitchRule IndiClient::readSwitchRuleFromString(const QString& string)
{
	if (string == "OneOfMany")
		return SwitchOnlyOne;
	else if (string == "AtMostOne")
		return SwitchAtMostOne;
	else
		return SwitchAny;
}

bool IndiClient::readPropertyAttributes(QString& device,
                                                QString& name,
                                                QString& label,
                                                QString& group,
                                                QString& state,
                                                QString& permission,
                                                QString& timeout,
                                                bool checkPermission)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	device = attributes.value(A_DEVICE).toString();
	name = attributes.value(A_NAME).toString();
	label = attributes.value(A_LABEL).toString();
	group = attributes.value(A_GROUP).toString();
	state = attributes.value(A_STATE).toString();
	permission = attributes.value(A_PERMISSION).toString();
	timeout = attributes.value(A_TIMEOUT).toString();

	//Check for required attributes
	//Permission is not used for arrays of Lights.
	if (device.isEmpty() ||
	    name.isEmpty() ||
	    state.isEmpty() ||
	    (permission.isEmpty() && checkPermission))
	{
		qDebug() << "A required attribute is missing"
		         << "(device, name, state, permission):"
		         << device << name << state << permission;
		return false;
	}

	return true;
}

bool IndiClient::readPropertyAttributes(QString& device,
                                        QString& name,
                                        QString& state,
                                        QString& timeout)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	device = attributes.value(A_DEVICE).toString();
	name = attributes.value(A_NAME).toString();
	state = attributes.value(A_STATE).toString();
	timeout = attributes.value(A_TIMEOUT).toString();

	//Check for required attributes
	if (device.isEmpty() ||
	    name.isEmpty())
	{
		qDebug() << "A required attribute is missing"
		         << "(device, name):"
		         << device << name;
		return false;
	}

	return true;
}

void IndiClient::readPropertyAttributesTimestampAndMessage(const QString &device,
														   QDateTime &timestamp)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString timestampString = attributes.value(A_TIMESTAMP).toString();
	QString messageString = attributes.value(A_MESSAGE).toString();

	if (!timestampString.isEmpty())
	{
		timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
	}
	if (!messageString.isEmpty())
	{
		QString logString;
		if (timestamp.isValid())
			logString = QString("[%1] %2: %3").arg(timestamp.time().toString("hh:mm:ss")).arg(device).arg(messageString);
		emit messageLogged(device, timestamp, messageString);
	}
}

void IndiClient::readNumberPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	QString stateString;
	QString permissionString;
	QString timeoutString;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       stateString,
	                                                       permissionString,
	                                                       timeoutString,
	                                                       true);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	QDateTime timestamp;
	readPropertyAttributesTimestampAndMessage(device, timestamp);

	State initialState = readStateFromString(stateString);
	Permission permission = readPermissionFromString(permissionString);

	//TODO: Handle timestamp, timeout, etc.
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
		deviceProperties[device].insert(name, numberProperty);
		emit propertyDefined(device, numberProperty);
	}
	else
	{
		delete numberProperty;
	}

	//TODO: Emit timestamp/message, or only message is no timestamp is available
}

void IndiClient::readNumberElementDefinition(NumberProperty* numberProperty)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString name = attributes.value(A_NAME).toString();
	QString label = attributes.value(A_LABEL).toString();
	QString format = attributes.value(A_FORMAT).toString();
	QString min = attributes.value(A_MINIMUM).toString();
	QString max = attributes.value(A_MAXIMUM).toString();
	QString step = attributes.value(A_STEP).toString();
	if (name.isEmpty() ||
	    format.isEmpty() ||
	    min.isEmpty() ||
	    max.isEmpty() ||
	    step.isEmpty())
	{
		qDebug() << "A required attribute is missing"
		         << "(name, format, min, max, step):"
		         << name << format << min << max << step;
		xmlReader.skipCurrentElement();
		return;
	}
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

void IndiClient::readSwitchPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	QString stateString;
	QString permissionString;
	QString timeoutString;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       stateString,
	                                                       permissionString,
	                                                       timeoutString,
	                                                       true);

	QString ruleString = xmlReader.attributes().value(A_RULE).toString();
	if (ruleString.isEmpty())
	{
		hasAllRequiredAttributes = false;
	}

	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	QDateTime timestamp;
	readPropertyAttributesTimestampAndMessage(device, timestamp);

	State initialState = readStateFromString(stateString);
	Permission permission = readPermissionFromString(permissionString);
	SwitchRule rule = readSwitchRuleFromString(ruleString);

	//TODO: Handle timestamp, timeout, etc.
	SwitchProperty* switchProperty = new SwitchProperty(name, initialState, permission, rule, label, group);
	while(true)
	{
		xmlReader.readNext();
		if (xmlReader.name() == T_DEF_SWITCH_VECTOR && xmlReader.isEndElement())
			break;
		else if (xmlReader.name() == T_DEF_SWITCH)
		{
			readSwitchElementDefinition(switchProperty);
		}
	}

	if (switchProperty->elementCount() > 0)
	{
		deviceProperties[device].insert(name, switchProperty);
		emit propertyDefined(device, switchProperty);
	}
	else
	{
		delete switchProperty;
	}

	//TODO: Emit timestamp/message, or only message is no timestamp is available
}

void IndiClient::readSwitchElementDefinition(SwitchProperty* switchProperty)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString name = attributes.value(A_NAME).toString();
	QString label = attributes.value(A_LABEL).toString();
	//TODO: Check this?

	QString value;
	while(!(xmlReader.name() == T_DEF_SWITCH && xmlReader.isEndElement()))
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
		qDebug() << "defSwitch element is empty?";
		return;
	}

	SwitchElement* switchElement = new SwitchElement(name, value, label);
	switchProperty->addElement(switchElement);
}

void IndiClient::readNumberProperty()
{
	QString device;
	QString name;
	QString state;
	QString timeout;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       state,
	                                                       timeout);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	QDateTime timestamp;
	readPropertyAttributesTimestampAndMessage(device, timestamp);
	//TODO: Handle state, timeout

	if (!deviceProperties.contains(device))
	{
		qDebug() << "Unknown device name:" << device;
		xmlReader.skipCurrentElement();
		return;
	}
	if (!deviceProperties[device].contains(name))
	{
		qDebug() << "Unknown property name:" << name;
		xmlReader.skipCurrentElement();
		return;
	}
	Property* property = deviceProperties[device].value(name);
	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
	if (numberProperty == 0)//TODO: What does it return exactly if the cast fails?
	{
		qDebug() << "Not a number property:" << name;
		xmlReader.skipCurrentElement();
		return;
	}

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

void IndiClient::readSwitchProperty()
{
	QString device;
	QString name;
	QString state;
	QString timeout;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       state,
	                                                       timeout);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	QDateTime timestamp;
	readPropertyAttributesTimestampAndMessage(device, timestamp);
	//TODO: Handle state, timeout, etc.

	if (!deviceProperties.contains(device))
	{
		qDebug() << "Unknown device name:" << device;
		xmlReader.skipCurrentElement();
		return;
	}
	if (!deviceProperties[device].contains(name))
	{
		qDebug() << "Unknown property name:" << name;
		xmlReader.skipCurrentElement();
		return;
	}
	Property* property = deviceProperties[device].value(name);
	SwitchProperty* switchProperty = dynamic_cast<SwitchProperty*>(property);
	if (switchProperty == 0)
	{
		qDebug() << "Not a SwitchProperty" << name;
		xmlReader.skipCurrentElement();
		return;
	}

	while(!(xmlReader.name() == T_SET_SWITCH_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_SWITCH)
		{
			readSwitchElement(switchProperty);
		}
		xmlReader.readNext();
	}

	//TODO: Add check if anything has been updated at all?
	emit propertyUpdated(device, switchProperty);
}

void IndiClient::readSwitchElement(SwitchProperty* switchProperty)
{
	QString name = xmlReader.attributes().value(A_NAME).toString();
	QString value;
	while(!(xmlReader.name() == T_ONE_SWITCH && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
			value = xmlReader.text().toString().trimmed();
		xmlReader.readNext();
	}
	if (value.isEmpty())
	{
		return;
	}
	switchProperty->updateElement(name, value);
}

void IndiClient::writeTextProperty(const QString &device, const QString &property, const QHash<QString, QString> &newValues)
{
	Q_UNUSED(device)
	Q_UNUSED(property)
	Q_UNUSED(newValues)
}

//TODO: Look into using a template function
void IndiClient::writeNumberProperty(const QString& device,
                                     const QString& property,
                                     const QHash<QString, double>& newValues)
{
	if (!deviceProperties.contains(device) ||
	    !deviceProperties[device].contains(property) ||
	    newValues.isEmpty())
	{
		//TODO: Log?
		return;
	}

	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(deviceProperties[device][property]);
	if (numberProperty)
	{
		QXmlStreamWriter xmlWriter(ioDevice);
		xmlWriter.writeStartDocument();
		xmlWriter.writeStartElement(T_NEW_NUMBER_VECTOR);
		xmlWriter.writeAttribute(QXmlStreamAttribute(A_DEVICE, device));
		xmlWriter.writeAttribute(QXmlStreamAttribute(A_NAME, property));
		//TODO: Add timestamp?

		QStringList elements = numberProperty->getElementNames();//TODO: Optimize this
		foreach (const QString& element, elements)
		{
			xmlWriter.writeStartElement(T_ONE_NUMBER);
			xmlWriter.writeAttribute(QXmlStreamAttribute(A_NAME, element));
			double value;
			//TODO: Check if this is actually necessary
			if (newValues.contains(element))
				value = newValues[element];
			else
				value = numberProperty->getElement(element)->getValue();
			xmlWriter.writeCharacters(QString::number(value));
			xmlWriter.writeEndElement();
		}

		xmlWriter.writeEndElement();
		xmlWriter.writeEndDocument();
	}
}

void IndiClient::writeSwitchProperty(const QString &device, const QString &property, const QHash<QString, bool> &newValues)
{
	Q_UNUSED(device)
	Q_UNUSED(property)
	Q_UNUSED(newValues)
}

void IndiClient::writeBlobProperty(const QString &device, const QString &property, const QHash<QString, QByteArray> &newValues)
{
	Q_UNUSED(device)
	Q_UNUSED(property)
	Q_UNUSED(newValues)
}
