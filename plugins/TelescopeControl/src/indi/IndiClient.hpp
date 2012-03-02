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

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QSignalMapper>
#include <QString>
#include <QTextStream>
#include <QVariant>
#include <QXmlStreamReader>

#include "IndiTypes.hpp"
#include "IndiElement.hpp"
#include "IndiProperty.hpp"

//! Class implementing a client for the INDI wire protocol.
//! Properties are stored internally. Qt signals are emitted when a property
//! is changed or defined.
//! A single instance of IndiClient represents one connection.
//! \todo Device snooping!
//! \todo Split the Indi.hpp file. (Partially DONE.)
//! \todo Use shared pointers.
/*! Blueprint for device snooping if necessary on the client side:
Tree hashes (or maps?):
  reportpropertyto: if contains device/property, report to value
  reportallpropertiesto: if contains device, report to value
  reportalldevices (list?): if contains device, report to that device?
  
  Alternative: make device/driver an object that handles the incoming XML and emits property defined/deleted signals. Device objects are tied to device widgets, Property objects - to property widgets, snooping is done mainly with signals. (Works well for property-to-device snooping? Device-to-device will require signals between the Device object and its Property objects. All-to-device... :()
  
  All of that requires the properties to be parsed, validated and then written again, which is slow. Another option: send stuff to a QXmlStreamWriter during the parsing.
  */
class IndiClient : public QObject
{
	Q_OBJECT

public:
	IndiClient(const QString& newClientId,
	           QIODevice* openIoDevice = 0,
	           QObject* parent = 0);
	~IndiClient();

	QString getId() const;

	bool isConnected() const;

	//! \todo A temporary function to fill the gap.
	void sendRawCommand(const QString& command);

	//! Loads drivers.xml
	//! \returns a hash with keys device names, values driver names.
	static QHash<QString, QString> loadDeviceDescriptions();

	static const int DEFAULT_INDI_TCP_PORT = 7624;

	//INDI XML tags
	static const char* T_GET_PROPERTIES;
	static const char* T_DEL_PROPERTY;
	static const char* T_ENABLE_BLOB;
	static const char* T_MESSAGE;

	//INDI standard properties
	//TODO: Move to the telescope client?
	//http://www.indilib.org/index.php?title=Standard_Properties
	static const char* SP_CONNECTION;
	static const char* SP_CONNECT;
	static const char* SP_DISCONNECT;
	static const char* SP_J2000_COORDINATES;
	static const char* SP_JNOW_COORDINATES;
	static const char* SP_J2000_COORDINATES_REQUEST;
	static const char* SP_JNOW_COORDINATES_REQUEST;

public slots:
	//! Sends a \b getProperties element to the specified device.
	//! This asks the other side of the connection to send definitions of its
	//! properties. If no device name is specified, the command applies to all
	//! devices connected via this connection. If no property name is specified,
	//! the command applies to all properties.
	void writeGetProperties(const QString& device = QString(),
	                        const QString& property = QString());
	//! Sends a \b newXVector element to the specified device.
	void writeProperty(const QString& device,
	                   const QString& property,
	                   const QVariantHash& newValues);
	//!
	void writeEnableBlob(SendBlobs sendBlobs,
	                     const QString& device,
	                     const QString& property = QString());

signals:
	//! Emitted when a \b def[type]Vector element has been parsed.
	void propertyDefined(const QString& clientId,
	                     const QString& deviceName,
	                     const PropertyP& property);
	//! Emitted when a \b new[type]Vector element has been parsed.
	//! Such a message is sent by the device to notify that a given property
	//! or its attributes have changed and have a \b new value.
	void propertyUpdated(const QString& clientId,
	                     const QString& deviceName,
	                     const PropertyP& property);
	//! Emitted when a \b delProperty message has been parsed.
	void propertyRemoved(const QString& clientId,
	                     const QString& deviceName,
	                     const QString& propertyName);
	//! Emitted when a property definition or update contains a message, or when
	//! a \b message element has been received.
	//! \param timestamp uses QDateTime for now. It supports millisecond
	//! precision. If higher precision is necessary, an IndiTimestamp class may
	//! be necessary.
	void messageReceived(const QString& deviceName, const QDateTime& timestamp, const QString& message);

private slots:
	void parseStreamData();
	//! Example log message handling function.
	void logMessage(const QString& deviceName,
	                const QDateTime& timestamp,
	                const QString& message);

private:
	//! Identifier of the INDI client represented by the current object.
	QString clientId;

	//! May be a QProcess or a QTcpSocket.
	QIODevice* ioDevice;
	
	//! Represents all the named properties of a single device.
	typedef QHash<QString,PropertyP> DeviceProperties;
	//! The properties of all named devices.
	QHash<QString,DeviceProperties> deviceProperties;
	//! Returns the property matching the \b device and \b name attributes.
	//! \returns 0 if no such property exists.
	PropertyP getProperty(const SetTagAttributes& attributes);
	
