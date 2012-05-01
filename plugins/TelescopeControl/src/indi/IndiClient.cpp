/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010, 2012 Bogdan Marinov
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
#include <QStringList>
#include <QXmlStreamWriter>
#include <stdexcept>

const char* IndiClient::T_GET_PROPERTIES = "getProperties";
const char* IndiClient::T_DEL_PROPERTY = "delProperty";
const char* IndiClient::T_ENABLE_BLOB = "enableBLOB";
const char* IndiClient::T_MESSAGE = "message";

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
	ioDevice(0),
	currentProperty(0),
	currentElement(0),
	currentBlobElement(0),
	currentPropertyAttributes(0)
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
	        this, SLOT(parseStreamData()));

	//Make the parser think it's parsing parts of a large document
	//(otherwise it thinks that the first tag in the message is the root one)
	//"Extra content at end of document."
	//TODO: Think of a better way?
	xmlReader.addData("<indi>");
	
	defVectorTags << Property::T_DEF_TEXT_VECTOR
	              << Property::T_DEF_NUMBER_VECTOR
	              << Property::T_DEF_SWITCH_VECTOR
	              << Property::T_DEF_LIGHT_VECTOR
	              << Property::T_DEF_BLOB_VECTOR;
	setVectorTags << Property::T_SET_TEXT_VECTOR
	              << Property::T_SET_NUMBER_VECTOR
	              << Property::T_SET_SWITCH_VECTOR
	              << Property::T_SET_LIGHT_VECTOR 
	              << Property::T_SET_BLOB_VECTOR;
	defElementTags << Property::T_DEF_TEXT << Property::T_DEF_NUMBER
	               << Property::T_DEF_SWITCH << Property::T_DEF_LIGHT
	               << Property::T_DEF_BLOB;
	oneElementTags << Property::T_ONE_TEXT << Property::T_ONE_NUMBER
	               << Property::T_ONE_SWITCH << Property::T_ONE_LIGHT
	               << Property::T_ONE_BLOB;
}

