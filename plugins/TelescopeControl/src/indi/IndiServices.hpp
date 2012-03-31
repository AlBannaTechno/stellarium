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
class QStandardItemModel;

//! Provides various functions related to using libindi.
//! This includes loading the list of installed drivers, starting, stopping, and
//! sending commands to indiserver, etc.
//! Must be a singleton.
//! Maintains a common instance of indiserver and a common IndiClient for all
//! drivers running on this computer.
//! \author Bogdan Marinov
//! \todo Migrate all relevant functions from IndiClient.
//! \todo Add a common QTcpSocket for the indiserver instance.
//! \todo Put all remote INDI connections here? They can emit a signal when connected/client initialized.=> connect to the control panel. Also, some kind of object instantiating TelescopeClientIndi's when a client defines a slewable device (the plugin itself?)
class IndiServices : public QObject
{
	Q_OBJECT
public:
	explicit IndiServices(QObject *parent = 0);
	~IndiServices();
	
	//! Loads driver info from the INDI driver description directory.
	//! Parses all *.xml files in the directory. The result is aggregated in
	//! a tree-table model, where the first column contains the name of the
	//! device groups (e.g. "Telescopes"). Their children are tables, where
	//! column 0 is the device model name (e.g. "ETX125") and column 1 displays
	//! the driver label ("LX200 Autostar") with the name of the driver
	//! executable ("indi_lx200autostar") in the field.
	//! \todo Setting the path to the directory (for Windows, etc.)
	static QStandardItemModel* loadDriverDescriptions();
	
	
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
	
	IndiClient* getCommonClient();
	
signals:
	void commonClientConnected(IndiClient* client);
	
public slots:
	
	
private slots:
	void initCommonClient();
	
	void handleConnectionError(QAbstractSocket::SocketError error);
	void handleProcessError(QProcess::ProcessError error);
	
private:
	//! Common process instance of indiserver.
	QProcess* serverProcess;
	
	//! Path to the command pipe/fifo used to communicate with indiserver.
	QString commandPipePath;
	
	//! TCP connection to the server.
	QTcpSocket* serverSocket;
	
	//!
	IndiClient* commonClient;
};

#endif // INDISERVICES_HPP
