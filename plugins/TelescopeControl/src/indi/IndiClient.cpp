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
#include <stdexcept>

const char* IndiClient::T_GET_PROPERTIES = "getProperties";
const char* IndiClient::T_DEL_PROPERTY = "delProperty";
const char* IndiClient::T_ENABLE_BLOB = "enableBLOB";
const char* IndiClient::T_MESSAGE = "message";
const char* IndiClient::T_DEF_TEXT_VECTOR = "defTextVector";
const char* IndiClient::T_DEF_NUMBER_VECTOR = "defNumberVector";
const char* IndiClient::T_DEF_SWITCH_VECTOR = "defSwitchVector";
const char* IndiClient::T_DEF_LIGHT_VECTOR = "defLightVector";
const char* IndiClient::T_DEF_BLOB_VECTOR = "defBLOBVector";
const char* IndiClient::T_SET_TEXT_VECTOR = "setTextVector";
const char* IndiClient::T_SET_NUMBER_VECTOR = "setNumberVector";
const char* IndiClient::T_SET_SWITCH_VECTOR = "setSwitchVector";
const char* IndiClient::T_SET_LIGHT_VECTOR = "setLightVector";
const char* IndiClient::T_SET_BLOB_VECTOR = "setBLOBVector";
const char* IndiClient::T_NEW_TEXT_VECTOR = "newTextVector";
const char* IndiClient::T_NEW_NUMBER_VECTOR = "newNumberVector";
const char* IndiClient::T_NEW_SWITCH_VECTOR = "newSwitchVector";
const char* IndiClient::T_NEW_BLOB_VECTOR = "newBLOBVector";
const char* IndiClient::T_DEF_TEXT = "defText";
const char* IndiClient::T_DEF_NUMBER = "defNumber";
const char* IndiClient::T_DEF_SWITCH = "defSwitch";
const char* IndiClient::T_DEF_LIGHT = "defLight";
const char* IndiClient::T_DEF_BLOB = "defBLOB";
const char* IndiClient::T_ONE_TEXT = "oneText";
const char* IndiClient::T_ONE_NUMBER = "oneNumber";
const char* IndiClient::T_ONE_SWITCH = "oneSwitch";
const char* IndiClient::T_ONE_LIGHT = "oneLight";
const char* IndiClient::T_ONE_BLOB = "oneBLOB";

const char* IndiClient::A_VERSION = "version";
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
const char* IndiClient::SP_CONNECT = "CONNECT";
const char* IndiClient::SP_DISCONNECT = "DISCONNECT";
const char* IndiClient::SP_J2000_COORDINATES = "EQUATORIAL_COORD";
const char* IndiClient::SP_JNOW_COORDINATES = "EQUATORIAL_EOD_COORD";
const char* IndiClient::SP_J2000_COORDINATES_REQUEST = "EQUATORIAL_COORD_REQUEST";
const char* IndiClient::SP_JNOW_COORDINATES_REQUEST = "EQUATORIAL_EOD_COORD_REQUEST";

IndiClient::IndiClient(const QString& _clientId,
                       QIODevice* _ioDevice,
                       QObject* parent)
	: QObject(parent),
	clientId(_clientId),
	ioDevice(0),
	textStream(0)
{
	if (_ioDevice == 0 ||
		!_ioDevice->isOpen() ||
		!_ioDevice->isReadable() ||
		!_ioDevice->isWritable())
	{
		throw std::invalid_argument(std::string("ioDevice is not ready."));
	}
	else
		ioDevice = _ioDevice;

	textStream = new QTextStream(ioDevice);

	connect(ioDevice, SIGNAL(readyRead()),
	        this, SLOT(handleIncomingCommands()));

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

QString IndiClient::getId() const
{
	return clientId;
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
			//TODO: Sort by frequence.
			if (xmlReader.name() == T_DEF_TEXT_VECTOR)
			{
				readTextPropertyDefinition();
			}
			else if (xmlReader.name() == T_DEF_NUMBER_VECTOR)
			{
				readNumberPropertyDefinition();
			}
			else if (xmlReader.name() == T_DEF_SWITCH_VECTOR)
			{
				readSwitchPropertyDefinition();
			}
			else if (xmlReader.name() == T_DEF_LIGHT_VECTOR)
			{
				readLightPropertyDefintion();
			}
			else if (xmlReader.name() == T_DEF_BLOB_VECTOR)
			{
				readBlobPropertyDefinition();
			}
			else if (xmlReader.name() == T_SET_NUMBER_VECTOR)
			{
				readNumberProperty();
			}
			else if (xmlReader.name() == T_SET_SWITCH_VECTOR)
			{
				readSwitchProperty();
			}
			else if (xmlReader.name() == T_MESSAGE)
			{
				readMessageElement();
			}
			//TODO: To be continued...
		}
	}
}