IndiClient::~IndiClient()
{

	if (ioDevice)
	{
		disconnect(ioDevice, SIGNAL(readyRead()),
		           this, SLOT(parseStreamData()));
	}
	
	// Clean the temp objects
	resetParserState();
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

DeviceP IndiClient::getDevice(const QString& deviceName)
{
	if (deviceName.isEmpty())
		return DeviceP();
	
	return devices.value(deviceName);
}

void IndiClient::parseStreamData()
{
	if (!ioDevice)
		return;
	//qDebug() << "Starting to parse chunk...";

	//Get rid of "XML declaration not at start of document." errors
	//(Damn INDI and badly formed code!)
	//const QRegExp xmlDeclaration("<\\?[^>]+>");
	//buffer.remove(xmlDeclaration);
	//xmlReader.addData(buffer);

	// TODO: Initialize the QXmlStreamReader only once...
	/* QByteArray newPortion = ioDevice->readAll();
	//qDebug() << newPortion;
	buffer.append(newPortion);
	QXmlStreamReader localXmlReader(buffer);
	qint64 startPos = 0, offset = 0;

	while (!localXmlReader.atEnd())
	{
		localXmlReader.readNext();
		//qDebug() << xmlReader.tokenString();
		//TODO: Is this check necessary? Will it work?
		if (localXmlReader.isEndDocument())
		{
			qDebug() << "EOF reached?" << buffer << localXmlReader.name();
			buffer.clear();
			break;
		}
		else if (localXmlReader.tokenType() == QXmlStreamReader::Invalid)
		{
			QXmlStreamReader::Error errorCode = localXmlReader.error();
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
			         && localXmlReader.errorString() == "Extra content at end of document.")
			{
				QString command = buffer.mid(startPos, offset-startPos);
				parseIndiCommand(command);
				//qDebug() << "Command:" << command;
				buffer = buffer.mid(offset);
				localXmlReader.clear();
				localXmlReader.addData(buffer);
				offset = 0;
				startPos = 0;
			}
			else
			{
				qDebug() << errorCode << localXmlReader.errorString();
				localXmlReader.clear();//Is this necessary?
				break;
			}
		}
		else if (localXmlReader.tokenType() == QXmlStreamReader::NoToken)
		{
			break;
		}
		else if (localXmlReader.isWhitespace())
			continue;

		offset = localXmlReader.characterOffset();
	}
	qDebug() << "handleIncomingCommands() ends.";
	*/
	
	//========================================================================
	// New parser code follows.
	
	xmlReader.addData(ioDevice->readAll());
	
	while (!xmlReader.atEnd())
	{
		xmlReader.readNext();
		
		//TODO: The order can be changed to optimize speed
		if (xmlReader.isStartElement())
		{
			QString tag = xmlReader.name().toString();
			
			//TODO: Again, the order here can be optimized by frequency
			if (defVectorTags.contains(tag))
			{
//				clear current property
//				clear current element
//				clear element values
//				clear blob value
//				create new property
//				make it current property
				resetParserState();
				readPropertyVectorDefinition(tag);
				definitionInProgress = true;
			}
			else if (defElementTags.contains(tag))
			{
				if (currentProperty)
				{
					if (currentElement || !definitionInProgress ||
					    tag != currentProperty->defElementTag())
					{
						qDebug() << "Bad INDI structure - no closing tag for "
						            "the previous element or mismatched "
						            "element type.";
						resetParserState();
						continue;
					}
					
					readPropertyElementDefinition(tag);
					currentElementValue.clear();
				}
			}
			else if (setVectorTags.contains(tag))
			{
//				if anything else is being read
//					clear current property
//					clear current element
//					clear element values
//				if property is in the list
//					make current property
//					(what about the state, timestamp, etc?)
				resetParserState();
				readPropertyVector(tag);
				definitionInProgress = false;
			}
			else if (oneElementTags.contains(tag))
			{
				if (currentProperty)
				{
					if (currentElement || definitionInProgress ||
					    tag != currentProperty->oneElementTag())
					{
						resetParserState();
						continue;
					}
					
					readPropertyElement(tag);
					currentElementValue.clear();
				}
			}
			else if (tag == T_MESSAGE)
			{
				readMessage();
			}
			else if (tag == T_DEL_PROPERTY)
			{
				QXmlStreamAttributes attributes = xmlReader.attributes();
				QString deviceName = attributes.value(TagAttributes::DEVICE).toString();
				DeviceP device = devices.value(deviceName);
				if (device)
				{
					QString propName = attributes.value(TagAttributes::NAME).toString();
					if (propName.isEmpty())
					{
						// Remove the whole device.
						device->removeAllProperties();
						devices.remove(deviceName);
						device.clear();
						emit deviceRemoved(clientId, deviceName);
					}
					else
						device->removeProperty(propName);
					
					QDateTime timestamp = TagAttributes::readTimestampAttribute(attributes);
					QString message = attributes.value(TagAttributes::MESSAGE).toString();
					if (!message.isEmpty())
						emit messageReceived(deviceName, timestamp, message);
				}
			}
			else if (tag == T_GET_PROPERTIES)
			{
				//TODO
			}
		}
		else if (xmlReader.isEndElement())
		{
			QString tag = xmlReader.name().toString();
			
			if (defVectorTags.contains(tag))
			{
//				if current element
//					there has been a problem:
//					clear current element
//					clear current property
//				else if current property and it matches
//					if everything is OK
//						add property to property list
//						emit property defined
//						clear current property ptr
//					if anything is wrong
//						clear stuff
				
				if (currentProperty)
				{
					if (currentElement || tag != currentPropertyTag)
					{
						// TODO
						qDebug() << "Malformed INDI code.";
						resetParserState();
						continue;
					}
					
					foreach (Element* element, definedElements)
						currentProperty->addElement(element);
					definedElements.clear();
					
					if (currentProperty->elementCount() > 0)
					{
						const QString& deviceId = currentProperty->getDevice();
						DeviceP device = devices.value(deviceId);
						if (device.isNull())
						{
							device = DeviceP(new Device(deviceId));
							devices.insert(deviceId, device);
							emit deviceDefined(clientId, device);
							// TODO: Stupid!
							emit deviceNameDefined(clientId, deviceId);
						}
						bool added = device->addProperty(currentProperty);
						if (added)
						{
							connect(currentProperty.data(),
							        SIGNAL(valuesToSend(QByteArray)),
							        this,
							        SLOT(sendData(QByteArray)));
						}
						//emit propertyDefined(clientId, devId, currentProperty);
					}
					
					currentProperty.clear();
					currentPropertyTag.clear();
					definitionInProgress = false;
				}
			}
			else if (defElementTags.contains(tag)) //End tag
			{
				if (currentProperty && currentElement)
				{
					if (tag != currentElementTag)
					{
						//TODO
						resetParserState();
						continue;
					}
					
					// Even if the value is empty or invalid, add the element.
					currentElement->setValue(currentElementValue.trimmed());
					definedElements.append(currentElement);
					
					currentElement = 0;
					currentElementTag.clear();
					currentElementValue.clear();
				}
			}
			else if (setVectorTags.contains(tag)) //End tag
			{
//				if current property and it matches and nothing is waiting to be read
//					update it with element values
//					(emit property changed or leave it to the property object?)
//				else
//					clear stuff
				if (currentProperty)
				{
					if (currentElement || tag != currentPropertyTag)
					{
						resetParserState();
						continue;
					}
					
					handleMessageAttribute(*currentPropertyAttributes);
					currentProperty->update(elementsValues,
					                        *currentPropertyAttributes);
					currentProperty.clear();
					currentPropertyTag.clear();
					delete currentPropertyAttributes;
					currentPropertyAttributes = 0;
					elementsValues.clear();
					// TODO: Handle BLOBs
				}
			}
			else if (oneElementTags.contains(tag)) //End tag
			{
				if (currentProperty && !currentElementName.isEmpty())
				{
					if (tag != currentElementTag)
					{
						//TODO:
						resetParserState();
						continue;
					}
					
					if (currentBlobElement)
					{
						currentBlobElement->finishReceivingData();
						currentBlobElement = 0;
					}
					else
					if (!currentElementValue.isEmpty())
					{
						elementsValues.insert(currentElementName, currentElementValue.trimmed());
					}
					currentElementName.clear();
					currentElementTag.clear();
					currentElementValue.clear();
				}
			}
		}
		else if (xmlReader.isWhitespace())
		{
			// Whitespace makes both isWhitespace() and isCharacters()
			// to return true, so the order here should not be arbitrary.
			// TODO: Decide if skipping whitespaces makes any difference...
			continue;
		}
		else if (xmlReader.isCharacters())
		{
//			if (element value is being read)
			if (currentBlobElement)
				currentBlobElement->receiveData(xmlReader.text().toString());
			else
				currentElementValue.append(xmlReader.text());
		}
		
//		TODO:
//		- what is the difference between DTD's with "EMPTY" and those with nothing? Because of message and delProperty. See http://www.w3.org/TR/REC-xml/#dt-empty According to the examples, message is used empty.
		
	} //End while
	if (xmlReader.hasError())
	{
		if (xmlReader.error() == QXmlStreamReader::PrematureEndOfDocumentError)
		{
			//qDebug() << "End of pseudo-XML chunk in INDI client" << clientId;
		}
		else
		{
			qDebug() << "XML parser error in INDI client" << clientId 
			         << xmlReader.error() << xmlReader.errorString();
			// TODO: Mark as disconnected?
		}
	}
}

void IndiClient::resetParserState()
{
	definitionInProgress = false;
	currentPropertyTag.clear();
	currentElementTag.clear();
	currentElementName.clear();
	if (currentProperty)
	{
		currentProperty.clear();
	}
	if (currentElement)
	{
		delete currentElement;
		currentElement = 0;
	}
	if (currentBlobElement)
	{
		// Already belongs to a Property, so clear the pointer only.
		currentBlobElement = 0;
	}
	if (currentPropertyAttributes)
	{
		delete currentPropertyAttributes;
		currentPropertyAttributes = 0;
	}
	qDeleteAll(definedElements);
	definedElements.clear();
	elementsValues.clear();
}

void IndiClient::readPropertyVectorDefinition(const QString& tag)
{
	// This is a bit stupid, but I'm reusing the existing code.
	// May clean it up later.
	if (tag == Property::T_DEF_TEXT_VECTOR)
	{
		StandardDefTagAttributes attributes(xmlReader);
		if (attributes.areValid && !hasProperty(attributes))
		{
			handleMessageAttribute(attributes);
			currentProperty = TextPropertyP(new TextProperty(attributes));
			currentPropertyTag = tag;
		}
	}
	else if (tag == Property::T_DEF_NUMBER_VECTOR)
	{
		StandardDefTagAttributes attributes(xmlReader);
		if (attributes.areValid && !hasProperty(attributes))
		{
			handleMessageAttribute(attributes);
			currentProperty = NumberPropertyP(new NumberProperty(attributes));
			currentPropertyTag = tag;
		}
	}
	else if (tag == Property::T_DEF_SWITCH_VECTOR)
	{
		DefSwitchTagAttributes attributes(xmlReader);
		if (attributes.areValid && !hasProperty(attributes))
		{
			handleMessageAttribute(attributes);
			currentProperty = SwitchPropertyP(new SwitchProperty(attributes));
			currentPropertyTag = tag;
		}
	}
	else if (tag == Property::T_DEF_LIGHT_VECTOR)
	{
		BasicDefTagAttributes attributes(xmlReader);
		if (attributes.areValid && !hasProperty(attributes))
		{
			handleMessageAttribute(attributes);
			currentProperty = LightPropertyP(new LightProperty(attributes));
			currentPropertyTag = tag;
		}
	}
	else if (tag == Property::T_DEF_BLOB_VECTOR)
	{
		StandardDefTagAttributes attributes(xmlReader);
		if (attributes.areValid && !hasProperty(attributes))
		{
			handleMessageAttribute(attributes);
			currentProperty = BlobPropertyP(new BlobProperty(attributes));
			currentPropertyTag = tag;
		}
	}
}

void IndiClient::readPropertyElementDefinition(const QString& tag)
{
	Element* element = 0;
	QXmlStreamAttributes attributes = xmlReader.attributes();
	if (tag == Property::T_DEF_TEXT)
		element = new TextElement(attributes);
	else if (tag == Property::T_DEF_NUMBER)
		element = new NumberElement(attributes);
	else if (tag == Property::T_DEF_SWITCH)
		element = new SwitchElement(attributes);
	else if (tag == Property::T_DEF_LIGHT)
		element = new LightElement(attributes);
	else if (tag == Property::T_DEF_BLOB)
		element = new BlobElement(attributes);
	
	if (element->isValid())
	{
		currentElement = element;
		currentElementTag = tag;
	}
	else
		delete element;
}

void IndiClient::readPropertyVector(const QString& tag)
{
	SetTagAttributes* attributes = new SetTagAttributes(xmlReader);
	if (!attributes->areValid)
		return;
	
	PropertyP property = getProperty(*attributes);
	if (property)
	{
		currentProperty = property;
		currentPropertyTag = tag;
		currentPropertyAttributes = attributes;
	}
	else
		delete attributes;
	
	//TODO: Handle BLOBs
}

void IndiClient::readPropertyElement(const QString& tag)
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString name = attributes.value(TagAttributes::NAME).toString();
	if (name.isEmpty())
		return;
	
	if (currentProperty->getType() == Property::BlobProperty)
	{
		QString size = attributes.value(Element::A_SIZE).toString();
		QString format = attributes.value(Element::A_FORMAT).toString();
		if (size.isEmpty() ||
		    format.isEmpty())
		{
			qWarning() << "Missing BLOB size or format for"
			           << currentProperty->getName() << name;
			return;
		}
		
		currentBlobElement = (qSharedPointerDynamicCast<BlobProperty>(currentProperty))->getElement(name);
		if (currentBlobElement)
		{
			currentBlobElement->prepareToReceiveData(size, format);
		}
	}
	
	currentElementTag = tag;
	currentElementName = name;
}