	//! \todo Think of a better way to do these or make them static.
	QSet<QString> defVectorTags;
	QSet<QString> setVectorTags;
	QSet<QString> defElementTags;
	QSet<QString> oneElementTags;
	
	//! Tag name of the property/vector being parsed at the moment.
	//! Empty if no property is being parsed.
	//! Used to match start tag to end tag.
	QString currentPropertyTag;
	//! Tag name of the property element being parsed at the moment.
	//! Empty if no property element is being parsed.
	//! Used to match start tag to end tag.
	QString currentElementTag;
	//! The Property being parsed at the moment.
	//! 0 if no INDI vector is being parsed.
	PropertyP currentProperty;
	//! The Element being parsed at the moment.
	//! 0 if no Element is being parsed.
	Element* currentElement;
	//! This is getting ridiculous.
	SetTagAttributes* currentPropertyAttributes;
	//! Really ridiculous.
	QString currentElementName;
	//! Buffer for the value of the property element being read.
	QString currentElementValue;
	//! Used when defining property elements.
	QList<Element*> definedElements;
	//! Used when updating property values.
	QHash<QString,QString> elementsValues;
	//!
	bool definitionInProgress;
	//! 
	void resetParserState();
	
	//! Read a property vector definition and create one at currentProperty.
	//! No validation - make sure that currentProperty is null, etc.
	void readPropertyVectorDefinition(const QString& tag);
	//! Read a property element definition and create one at currentElement.
	//! No validation - make sure that currentElement is null, etc.
	void readPropertyElementDefinition(const QString& tag);
	//!
	void readPropertyVector(const QString& tag);
	//! 
	void readPropertyElement(const QString& tag);
	//...
	//! Read a message tag.
	void readMessage();
	
	//!
	bool hasProperty(const TagAttributes& attributes);

	//! \todo Better name...
	QXmlStreamReader xmlReader;

	//!
	QByteArray buffer;

	//! \obsolete
	//void parseIndiCommand(const QString& command);
	
	//! 

	//! Emits messageReceived() if the \b message attribute is not empty.
	void handleMessageAttribute(const TagAttributes& attributes);
	//!
	/*
	template<class P,class E> void readPropertyElementsDefinitions
		(QXmlStreamReader& xmlReader,
		 const QString& propertyName,
		 const QString& deviceName,
		 P* property,
		 const QString& propertyTagName,
		 const QString& elementTagName);
	
	//!
	QString	readElementRawValue(QXmlStreamReader& xmlReader, const QString& tag);
	//!
	void readTextProperty(QXmlStreamReader& xmlReader);
	//!
	void readNumberProperty(QXmlStreamReader& xmlReader);
	//!
	void readSwitchProperty(QXmlStreamReader& xmlReader);
	//!
	void readLightProperty(QXmlStreamReader& xmlReader);
	//!
	void readBlobProperty(QXmlStreamReader& xmlReader);
	*/
	//! Reads all \b oneX tags except \b onewBLOB.
	//! \param[in] tag
	//! \param[out] newValues
	void readOneElement(QXmlStreamReader& xmlReader,
	                    const QString& tag,
	                    QHash<QString,QString>& newValues);
	//! Reads a \b oneBLOB tag.
	void readBlobElement(QXmlStreamReader& xmlReader,
	                     const BlobPropertyP& blobProperty);

	//! Helper for writeProperty().
	//! Writes \b newTextVector element, including a series of \b oneText
	//! elements to the XML stream.
	//! \param newValues is expected to have QString values;
	//! \todo Needs to be actually implemented.
	void writeTextProperty(QXmlStreamWriter& xmlWriter,
	                       const QString& device,
	                       const TextPropertyP& property,
	                       const QVariantHash& newValues);
	//! Helper for writeProperty().
	//! Writes \b newNumberVector element, including a series of \b oneNumber
	//! elements to the XML stream.
	//! \param newValues is expected to have "double" values.
	//! \todo Support string values.
	void writeNumberProperty(QXmlStreamWriter& xmlWriter,
	                         const QString& device,
	                         const NumberPropertyP& property,
	                         const QVariantHash& newValues);
	//! Helper for writeProperty().
	//! Writes \b newSwitchVector element, including a series of \b oneSwitch
	//! elements to the XML stream.
	//! \param newValues is expected to have "bool" values.
	void writeSwitchProperty(QXmlStreamWriter& xmlWriter,
	                         const QString& device,
	                         const SwitchPropertyP& property,
	                         const QVariantHash& newValues);
	//! Helper for writeProperty().
	//! Writes \b newBLOBVector element, including a series of \b oneBLOB
	//! elements to the XML stream.
	//! \param newValues is expected to have QByteArray values.
	//! \todo Needs to be actually implemented.
	void writeBlobProperty(QXmlStreamWriter& xmlWriter,
	                       const QString& device,
	                       const BlobPropertyP& property,
	                       const QVariantHash& newValues);
};

#endif //_INDI_CLIENT_HPP_