void IndiClient::logMessage(const QString& device,
                            const QDateTime& timestamp,
                            const QString& message)
{
	QDateTime time = QDateTime::currentDateTime();
	if (timestamp.isValid())
		time = timestamp.toLocalTime();

	if (device.isEmpty())
		qWarning() << "INDI:" << time.toString(Qt::ISODate) << message;
	else
		qWarning() << "INDI:" << time.toString(Qt::ISODate) << device << message;
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
                                        State& state,
                                        Permission& permission,
                                        SwitchRule& switchRule,
                                        QString& timeout,
                                        QDateTime& timestamp,
                                        bool checkPermission,
                                        bool checkSwitchRule)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	device = attributes.value(A_DEVICE).toString();
	name = attributes.value(A_NAME).toString();
	label = attributes.value(A_LABEL).toString();
	group = attributes.value(A_GROUP).toString();
	QString stateString = attributes.value(A_STATE).toString();
	QString permissionString = attributes.value(A_PERMISSION).toString();
	QString ruleString  = attributes.value(A_RULE).toString();
	timeout = attributes.value(A_TIMEOUT).toString();
	timestamp = readTimestampAttribute();
	readMessageAttribute(device, timestamp);

	//Check for required attributes
	//Permission is not used for arrays of Lights.
	if (device.isEmpty() ||
	    name.isEmpty() ||
	    stateString.isEmpty() ||
	    (permissionString.isEmpty() && checkPermission) ||
	    (ruleString.isEmpty() && checkSwitchRule))
	{
		qDebug() << "A required attribute is missing"
		         << "(device, name, state, permission, switch rule):"
		         << device << name << stateString << permissionString << ruleString;
		return false;
	}

	state = readStateFromString(stateString);

	if (checkPermission)
		permission = readPermissionFromString(permissionString);

	if (checkSwitchRule)
		switchRule = readSwitchRuleFromString(ruleString);

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

QDateTime IndiClient::readTimestampAttribute()
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString timestampString = attributes.value(A_TIMESTAMP).toString();
	QDateTime timestamp;
	if (!timestampString.isEmpty())
	{
		timestamp = QDateTime::fromString(timestampString, Qt::ISODate);
		timestamp.setTimeSpec(Qt::UTC);
	}
	return timestamp;
}

void IndiClient::readMessageAttribute(const QString &device,
                                      const QDateTime &timestamp)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString messageString = attributes.value(A_MESSAGE).toString();
	if (!messageString.isEmpty())
	{
		emit messageReceived(device, timestamp, messageString);
	}
}

void IndiClient::readMessageElement()
{
	QString device = xmlReader.attributes().value(A_DEVICE).toString();
	QDateTime timestamp = readTimestampAttribute();
	readMessageAttribute(device, timestamp);
	while (!xmlReader.isEndElement() && xmlReader.name() != T_MESSAGE)
		xmlReader.readNext();
}

template<class P, class E>
void IndiClient::readPropertyElementsDefinitions
	(const QString& propertyName,
	 const QString& device,
	 P* property,
	 const QString& propertyTag,
	 const QString& elementTag)
{
	while (true)
	{
		xmlReader.readNext();
		if (xmlReader.name() == propertyTag && xmlReader.isEndElement())
			break;
		else if (xmlReader.name() == elementTag)
		{
			QXmlStreamAttributes attributes = xmlReader.attributes();
			QString name = attributes.value(A_NAME).toString();
			QString label = attributes.value(A_LABEL).toString();
			if (name.isEmpty())
				return;

			QString value = readElementRawValue(elementTag);
			if (value.isEmpty())
			{
				return;
			}

			E* element = new E(name, value, label);
			property->addElement(element);
		}
	}

	if (property->elementCount() > 0)
	{
		deviceProperties[device].insert(propertyName, property);
		emit propertyDefined(clientId, device, property);
	}
	else
	{
		delete property;
	}
}

