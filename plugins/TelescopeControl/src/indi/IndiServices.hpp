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
/** Services include:
  *  - loading a list of the available XML descriptions of INDI drivers;
  *  - receiving INDI streams over network connections;
  *  - an instance of \c indiserver in a separate process that can run drivers;
  *  - dynamically starting/stopping drivers via that instance.
  *
  * It is strongly recommended to use only a single instance of this class.
  *
  * To load the device description files, use loadDriverDescriptions()
  * or the static loadDriverDescriptionFiles(). In the first case,
  * use getDriverDescriptions() to access the stored descriptions.
  *
  * To establish a network connection, use openConnection(). If it connects,
  * a new IndiClient instance will be created for that connection and
  * clientConnected() will be emitted. Use closeConnection() to close a
  * connection.
  *
  * Local drivers are managed by a single common instance of \c indiserver per
  * IndiServices object, with a single common IndiClient connected to it.
  * The server process is launched with startServer(). When connection to the
  * server is established, commonClientConnected() is emitted with a pointer
  * to the common client and you can then use startDriver() and stopDriver()
  * to launch and stop driver sub-processes. Output on the server's \c stderr
  * is read line by line and emitted as messages with commonServerLog().
  *
  * The IndiClient that is initialized for each connection (including the one
  * to the local indiserver) must be shared by all device clients
  * relying on that connection.
  * @author Bogdan Marinov
  * @ingroup indi
  * @todo Close the server process on stopping all drivers?
  * @todo Keep track of TCP ports to prevent conflicts?
  * @todo Handle server process closing.
  * @todo Handle remote connection errors.
  * @todo Decide whether to close remote connections normally or with abort().
  * @todo Manage the list of connected clients - remove clients on disconnect.
  */
class IndiServices : public QObject
{
	Q_OBJECT
public:
	explicit IndiServices(QObject *parent = 0);
	~IndiServices();
	
	//! Loads driver info from the files in the driver description directory.
	/** Parses all *.xml files in the directory. The result is aggregated in
	  * a tree-table model, where the first column contains the name of the
	  * device groups (e.g. "Telescopes"). Their children are tables, where
	  * column 0 is the device model name (e.g. "ETX125") and column 1 displays
	  * the driver label ("LX200 Autostar") with the name of the driver
	  * executable ("indi_lx200autostar") in the field.
	  * @param dirPath path to the directory from which to read the files.
	  */
	static QStandardItemModel*
	loadDriverDescriptionFiles(const QString& dirPath);
	
	//! Loads the available driver descriptions and stores the result.
	void loadDriverDescriptions();
	//! Access to the stored driver descriptions.
	QStandardItemModel* getDriverDescriptions();
	
	
	//! Tells indiserver to start the driver.
	//! If indiserver is not running, starts it with the appropriate args.
	bool startDriver (const QString& driverName, const QString& deviceName);
	
	//! Tells indiserver to stop a driver.
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
	//! @todo Is this necessary?
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
	
	//!
	void commonServerLog(const QString& logMessage);
	
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
	//! 
	void handleProcessError(QProcess::ProcessError error);
	
	//! Read the common server's stderr.
	void readServerErrorStream();
	
	//! Called when one of the sockets disconnects.
	void destroySocket();
	
private:
	//! Model with device descriptions as read from drivers.xml, etc.
	QStandardItemModel* deviceDescriptions;
	
	//! Common process instance of indiserver.
	QProcess* serverProcess;
	
	//! Path to the command pipe/fifo used to communicate with indiserver.
	QString commandPipePath;
	
	//! TCP connection to the server.
	QTcpSocket* serverSocket;
	
	//! Client connected to the common instance of indiserver.
	IndiClient* commonClient;
	
	//! Everything except the common client.
	QHash<QString, IndiClient*> socketClients;
	
	//! Network sockets used for clients.
	QHash<QString,QTcpSocket*> sockets;
	//! Used to call initClient() when a network connection is established.
	QSignalMapper* connectedMapper;
	
	static const int BUFFER_SIZE = 255;
	char buffer[BUFFER_SIZE];
};

#endif // INDISERVICES_HPP