//void IndiClient::parseIndiCommand(const QString& command)
//{
//	// TODO: Use only one XML parser!
//	QXmlStreamReader localXmlReader(command);

//	// If we are going to use two, let's at least assume that this is well-formed XML...
//	while (localXmlReader.readNextStartElement())
//	{
//		if (localXmlReader.name() == Property::T_SET_BLOB_VECTOR)
//		{
//			readBlobProperty(localXmlReader);
//		}
////		else if (localXmlReader.name() == T_MESSAGE)
////		{
////			readMessage(localXmlReader);
////		}
//		//TODO: To be continued...
//		else
//		{
//			localXmlReader.skipCurrentElement();
//		}
//	}
//}

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

PropertyP IndiClient::getProperty(const SetTagAttributes& attributes)
{
	const QString& deviceName = attributes.device;
	DeviceP device = devices.value(deviceName);
	if (device.isNull())
	{
		qDebug() << "Unknown device name:" << deviceName;
		return PropertyP();
	}
	const QString& name = attributes.name;
	PropertyP property = device->getProperty(name);
	if (property.isNull())
	{
		qDebug() << "Unknown property name" << name
		         << "for device" << deviceName;
		return PropertyP();
	}
	return property;
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

void IndiClient::readMessage()
{
	QXmlStreamAttributes attributes = xmlReader.attributes();
	QString device = attributes.value(TagAttributes::DEVICE).toString();
	QDateTime timestamp = TagAttributes::readTimestampAttribute(attributes);
	QString message = attributes.value(TagAttributes::MESSAGE).toString();
	if (!message.isEmpty())
		emit messageReceived(device, timestamp, message);
}

bool IndiClient::hasProperty(const TagAttributes& attributes)
{
	if (!attributes.areValid)
		return false;
	
	DeviceP device = devices.value(attributes.device);
	if (device)
	{
		if (device->hasProperty(attributes.name))
			return true;
	}
	return false;
}

//template<class P, class E>
//void IndiClient::readPropertyElementsDefinitions
//	(QXmlStreamReader& xmlReader,
//	 const QString& propertyName,
//	 const QString& device,
//	 P* property,
//	 const QString& propertyTag,
//	 const QString& elementTag)
//{
//	while (true)
//	{
//		xmlReader.readNext();
//		if (xmlReader.name() == propertyTag && xmlReader.isEndElement())
//			break;
//		else if (xmlReader.name() == elementTag)
//		{
//			QXmlStreamAttributes attributes = xmlReader.attributes();
//			E* element = new E(attributes);
//			if (!element->isValid())
//			{
//				delete element;
//				return;
//			}

//			QString value = readElementRawValue(xmlReader, elementTag);
//			if (value.isEmpty() && elementTag != Property::T_DEF_BLOB)
//			{
//				return;
//			}

//			if (element->setValue(value))
//				property->addElement(element);
//			else
//				delete element;
//		}
//	}

//	[snip]
//}

/*
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
*/

//QString IndiClient::readElementRawValue(QXmlStreamReader& xmlReader,
//                                        const QString& tagName)
//{
//	QString value;
//	Q_UNUSED(tagName);
//	/*while(!(xmlReader.name() == tagName && xmlReader.isEndElement()))
//	{
//		if (xmlReader.isCharacters())
//		{
//			value = xmlReader.text().toString().trimmed();
//			if (!value.isEmpty())
//				break;
//		}
//		xmlReader.readNext();
//	}*/
//	//qDebug() << "readCharacters";
//	value = xmlReader.readElementText().trimmed();
//	return value;
//}

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
                                 const BlobPropertyP& blobProperty)
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
		// TODO!
		QString value;// = readElementRawValue(xmlReader, Property::T_ONE_BLOB);
		qDebug() << name << size << format;
		//qDebug() << value;
		if (!value.isEmpty())
		{
			qDebug() << "setValue called";
			BlobElement* element = blobProperty->getElement(name);
			element->prepareToReceiveData(size, format);
			element->receiveData(value);
			element->finishReceivingData();
		}
	}
}

