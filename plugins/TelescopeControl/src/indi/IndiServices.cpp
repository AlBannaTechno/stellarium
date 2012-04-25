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

#include "IndiServices.hpp"
#include "IndiClient.hpp"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSignalMapper>
#include <QStandardItemModel>
#include <QXmlStreamReader>

#ifndef _WIN32
#include <sys/types.h> // for mkfifo()
#include <sys/stat.h>  // for mkfifo()
#include <stdio.h>     // for remove()
#endif


IndiServices::IndiServices(QObject *parent) :
    QObject(parent),
    serverProcess(0),
    serverSocket(0),
    commonClient(0)
{
	serverSocket = new QTcpSocket(this);
	
	connectedMapper = new QSignalMapper(this);
	connect(connectedMapper, SIGNAL(mapped(QString)),
	        this, SLOT(initClient(QString)));
}

IndiServices::~IndiServices()
{
	// TODO: Close socket?
	stopServer();
	
	// TODO: Destroy server?
	
	// TODO: Close all sockets?
}

QStandardItemModel* IndiServices::loadDriverDescriptions()
{
	QStandardItemModel* model = new QStandardItemModel();
	QStandardItem* parent = model->invisibleRootItem();
	QStandardItem* currentGroup = 0, *currentDevice = 0;

	//TODO: It should allow the file path to be set somewhere
	QDir indiDriversDir("/usr/share/indi/");
	indiDriversDir.setNameFilters(QStringList("*.xml"));
	indiDriversDir.setFilter(QDir::Files | QDir::Readable);
	QFileInfoList fileList = indiDriversDir.entryInfoList();
	
	foreach (const QFileInfo& fi, fileList)
	{
		QFile indiDriversXmlFile(fi.absoluteFilePath());
		if (indiDriversXmlFile.open(QFile::ReadOnly | QFile::Text))
		{
			qDebug() << "Parsing file" << fi.absoluteFilePath();
			QXmlStreamReader localXmlReader;
			localXmlReader.addData("<indi>");
			localXmlReader.addData(indiDriversXmlFile.readAll());
			
			QString deviceName;
			while (!localXmlReader.atEnd())
			{
				if (localXmlReader.hasError())
				{
					qDebug() << "Error parsing" << fi.absoluteFilePath()
					         << localXmlReader.errorString();
					break;
				}
				
				if (localXmlReader.isEndDocument())
					break;
				
				if (localXmlReader.isStartElement())
				{
					QXmlStreamAttributes attr = localXmlReader.attributes();
					if (localXmlReader.name() == "devGroup")
					{
						QString groupName = attr.value("group").toString();
						
						QList<QStandardItem*> existing = model->findItems(groupName);
						if (existing.isEmpty())
						{
							currentGroup = new QStandardItem(groupName);
							currentGroup->setSelectable(false);
							currentGroup->setEditable(false);
							parent->appendRow(currentGroup);
						}
						else
							currentGroup = existing.first();
					}
					else if (localXmlReader.name() == "device")
					{
						deviceName = attr.value("label").toString();
						if (deviceName.isEmpty())
						{
							localXmlReader.skipCurrentElement();
							continue;
						}
						
						if (currentGroup)
						{
							currentDevice = new QStandardItem(deviceName);
							currentDevice->setEditable(false);
							currentGroup->appendRow(currentDevice);
						}
						else
						{
							qDebug() << "Error parsing" << fi.absoluteFilePath();
							break;
						}
					}
					else if (localXmlReader.name() == "driver")
					{
						if (currentDevice)
						{
							QString driverName = attr.value("name").toString();
							
							// TODO: Separate driver name from driver exe
							QString driverFile = localXmlReader.readElementText().trimmed();
							if (driverFile.isEmpty())
							{
								localXmlReader.skipCurrentElement();
								continue;
							}
							
							if (driverName.isEmpty())
								driverName = driverFile;
							
							QStandardItem* driver = new QStandardItem(driverName);
							// TODO: This is because of selections in the combo box. Find a better way!s
							driver->setSelectable(false);
							driver->setData(driverFile, Qt::UserRole);
							driver->setToolTip(driverFile);
							// TODO: Find if it exists?
							int row = currentGroup->rowCount() - 1;
							currentGroup->setChild(row, 1, driver);
						}
						// TODO
					}
				}
				else if (localXmlReader.isEndElement())
				{
					// Ensure structure validation
					if (localXmlReader.name() == "devGroup")
						currentGroup = 0;
					else if (localXmlReader.name() == "device")
						currentDevice = 0;
				}
				
				localXmlReader.readNext();
			}
			
			indiDriversXmlFile.close();
		}
		else
		{
			qDebug() << "Unable to open:" << fi.absoluteFilePath();
		}
	}

	model->setHorizontalHeaderLabels(QStringList() << "Device" << "Driver");
	return model;
}

