/*
 * Stellarium Telescope Control plug-in
 * Copyright (C) 2010  Bogdan Marinov
 * Copyright (C) 2011  Timothy Reaves
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TelescopeClientIndiTcp.hpp"

#include <cmath>
#include <QFile>
#include <QFileInfo>

#include "StelUtils.hpp"

TelescopeClientIndiTcp::TelescopeClientIndiTcp(const QString& name, const QString& host, int port, Equinox eq):
	TelescopeClientIndi(name, eq),
	tcpSocket(0)
{
	qDebug() << "Creating INDI TCP telescope client:" << name;

	if (host.isEmpty())
		return;
	//TODO: Validation
	if (port <= 0)
		return;

	tcpSocket = new QTcpSocket();
	for (int i=0; i<3; i++)
	{
		tcpSocket->connectToHost(host, port);
		if (tcpSocket->waitForConnected(1000))
		{
			connect(tcpSocket,
					  SIGNAL(error(QAbstractSocket::SocketError)),
					  this,
					  SLOT(handleConnectionError(QAbstractSocket::SocketError)));
			indiClient.addConnection(tcpSocket);
			break;
		}
	}
}

TelescopeClientIndiTcp::~TelescopeClientIndiTcp()
{
	//TODO: Disconnect stuff
	if (tcpSocket)
	{
		disconnect(tcpSocket,
					  SIGNAL(error(QAbstractSocket::SocketError)),
					  this,
					  SLOT(handleConnectionError(QAbstractSocket::SocketError)));
		tcpSocket->disconnectFromHost();
		tcpSocket->deleteLater();
	}
}

bool TelescopeClientIndiTcp::isInitialized() const
{
	//TODO: Improve the checks.
	if (tcpSocket->isOpen() &&
		 tcpSocket->isReadable() &&
		 tcpSocket->isWritable())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool TelescopeClientIndiTcp::isConnected() const
{
	//TODO: Should include a check for the CONNECTION property
	if (!isInitialized())
		return false;
	else
		return true;
}

bool TelescopeClientIndiTcp::prepareCommunication()
{
	if (!isInitialized())
		return false;

	//TODO: Request properties?
	//TODO: Try to connect

	return true;
}

void TelescopeClientIndiTcp::performCommunication()
{
	//
}

void TelescopeClientIndiTcp::handleConnectionError(QTcpSocket::SocketError error)
{
	//TODO
	qDebug() << error;
	qDebug() << tcpSocket->errorString();
}

