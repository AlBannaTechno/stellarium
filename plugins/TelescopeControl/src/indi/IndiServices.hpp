/* 
Qt-based INDI protocol client
Copyright (C) 2012  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef INDISERVICES_HPP
#define INDISERVICES_HPP

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTcpSocket>

class IndiClient;
class QSignalMapper;
class QStandardItemModel;

//! Provides various functions related to using the INDI library (libindi).
//! This includes loading the list of installed drivers, managing network
//! connections and maintaining a common instance of indiserver in a separate
//! process, including sending commands to start and stop named device drivers.
//! It is strongly recommended to use only a single instance of this class.
//! An IndiClient is initialized for each connection (including the one to the
//! local indiserver) and it must be shared by all device clients relying on
//! that connection.
//! \author Bogdan Marinov
//! \todo Handle remote connection errors.
//! \todo Decide whether to close remote connections normally or with abort().
class IndiServices : public QObject
{
	Q_OBJECT
public:
	explicit IndiServices(QObject *parent = 0);
	~IndiServices();
	
	//! Loads driver info from the files in the driver description directory.
	//! Parses all *.xml files in the directory. The result is aggregated in
	//! a tree-table model, where the first column contains the name of the
	//! device groups (e.g. "Telescopes"). Their children are tables, where
	//! column 0 is the device model name (e.g. "ETX125") and column 1 displays
	//! the driver label ("LX200 Autostar") with the name of the driver
	//! executable ("indi_lx200autostar") in the field.
	//! \todo Setting the path to the directory (for Windows, etc.)
	static QStandardItemModel* loadDriverDescriptionFiles();
	
	void loadDriverDescriptions();
	QStandardItemModel* getDriverDescriptions();
	
	
	//! Tells indiserver to start the driver.
	//! If indiserver is not running, starts it with the appropriate args.
	bool startDriver (const QString& driverName, const QString& deviceName);
	
	//! Tells indiserver to stop a driver.
	//! \todo Stop the server after the last driver is stopped?
	void stopDriver (const QString& driverName, const QString& deviceName);
	
	//! Starts an instance of indiserver if it's not already running.
	//! \todo Log and verbosity level.
	//! \todo Port number.
	//! \todo Find a way to check if the process is still running/when an error occurs.
	//! \returns true on sucess or if the server is already running.
	bool startServer();
	
	//! Stops the indiserver process.
	//! \returns true on success or if the server doesn't run.
	//! \todo Disconnect/destroy the socket, server, etc.
	bool stopServer();
	
	//! Get the common client connected to the local instance of indiserver.
	//! \returns 0 (null pointer) if the common client is not initialized.
	IndiClient* getCommonClient();
	//! Get the client parsing the connection with that ID.
	//! \returns 0 if there is no such client.
	IndiClient* getClient(const QString& id);
	//!
	QHashIterator<QString,IndiClient*> getClientIterator();
	
	//! Try connecting to another instance of indiserver over a TCP connection.
	//! \param id the identifier will be re-used for the client created for this
	//! connection.
	//! \param host host name, may be an IP address.
	//! \param port TCP port number.
	void openConnection(const QString& id, const QString& host, quint16 port);
	
	//! 
	void closeConnection(const QString& id);
	
signals:
	//! Emitted when the client linked to the common indiserver process has
	//! been initialized.
	void commonClientConnected(IndiClient* client);
	//!
	void clientConnected(IndiClient* client);
	
public slots:
	
	
private slots:
	//! Called when a TCP connection has been established to the local server.
	void initCommonClient();
	//! Create an IndiClient for the TCP connection with the given ID.
	//! Called when a TCP connection has been established.
	//! Emits clientConnected().
	void initClient(const QString& id);
	
	//! \todo Make this handle all connection errors, not only the local one.
	void handleConnectionError(QAbstractSocket::SocketError error);
	void handleProcessError(QProcess::ProcessError error);
	
	//! Called when one of the sockets disconnects.
	void destroySocket();
	
private:
	//! \todo Temporary.
	QStandardItemModel* deviceDescriptions;
	
	//! Common process instance of indiserver.
	QProcess* serverProcess;
	
	//! Path to the command pipe/fifo used to communicate with indiserver.
	QString commandPipePath;
	
	//! TCP connection to the server.
	QTcpSocket* serverSocket;
	
	//! 
	IndiClient* commonClient;
	
	//! Everything except the common client.
	QHash<QString, IndiClient*> socketClients;
	
	//! 
	QHash<QString,QTcpSocket*> sockets;
	//! 
	QSignalMapper* connectedMapper;
};

#endif // INDISERVICES_HPP