bool IndiServices::startServer()
{
	if (serverProcess)
		return true;

	// Open a named pipe to send commands to the server.
	// Platform specific.
#ifndef _WIN32
	commandPipePath = QDesktopServices::storageLocation(QDesktopServices::TempLocation);
	commandPipePath.append("/indi-pipe-");
	commandPipePath.append(QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmss"));
	qDebug() << commandPipePath;
	
	if (mkfifo(commandPipePath.toAscii(), 0660) < 0)
	{
		// TODO: Handle errno
		commandPipePath.clear();
		return false;
	}
#endif
	QStringList arguments;
	//arguments << "-l" << "~/"; // Log directory
	arguments << "-f" << commandPipePath;
	serverProcess = new QProcess();
	// Redirect stdout/stderr to Stellarium's
	// TODO: Redirect to Stellarium's log or somewhere else.
	serverProcess->setProcessChannelMode(QProcess::ForwardedChannels);
	serverProcess->start("indiserver", arguments);
	// TODO: Replace the wait with a slot?
	if (serverProcess->waitForStarted(3000))
	{
		return true;
	}
	else
	{
		serverProcess->close();
		serverProcess->deleteLater();
		serverProcess = 0;
		return false;
	}
}

bool IndiServices::stopServer()
{
	if (!serverProcess)
	{
		return true;
	}
	
	// Disconnect the TCP connection.
	if (serverSocket)
	{
		disconnect(serverSocket,
		           SIGNAL(error(QAbstractSocket::SocketError)),
		           this,
		           SLOT(handleConnectionError(QAbstractSocket::SocketError)));
		serverSocket->disconnectFromHost();
		serverSocket->deleteLater();
		serverSocket = 0;
	}
	
	serverProcess->terminate();
	if (serverProcess->waitForFinished())
	{
		serverProcess->deleteLater();
		serverProcess = 0;
		
#ifndef _WIN32
		if (!commandPipePath.isEmpty())
			remove(commandPipePath.toAscii());
#endif
		
		return true;
	}
	
	return false;
}

bool IndiServices::startDriver(const QString& driverName,
                               const QString& deviceName)
{
	if (driverName.isEmpty() ||
	    deviceName.isEmpty())
	{
		return false;
	}
	
	if (!serverProcess)
	{
		if (!startServer())
			return false;
	}
	
	QFile pipe(commandPipePath);
	if (pipe.open(QFile::WriteOnly))
	{
		QString command = QString("start %1 \"%2\"").arg(driverName, deviceName);
		qDebug() << command;
		pipe.write(qPrintable(command));
		pipe.flush();
		pipe.close();
		
		// TODO: Or move to a slot triggered by the server process or pipe?
		// TODO: Or check the client?
		if (!serverSocket->isOpen())
		{
			connect(serverSocket, SIGNAL(connected()),
			        this, SLOT(initCommonClient()));
			connect(serverSocket,
			        SIGNAL(error(QAbstractSocket::SocketError)),
			        this,
			        SLOT(handleConnectionError(QAbstractSocket::SocketError)));
			
			// TODO: Customizable port number.
			serverSocket->connectToHost("localhost", 7624);
		}
		
		return true;
	}
	
	qDebug() << pipe.errorString();
	return false;
}

void IndiServices::stopDriver(const QString& driverName,
                              const QString& deviceName)
{
	if (!serverProcess ||
	    driverName.isEmpty() ||
	    deviceName.isEmpty())
	{
		return;
	}
	
	QFile pipe(commandPipePath);
	if(pipe.open(QFile::WriteOnly))
	{
		QString command = QString("stop %1 \"%2\"").arg(driverName, deviceName);
		pipe.write(command.toAscii());
		pipe.close();
	}
	else
	{
		qDebug() << pipe.errorString();
	}
}

IndiClient* IndiServices::getCommonClient()
{
	return commonClient;
}

IndiClient* IndiServices::getClient(const QString& id)
{
	return socketClients.value(id, 0);
}

QHashIterator<QString, IndiClient *> IndiServices::getClientIterator()
{
	return QHashIterator<QString,IndiClient*>(socketClients);
}


void IndiServices::openConnection(const QString& id,
                                  const QString& host,
                                  quint16 port)
{
	if (id.isEmpty() || host.isEmpty())
		return;
	
	if (sockets.contains(id))
	{
		qWarning() << "IndiServices: Error! There's already a connection"
		           << "with this ID:" << id;
		return;
	}
	
	QTcpSocket* newSocket = new QTcpSocket(this);
	sockets.insert(id, newSocket);
	connectedMapper->setMapping(newSocket, id);
	connect(newSocket, SIGNAL(connected()),
	        connectedMapper, SLOT(map()));
	connect(newSocket, SIGNAL(disconnected()),
	        this, SLOT(destroySocket()));
	
	newSocket->connectToHost(host, port);
}

void IndiServices::closeConnection(const QString& id)
{
	if (!sockets.contains(id))
	{
		qWarning() << "IndiServices: Error: No connection with this ID"
		           << "to close:" << id;
		return;
	}
	
	QTcpSocket* socket = sockets.take(id);
	if (socket)
	{
		disconnect(socket, SIGNAL(connected()), connectedMapper, SLOT(map()));
		// TODO: Or should I use abort()? (discards the data to be written)
		socket->disconnectFromHost();
	}
}

void IndiServices::initCommonClient()
{
	if (!commonClient)
	{
		commonClient = new IndiClient("common", serverSocket);
		emit commonClientConnected(commonClient);
	}
}

void IndiServices::initClient(const QString& id)
{
	if (id.isEmpty())
		return;
	
	QTcpSocket* socket = sockets.value(id, 0);
	if (socket)
	{
		IndiClient* newClient = new IndiClient(id, socket);
		emit clientConnected(newClient);
	}
}

void IndiServices::handleConnectionError(QAbstractSocket::SocketError error)
{
	//TODO
	Q_UNUSED(error);
	
	qDebug() << serverSocket->errorString();
}

void IndiServices::handleProcessError(QProcess::ProcessError error)
{
	//TODO
	Q_UNUSED(error);
	
	qDebug() << serverProcess->errorString();
}

void IndiServices::destroySocket()
{
	sender()->deleteLater();
}
