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

const char* IndiClient::SP_CONNECTION = "CONNECTION";
const char* IndiClient::SP_CONNECT = "CONNECT";
const char* IndiClient::SP_DISCONNECT = "DISCONNECT";
const char* IndiClient::SP_J2000_COORDINATES = "EQUATORIAL_COORD";
const char* IndiClient::SP_JNOW_COORDINATES = "EQUATORIAL_EOD_COORD";
const char* IndiClient::SP_J2000_COORDINATES_REQUEST = "EQUATORIAL_COORD_REQUEST";
const char* IndiClient::SP_JNOW_COORDINATES_REQUEST = "EQUATORIAL_EOD_COORD_REQUEST";

IndiClient::IndiClient(const QString& newClientId,
                       QIODevice* openIoDevice,
                       QObject* parent)
	: QObject(parent),
	clientId(newClientId),
	ioDevice(0)
{
	if (openIoDevice == 0 ||
		!openIoDevice->isOpen() ||
		!openIoDevice->isReadable() ||
		!openIoDevice->isWritable())
	{
		throw std::invalid_argument(std::string("ioDevice is not ready."));
	}
	else
		ioDevice = openIoDevice;

	connect(ioDevice, SIGNAL(readyRead()),
	        this, SLOT(handleIncomingCommands()));

	//Make the parser think it's parsing parts of a large document
	//(otherwise it thinks that the first tag in the message is the root one)
	//"Extra content at end of document."
	//TODO: Think of a better way?
	//xmlReader.addData("<indi>");
}

IndiClient::~IndiClient()
{

	if (ioDevice)
	{
		disconnect(ioDevice, SIGNAL(readyRead()),
		           this, SLOT(handleIncomingCommands()));
	}
}

QString IndiClient::getId() const
{
	return clientId;
}

bool IndiClient::isConnected() const
{
	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isReadable() ||
		!ioDevice->isWritable())
		return false;

	return true;
}