template<>
void IndiClient::readPropertyElementsDefinitions<NumberProperty,NumberElement>
	(const QString& propertyName,
	const QString& device,
	NumberProperty* property,
	const QString& propertyTag,
	const QString& elementTag)
{
	while (true)
	{
		xmlReader.readNext();
		if (xmlReader.name() == propertyTag && xmlReader.isEndElement())
			break;
		else if (xmlReader.name() == elementTag)
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

			QString value = readElementRawValue(elementTag);
			if (value.isEmpty())
			{
				return;
			}

			NumberElement* numberElement = new NumberElement(name, value, format, min, max, step, label);
			property->addElement(numberElement);
		}
	}

	if (property->elementCount() > 0)
	{
		deviceProperties[device].insert(propertyName, property);
		emit propertyDefined(clientId, device, property);
	}
	else
	{
		delete property;
	}
}

template
void IndiClient::readPropertyElementsDefinitions<TextProperty,TextElement>
	(const QString&, const QString&, TextProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<NumberProperty,NumberElement>
	(const QString&, const QString&, NumberProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<SwitchProperty,SwitchElement>
	(const QString&, const QString&, SwitchProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<LightProperty,LightElement>
	(const QString&, const QString&, LightProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<BlobProperty,BlobElement>
	(const QString&, const QString&, BlobProperty*, const QString&, const QString&);

void IndiClient::readTextPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	State state;
	Permission permission;
	SwitchRule rule;
	QString timeoutString;
	QDateTime timestamp;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       state,
	                                                       permission,
	                                                       rule,
	                                                       timeoutString,
	                                                       timestamp,
	                                                       true,
	                                                       false);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	//TODO: Handle timeout, etc.

	TextProperty* property = new TextProperty
		(name, state, permission, label, group, timestamp);
	readPropertyElementsDefinitions<TextProperty, TextElement>
		(name, device, property, T_DEF_TEXT_VECTOR, T_DEF_TEXT);
}

void IndiClient::readNumberPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	State state;
	Permission permission;
	SwitchRule rule;
	QString timeoutString;
	QDateTime timestamp;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       state,
	                                                       permission,
	                                                       rule,
	                                                       timeoutString,
	                                                       timestamp,
	                                                       true,
	                                                       false);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	//TODO: Handle timeout, etc.

	NumberProperty* property = new NumberProperty
		(name, state, permission, label, group, timestamp);
	readPropertyElementsDefinitions<NumberProperty, NumberElement>
		(name, device, property, T_DEF_NUMBER_VECTOR, T_DEF_NUMBER);
}

void IndiClient::readSwitchPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	State state;
	Permission permission;
	SwitchRule rule;
	QString timeoutString;
	QDateTime timestamp;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       state,
	                                                       permission,
	                                                       rule,
	                                                       timeoutString,
	                                                       timestamp,
	                                                       true,
	                                                       true);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	//TODO: Handle timeout, etc.

	SwitchProperty* property = new SwitchProperty
		(name, state, permission, rule, label, group, timestamp);
	readPropertyElementsDefinitions<SwitchProperty, SwitchElement>
		(name, device, property, T_DEF_SWITCH_VECTOR, T_DEF_SWITCH);
}

void IndiClient::readLightPropertyDefintion()
{
	QString device;
	QString name;
	QString label;
	QString group;
	State state;
	Permission permission;
	SwitchRule rule;
	QString timeoutString;
	QDateTime timestamp;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       state,
	                                                       permission,
	                                                       rule,
	                                                       timeoutString,
	                                                       timestamp,
	                                                       false,
	                                                       false);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	//TODO: Handle timeout, etc.

	LightProperty* property = new LightProperty
		(name, state, label, group, timestamp);
	readPropertyElementsDefinitions<LightProperty, LightElement>
		(name, device, property, T_DEF_LIGHT_VECTOR, T_DEF_LIGHT);
}

