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
#include <QSignalMapper>
#include <QString>
#include <QTextStream>
#include <QXmlStreamReader>

#include "Indi.hpp"

//! Class implementing a client for the INDI wire protocol.
//! Properties are stored internally. Qt signals are emitted when a property
//! is changed or defined.
//! \todo Decide whether to use only one instance of IndiClient to handle all
//! INDI connections, or to use one copy per connection. (In the second case,
//! pointer to the QIODevice should be given to the constructor, not via a
//! separate method.
class IndiClient : public QObject
{
	Q_OBJECT

public:
	IndiClient(QObject* parent = 0);
	~IndiClient();

	//! Error handling on the device level should be done by the caller.
	//! IndiClient only handles the command streams.
	void addConnection(QIODevice* ioDevice);

	//! \todo A temporary function to fill the gap.
	void sendRawCommand(const QString& command);

	//! Loads drivers.xml
	//! \returns a hash with keys device names, values driver names.
	static QHash<QString, QString> loadDeviceDescriptions();

	static const int DEFAULT_INDI_TCP_PORT = 7624;

	//INDI XML tags
	static const char* T_GET_PROPERTIES;
	static const char* T_DEF_NUMBER_VECTOR;
	static const char* T_DEF_SWITCH_VECTOR;
	static const char* T_SET_NUMBER_VECTOR;
	static const char* T_SET_SWITCH_VECTOR;
	static const char* T_NEW_NUMBER_VECTOR;
	static const char* T_NEW_SWITCH_VECTOR;
	static const char* T_DEF_NUMBER;
	static const char* T_DEF_SWITCH;
	static const char* T_ONE_NUMBER;
	static const char* T_ONE_SWITCH;


	//INDI XML attributes
	static const char* A_VERSION;
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
	static const char* A_RULE;

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
	//! Sends a \b newTextVector element to the specified device.
	//! \todo Needs to be actually implemented.
	void writeTextProperty(const QString& device,
	                       const QString& property,
	                       const QHash<QString,QString>& newValues);
	//! Sends a \b newNumberVector element to the specified device.
	void writeNumberProperty(const QString& device,
	                         const QString& property,
	                         const QHash<QString,double>& newValues);
	//! Sends a \b newSwitchVector element to the specified device.
	void writeSwitchProperty(const QString& device,
	                         const QString& property,
	                         const QHash<QString,bool>& newValues);
	//! Sends a \b newBlobVector element to the specified device.
	//! \todo Needs to be actually implemented.
	void writeBlobProperty(const QString& device,
	                       const QString& property,
	                       const QHash<QString,QByteArray>& newValues);

signals:
	//! Emitted when a \b def[type]Vector element has been parsed.
	void propertyDefined(const QString& deviceName, Property* property);
	//! Emitted when a \b new[type]Vector element has been parsed.
	//! Such a message is sent by the device to notify that a given property
	//! or its attributes have changed and have a \b new value.
	void propertyUpdated(const QString& deviceName, Property* property);
	//! Emitted when a \b delProperty message has been parsed.
	void propertyRemoved(const QString& deviceName, const QString& propertyName);
	//! Emitted when a property definition or update contains a message, or when
	//! a \b message element has been received.
	//! \param timestamp uses QDateTime for now. It supports millisecond
	//! precision. If higher precision is necessary, an IndiTimestamp class may
	//! be necessary.
	void messageLogged(const QString& deviceName, QDateTime timestamp, const QString& message);

private slots:
	void handleIncomingCommands();

private:
	//!
	Permission readPermissionFromString(const QString& string);
	//!
	State readStateFromString(const QString& string);
	//!
	SwitchRule readSwitchRuleFromString(const QString& string);

	//!
	bool readPropertyAttributes(QString& device,
	                            QString& property,
	                            QString& label,
	                            QString& group,
	                            QString& state,
	                            QString& permission,
	                            QString& timeout,
	                            bool checkPermission);
	//!
	bool readPropertyAttributes(QString& device,
								QString& name,
								QString& state,
								QString& timeout);
	//!
	void readPropertyAttributesTimestampAndMessage(const QString& device, QDateTime& timestamp);
	//!
	void readNumberPropertyDefinition();
	//!
	void readNumberElementDefinition(NumberProperty* numberProperty);
	//!
	void readSwitchPropertyDefinition();
	//!
	void readSwitchElementDefinition(SwitchProperty* switchProperty);
	//!
	void readNumberProperty();
	//!
	void readNumberElement(NumberProperty* numberProperty);
	//!
	void readSwitchProperty();
	//!
	void readSwitchElement(SwitchProperty* switchProperty);


	//! May be a QProcess or a QTcpSocket.
	QIODevice* ioDevice;

	QTextStream* textStream;

	//! \todo Better name...
	QXmlStreamReader xmlReader;

	//! Represents all the named properties of a single device.
	typedef QHash<QString,Property*> DeviceProperties;
	//! The properties of all named devices.
	QHash<QString,DeviceProperties> deviceProperties;
};

#endif //_INDI_CLIENT_HPP_