void IndiClient::sendRawCommand(const QString& command)
{
	if (!isConnected())
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
	qDebug() << "handleIncomingCommands()";
	if (!isConnected())
		return;

	//Get rid of "XML declaration not at start of document." errors
	//(Damn INDI and badly formed code!)

	//const QRegExp xmlDeclaration("<\\?[^>]+>");
	//buffer.remove(xmlDeclaration);
	//xmlReader.addData(buffer);

	// TODO: Initialize the QXmlStreamReader only once...
	QByteArray newPortion = ioDevice->readAll();
	//qDebug() << newPortion;
	buffer.append(newPortion);
	QXmlStreamReader xmlReader(buffer);
	qint64 startPos = 0, offset = 0;

	while (!xmlReader.atEnd())
	{
		xmlReader.readNext();
		//qDebug() << xmlReader.tokenString();
		//TODO: Is this check necessary? Will it work?
		if (xmlReader.isEndDocument())
		{
			qDebug() << "EOF reached?" << buffer << xmlReader.name();
			buffer.clear();
			break;
		}
		else if (xmlReader.tokenType() == QXmlStreamReader::Invalid)
		{
			QXmlStreamReader::Error errorCode = xmlReader.error();
			if (errorCode == QXmlStreamReader::PrematureEndOfDocumentError)
			{
				//Happens when the end of the current "transmission" has been
				//reached.
				//TODO: Better way of handling this.
				qDebug() << "Command segment read.";
				//break;
				return;
			}
			else if (errorCode == QXmlStreamReader::NotWellFormedError
			         && xmlReader.errorString() == "Extra content at end of document.")
			{
				QString command = buffer.mid(startPos, offset-startPos);
				parseIndiCommand(command);
				//qDebug() << "Command:" << command;
				buffer = buffer.mid(offset);
				xmlReader.clear();
				xmlReader.addData(buffer);
				offset = 0;
				startPos = 0;
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

		offset = xmlReader.characterOffset();
	}
	qDebug() << "handleIncomingCommands() ends.";
}

void IndiClient::parseIndiCommand(const QString& command)
{
	// TODO: Use only one XML parser!
	QXmlStreamReader xmlReader(command);

	// If we are going to use two, let's at least assume that this is well-formed XML...
	while (xmlReader.readNextStartElement())
	{
		//TODO: Sort by frequence.
		if (xmlReader.name() == T_DEF_TEXT_VECTOR)
		{
			readTextPropertyDefinition(xmlReader);
		}
		else if (xmlReader.name() == T_DEF_NUMBER_VECTOR)
		{
			readNumberPropertyDefinition(xmlReader);
		}
		else if (xmlReader.name() == T_DEF_SWITCH_VECTOR)
		{
			readSwitchPropertyDefinition(xmlReader);
		}
		else if (xmlReader.name() == T_DEF_LIGHT_VECTOR)
		{
			readLightPropertyDefintion(xmlReader);
		}
		else if (xmlReader.name() == T_DEF_BLOB_VECTOR)
		{
			readBlobPropertyDefinition(xmlReader);
		}
		else if (xmlReader.name() == T_SET_NUMBER_VECTOR)
		{
			readNumberProperty(xmlReader);
		}
		else if (xmlReader.name() == T_SET_SWITCH_VECTOR)
		{
			readSwitchProperty(xmlReader);
		}
		else if (xmlReader.name() == T_SET_BLOB_VECTOR)
		{
			readBlobProperty(xmlReader);
		}
		else if (xmlReader.name() == T_MESSAGE)
		{
			readMessageElement(xmlReader);
		}
		//TODO: To be continued...
		else
		{
			xmlReader.skipCurrentElement();
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

Property* IndiClient::getProperty(const SetTagAttributes& attributes)
{
	const QString& device = attributes.device;
	if (!deviceProperties.contains(device))
	{
		qDebug() << "Unknown device name:" << device;
		return 0;
	}
	const QString& name = attributes.name;
	if (!deviceProperties[device].contains(name))
	{
		qDebug() << "Unknown property name" << name
		         << "for device" << device;
		return 0;
	}
	return deviceProperties[device].value(name);
}

void IndiClient::handleMessageAttribute(const TagAttributes& attributes)
{
	if (!attributes.message.isEmpty())
	{
		emit messageReceived(attributes.device,
		                     attributes.timestamp,
		                     attributes.message);
	}
}

void IndiClient::readMessageElement(QXmlStreamReader& xmlReader)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString device = attributes.value(TagAttributes::DEVICE).toString();
	QDateTime timestamp = TagAttributes::readTimestampAttribute(attributes);
	QString message = attributes.value(TagAttributes::MESSAGE).toString();
	if (!message.isEmpty())
		emit messageReceived(device, timestamp, message);
	
	// TODO: Is this necessary?
	xmlReader.readNextStartElement();
}

template<class P, class E>
void IndiClient::readPropertyElementsDefinitions
	(QXmlStreamReader& xmlReader,
	 const QString& propertyName,
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
			E* element = new E(attributes);
			if (!element->isValid())
			{
				delete element;
				return;
			}

			QString value = readElementRawValue(xmlReader, elementTag);
			if (value.isEmpty() && elementTag != T_DEF_BLOB)
			{
				return;
			}

			element->setValue(value);
			if (!element->isEmpty())
				property->addElement(element);
			else
				delete element;
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
	(QXmlStreamReader& xmlReader, const QString&, const QString&, TextProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<NumberProperty,NumberElement>
	(QXmlStreamReader& xmlReader, const QString&, const QString&, NumberProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<SwitchProperty,SwitchElement>
	(QXmlStreamReader& xmlReader, const QString&, const QString&, SwitchProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<LightProperty,LightElement>
	(QXmlStreamReader& xmlReader, const QString&, const QString&, LightProperty*, const QString&, const QString&);
template
void IndiClient::readPropertyElementsDefinitions<BlobProperty,BlobElement>
	(QXmlStreamReader& xmlReader, const QString&, const QString&, BlobProperty*, const QString&, const QString&);

void IndiClient::readTextPropertyDefinition(QXmlStreamReader& xmlReader)
{
	StandardDefTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle timeout, etc.

	TextProperty* property = new TextProperty (attributes);
	readPropertyElementsDefinitions<TextProperty, TextElement>
		(xmlReader, attributes.name, attributes.device, property, T_DEF_TEXT_VECTOR, T_DEF_TEXT);
}

void IndiClient::readNumberPropertyDefinition(QXmlStreamReader& xmlReader)
{
	StandardDefTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle timeout, etc.

	NumberProperty* property = new NumberProperty(attributes);
	readPropertyElementsDefinitions<NumberProperty, NumberElement>
		(xmlReader, attributes.name, attributes.device, property, T_DEF_NUMBER_VECTOR, T_DEF_NUMBER);
}

void IndiClient::readSwitchPropertyDefinition(QXmlStreamReader& xmlReader)
{
	DefSwitchTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle timeout, etc.

	SwitchProperty* property = new SwitchProperty(attributes);
	readPropertyElementsDefinitions<SwitchProperty, SwitchElement>
		(xmlReader, attributes.name, attributes.device, property, T_DEF_SWITCH_VECTOR, T_DEF_SWITCH);
}

void IndiClient::readLightPropertyDefintion(QXmlStreamReader& xmlReader)
{
	BasicDefTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle timeout, etc.

	LightProperty* property = new LightProperty(attributes);
	readPropertyElementsDefinitions<LightProperty, LightElement>
		(xmlReader, attributes.name, attributes.device, property, T_DEF_LIGHT_VECTOR, T_DEF_LIGHT);
}

void IndiClient::readBlobPropertyDefinition(QXmlStreamReader& xmlReader)
{
	StandardDefTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle timeout, etc.

	BlobProperty* property = new BlobProperty (attributes);
	readPropertyElementsDefinitions<BlobProperty, BlobElement>
		(xmlReader, attributes.name, attributes.device, property, T_DEF_BLOB_VECTOR, T_DEF_BLOB);
}

QString IndiClient::readElementRawValue(QXmlStreamReader& xmlReader,
                                        const QString& tagName)
{
	QString value;
	Q_UNUSED(tagName);
	/*while(!(xmlReader.name() == tagName && xmlReader.isEndElement()))
	{
		if (xmlReader.isCharacters())
		{
			value = xmlReader.text().toString().trimmed();
			if (!value.isEmpty())
				break;
		}
		xmlReader.readNext();
	}*/
	//qDebug() << "readCharacters";
	value = xmlReader.readElementText().trimmed();
	return value;
}

void IndiClient::readNumberProperty(QXmlStreamReader& xmlReader)
{
	SetTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle state, timeout
	
	Property* property = getProperty(attributes);
	NumberProperty* numberProperty = dynamic_cast<NumberProperty*>(property);
	if (numberProperty == 0)//TODO: What does it return exactly if the cast fails?
	{
		qDebug() << "Not a number property:" << attributes.name;
		xmlReader.skipCurrentElement();
		return;
	}

	QHash<QString,QString> values;
	while (!(xmlReader.name() == T_SET_NUMBER_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_NUMBER)
		{
			readOneElement(xmlReader, T_ONE_NUMBER, values);
		}
		xmlReader.readNext();
	}
	//TODO: Is this a valid algorithm? Probably not... FIXME!
	if (!values.isEmpty())
	{
		if (!attributes.stateChanged)
			numberProperty->update(values, attributes.timestamp);
		else
			numberProperty->update(values, attributes.timestamp, attributes.state);
		emit propertyUpdated(clientId, attributes.device, numberProperty);
	}
}

void IndiClient::readOneElement(QXmlStreamReader& xmlReader,
                                const QString& tag,
                                QHash<QString, QString>& newValues)
{
	QString name = xmlReader.attributes().value(TagAttributes::NAME).toString();
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

void IndiClient::readBlobElement(QXmlStreamReader& xmlReader,
                                 BlobProperty* blobProperty)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString name = attributes.value(Element::A_NAME).toString();
	QString size = attributes.value(Element::A_SIZE).toString();
	QString format = attributes.value(Element::A_FORMAT).toString();
	if (name.isEmpty() ||
	    size.isEmpty() ||
	    format.isEmpty())
	{
		xmlReader.skipCurrentElement();
		return;
	}

	if (blobProperty->getElementNames().contains(name))
	{
		qDebug() << "Callin readElementRawValue()";
		QString value = readElementRawValue(xmlReader, T_ONE_BLOB);
		qDebug() << name << size << format;
		//qDebug() << value;
		if (!value.isEmpty())
		{
			qDebug() << "setValue called";
			blobProperty->getElement(name)->setValue(size, format, value);
		}
	}
}

void IndiClient::readSwitchProperty(QXmlStreamReader& xmlReader)
{
	SetTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle state, timeout
	
	Property* property = getProperty(attributes);
	SwitchProperty* switchProperty = dynamic_cast<SwitchProperty*>(property);
	if (switchProperty == 0)
	{
		qDebug() << "Not a SwitchProperty" << attributes.name;
		xmlReader.skipCurrentElement();
		return;
	}

	QHash<QString,QString> values;
	while(!(xmlReader.name() == T_SET_SWITCH_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_SWITCH)
		{
			readOneElement(xmlReader, T_ONE_SWITCH, values);
		}
		xmlReader.readNext();
	}
	if (!values.isEmpty())
	{
		if (!attributes.stateChanged)
			switchProperty->update(values, attributes.timestamp);
		else
			switchProperty->update(values, attributes.timestamp, attributes.state);
		emit propertyUpdated(clientId, attributes.device, switchProperty);
	}
}

void IndiClient::readBlobProperty(QXmlStreamReader& xmlReader)
{
	qDebug() << "readBlobProperty()";
	//TODO: Temporary implementation, ignore the grossness.
	SetTagAttributes attributes(xmlReader);
	if (!attributes.areValid)
	{
		xmlReader.skipCurrentElement();
		return;
	}
	handleMessageAttribute(attributes);
	//TODO: Handle state, timeout
	
	Property* property = getProperty(attributes);
	BlobProperty* blobProperty = dynamic_cast<BlobProperty*>(property);
	if (blobProperty == 0)
	{
		xmlReader.skipCurrentElement();
		return;
	}

	qDebug() << "Trying to read elements";
	while(!(xmlReader.name() == T_SET_BLOB_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == T_ONE_BLOB)
		{
			qDebug() << "Trying to read an element...";
			readBlobElement(xmlReader, blobProperty);
		}
		qDebug() << xmlReader.tokenString();
		xmlReader.readNext();
	}

	if (!attributes.stateChanged)
		blobProperty->update(attributes.timestamp);
	else
		blobProperty->update(attributes.timestamp, attributes.state);
	qDebug() << "end of readBlobProperty";
	emit propertyUpdated(clientId, attributes.device, blobProperty);
}

void IndiClient::writeGetProperties(const QString& device,
                                    const QString& property)
{
	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isWritable())
		return;

	QXmlStreamWriter xmlWriter(ioDevice);
	xmlWriter.writeStartDocument();
	xmlWriter.writeEmptyElement(T_GET_PROPERTIES);
	//TODO: Centralized storage of supported version number
	xmlWriter.writeAttribute(TagAttributes::VERSION, "1.7");
	if (!device.isEmpty())
		xmlWriter.writeAttribute(TagAttributes::DEVICE, device);
	if (!property.isEmpty())
		xmlWriter.writeAttribute(TagAttributes::NAME, property);
	xmlWriter.writeEndDocument();
}

void IndiClient::writeEnableBlob(SendBlobs sendBlobs,
                                 const QString& device,
                                 const QString& property)
{
	if (ioDevice == 0 ||
		!ioDevice->isOpen() ||
		!ioDevice->isWritable())
		return;

	if (device.isEmpty())
		return;

	QXmlStreamWriter xmlWriter(ioDevice);
	xmlWriter.writeStartDocument();
	xmlWriter.writeStartElement(T_ENABLE_BLOB);
	xmlWriter.writeAttribute(TagAttributes::DEVICE, device);
	if (!property.isEmpty())
		xmlWriter.writeAttribute(TagAttributes::NAME, property);
	QString characters;
	switch (sendBlobs)
	{
		case AlsoSendBlobs:
			characters = "Also";
			break;
		case OnlySendBlobs:
			characters = "Only";
			break;
		default:
		case NeverSendBlobs:
			characters = "Never";
			break;
	}
	xmlWriter.writeCharacters(characters);
	xmlWriter.writeEndElement();
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
	xmlWriter.writeAttribute(TagAttributes::DEVICE, device);
	xmlWriter.writeAttribute(TagAttributes::NAME, property->getName());
	//TODO: Add timestamp?

	QStringList elements = property->getElementNames();//TODO: Optimize this
	foreach (const QString& element, elements)
	{
		xmlWriter.writeStartElement(T_ONE_NUMBER);
		xmlWriter.writeAttribute(TagAttributes::NAME, element);
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
	xmlWriter.writeAttribute(TagAttributes::DEVICE, device);
	xmlWriter.writeAttribute(TagAttributes::NAME, property->getName());
	//TODO: Add timestamp?

	QStringList elements = property->getElementNames();//TODO: Optimize this
	foreach (const QString& element, elements)
	{
		xmlWriter.writeStartElement(T_ONE_SWITCH);
		xmlWriter.writeAttribute(TagAttributes::NAME, element);
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
