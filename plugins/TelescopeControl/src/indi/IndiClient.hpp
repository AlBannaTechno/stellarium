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

#include <QHash>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QXmlStreamReader>

#include "Indi.hpp"

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

	//! \todo A temporary function to fill the gap.
	void sendRawCommand(const QString& command);

	//! Loads drivers.xml
	//! \returns a hash with keys device names, values driver names.
	static QHash<QString, QString> loadDeviceDescriptions();

	static const int DEFAULT_INDI_TCP_PORT = 7624;

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
	//http://www.indilib.org/index.php?title=Standard_Properties
	static const char* SP_CONNECTION;
	static const char* SP_J2000_COORDINATES;
	static const char* SP_JNOW_COORDINATES;
	static const char* SP_J2000_COORDINATES_REQUEST;
	static const char* SP_JNOW_COORDINATES_REQUEST;

public slots:

signals:
	void propertyDefined(const QString& deviceName, Property* property);
	void propertyUpdated(const QString& deviceName, Property* property);
	void propertyRemoved(const QString& deviceName, const QString& propertyName);
	//! \todo determine parameters
	void messageLogged();

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