void IndiClient::readBlobPropertyDefinition()
{
	QString device;
	QString name;
	QString label;
	QString group;
	State state;
	Permission permission;
	SwitchRule rule;
	QString timeoutString;
	QDateTime timestamp;
	bool hasAllRequiredAttributes = readPropertyAttributes(device,
	                                                       name,
	                                                       label,
	                                                       group,
	                                                       state,
	                                                       permission,
	                                                       rule,
	                                                       timeoutString,
	                                                       timestamp,
	                                                       true,
	                                                       false);
	if (!hasAllRequiredAttributes)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	//TODO: Handle timeout, etc.

	//TODO: Intialize save directory
	BlobProperty* property = new BlobProperty
		(name, state, permission, "", label, group, timestamp);
	readPropertyElementsDefinitions<BlobProperty, BlobElement>
		(name, device, property, T_DEF_BLOB_VECTOR, T_DEF_BLOB);
}

QString IndiClient::readElementRawValue(const QString& tagName)
{
	QString value;
	while(!(xmlReader.name() == tagName && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
		{
			value = xmlReader.text().toString().trimmed();
			//TODO: break?
		}
		xmlReader.readNext();
	}
	return value;
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
	QDateTime timestamp = readTimestampAttribute();
	readMessageAttribute(device, timestamp);
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

	QHash<QString,QString> values;
	while (!(xmlReader.name() == T_SET_NUMBER_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_NUMBER)
		{
			readOneElement(T_ONE_NUMBER, values);
		}
		xmlReader.readNext();
	}
	if (!values.isEmpty())
	{
		if (state.isEmpty())
			numberProperty->update(values, timestamp);
		else
			numberProperty->update(values, timestamp, readStateFromString(state));
		emit propertyUpdated(clientId, device, numberProperty);;
	}
}

void IndiClient::readOneElement(const QString& tag,
                                   QHash<QString, QString>& newValues)
{
	QString name = xmlReader.attributes().value(A_NAME).toString();
	if (name.isEmpty())
	{
		xmlReader.skipCurrentElement();
		return;
	}
	QString value;
	while (!(xmlReader.name() == tag && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
			value = xmlReader.text().toString().trimmed();
		xmlReader.readNext();
	}
	if (value.isEmpty())
	{
		return;
	}
	newValues.insert(name, value);
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
	QDateTime timestamp = readTimestampAttribute();
	readMessageAttribute(device, timestamp);
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

	QHash<QString,QString> values;
	while(!(xmlReader.name() == T_SET_SWITCH_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_SWITCH)
		{
			readOneElement(T_ONE_SWITCH, values);
		}
		xmlReader.readNext();
	}
	if (!values.isEmpty())
	{
		if (state.isEmpty())
			switchProperty->update(values, timestamp);
		else
			switchProperty->update(values, timestamp, readStateFromString(state));
		emit propertyUpdated(clientId, device, switchProperty);
	}
}

void IndiClient::writeGetProperties(const QString& device,
                                    const QString& property)
{
	if (textStream == 0)
		return;

	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isWritable())
		return;

	QXmlStreamWriter xmlWriter(ioDevice);
	xmlWriter.writeStartDocument();
	xmlWriter.writeEmptyElement(T_GET_PROPERTIES);
	//TODO: Centralized storage of supported version number
	xmlWriter.writeAttribute(A_VERSION, "1.7");
	if (!device.isEmpty())
		xmlWriter.writeAttribute(A_DEVICE, device);
	if (!property.isEmpty())
		xmlWriter.writeAttribute(A_NAME, property);
	xmlWriter.writeEndDocument();
}

void IndiClient::writeProperty(const QString& deviceName,
                               const QString& propertyName,
                               const QVariantHash& newValues)
{
	if (!deviceProperties.contains(deviceName) ||
	    !deviceProperties[deviceName].contains(propertyName) ||
	    newValues.isEmpty())
	{
		//TODO: Log?
		return;
	}

	Property* property = deviceProperties[deviceName][propertyName];
	Q_ASSERT(property); //TODO: Check this!

	QXmlStreamWriter xmlWriter(ioDevice);
	xmlWriter.writeStartDocument();

	Property::PropertyType type = property->getType();
	switch (type)
	{
		case Property::TextProperty:
		{
			break;
		}

		case Property::NumberProperty:
		{
			NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
			if (numberProperty)
				writeNumberProperty(xmlWriter, deviceName,
				                    numberProperty, newValues);
			break;
		}

		case Property::SwitchProperty:
		{
			SwitchProperty* switchProperty = dynamic_cast<SwitchProperty*>(property);
			if (switchProperty)
				writeSwitchProperty(xmlWriter, deviceName,
				                    switchProperty, newValues);
			break;
		}

		case Property::BlobProperty:
		{
			break;
		}

		default:
		{
			//TODO: Log?
			break;
		}
	}

	xmlWriter.writeEndDocument();
}

void IndiClient::writeTextProperty(QXmlStreamWriter& xmlWriter,
                                   const QString& device,
                                   Property *property,
                                   const QVariantHash& newValues)
{
	Q_UNUSED(xmlWriter)
	Q_UNUSED(device)
	Q_UNUSED(property)
	Q_UNUSED(newValues)
}

void IndiClient::writeNumberProperty(QXmlStreamWriter& xmlWriter,
                                     const QString& device,
                                     NumberProperty* property,
                                     const QVariantHash& newValues)
{
	Q_ASSERT(property); //TODO: Proper check!

	xmlWriter.writeStartElement(T_NEW_NUMBER_VECTOR);
	xmlWriter.writeAttribute(A_DEVICE, device);
	xmlWriter.writeAttribute(A_NAME, property->getName());
	//TODO: Add timestamp?

	QStringList elements = property->getElementNames();//TODO: Optimize this
	foreach (const QString& element, elements)
	{
		xmlWriter.writeStartElement(T_ONE_NUMBER);
		xmlWriter.writeAttribute(A_NAME, element);
		double value;
		QVariant elementValue = newValues.value(element, QVariant());
		//Doubles as a check if such an element exists:
		//an empty QVariant is of type Invalid.
		if (elementValue.type() == QVariant::Double)
			value = elementValue.toDouble();
		else
			value = property->getElement(element)->getValue();
		xmlWriter.writeCharacters(QString::number(value, 'f'));
		xmlWriter.writeEndElement();
	}

	xmlWriter.writeEndElement();

	//TODO: Update property state and send it to the UI (additional signal?)
}

void IndiClient::writeSwitchProperty(QXmlStreamWriter& xmlWriter,
                                     const QString& device,
                                     SwitchProperty* property,
                                     const QVariantHash& newValues)
{
	Q_ASSERT(property); //TODO: Proper check!

	xmlWriter.writeStartElement(T_NEW_SWITCH_VECTOR);
	xmlWriter.writeAttribute(A_DEVICE, device);
	xmlWriter.writeAttribute(A_NAME, property->getName());
	//TODO: Add timestamp?

	QStringList elements = property->getElementNames();//TODO: Optimize this
	foreach (const QString& element, elements)
	{
		xmlWriter.writeStartElement(T_ONE_SWITCH);
		xmlWriter.writeAttribute(A_NAME, element);
		bool value;
		QVariant elementValue = newValues.value(element, QVariant());
		//Doubles as a check if such an element exists:
		//an empty QVariant is of type Invalid.
		if (elementValue.type() == QVariant::Bool)
			value = elementValue.toBool();
		else
			value = property->getElement(element)->isOn();
		xmlWriter.writeCharacters((value)?"On":"Off");
		xmlWriter.writeEndElement();
	}

	xmlWriter.writeEndElement();

	//TODO: Update property state and send it to the UI (additional signal?)
}

void IndiClient::writeBlobProperty(QXmlStreamWriter& xmlWriter,
                                   const QString& device,
                                   Property* property,
                                   const QVariantHash& newValues)
{
	Q_UNUSED(xmlWriter)
	Q_UNUSED(device)
	Q_UNUSED(property)
	Q_UNUSED(newValues)
}