/*
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
	
	PropertyP property = getProperty(attributes);
	BlobPropertyP blobProperty(property);
	if (blobProperty.isNull())
	{
		xmlReader.skipCurrentElement();
		return;
	}

	qDebug() << "Trying to read elements";
	while(!(xmlReader.name() == Property::T_SET_BLOB_VECTOR && xmlReader.isEndElement()))
	{
		if (xmlReader.name() == Property::T_ONE_BLOB)
		{
			qDebug() << "Trying to read an element...";
			readBlobElement(xmlReader, blobProperty);
		}
		qDebug() << xmlReader.tokenString();
		xmlReader.readNext();
	}

//	if (!attributes.stateChanged)
//		blobProperty->update(attributes.timestamp);
//	else
//		blobProperty->update(attributes.timestamp, attributes.state);
//	qDebug() << "end of readBlobProperty";
	emit propertyUpdated(clientId, attributes.device, blobProperty);
}
*/

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

void IndiClient::sendData(const QByteArray& indiText)
{
	if (ioDevice && ioDevice->isOpen() && ioDevice->isWritable())
	{
		qint64 bytesWritten = ioDevice->write(indiText);
		if (bytesWritten != indiText.length())
		{
			qDebug() << "INDI client" << clientId
			         << "There might be a problem";
		}
	}
	else
	{
		qWarning() << "INDI client" << clientId
		           << "can't write to device. Skipping:"
		           << indiText;
	}
}
