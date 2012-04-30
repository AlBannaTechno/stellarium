/*
 * Stellarium Device Control plug-in
 * 
 * Copyright (C) 2011 Bogdan Marinov (this file)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "DeviceControlPanel.hpp"
#include "ui_deviceControlPanel.h"

#include <QPlainTextEdit>
#include <QSplitter>
#include <QTabWidget>

#include "StelApp.hpp"
#include "StelGui.hpp"

#include "IndiClient.hpp"
#include "IndiDeviceWidget.hpp"
#include "StelDeviceWidget.hpp"

DeviceControlPanel::DeviceControlPanel()
{
	ui = new Ui_deviceControlPanelWidget;
}

DeviceControlPanel::~DeviceControlPanel()
{
	delete ui;
}

void DeviceControlPanel::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void DeviceControlPanel::createDialogContent()
{
	ui->setupUi(dialog);
	
	// TODO: Better default size
	dialog->resize(560, 420);
	
	// Connect the Close button
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	//Initialize the style
	updateStyle();

	splitter = new QSplitter(dialog);
	splitter->setOrientation(Qt::Vertical);
	splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	//Main tabs
	deviceTabWidget = new QTabWidget(dialog);
	splitter->addWidget(deviceTabWidget);

	//Log widget
	logWidget = new QPlainTextEdit(dialog);
	logWidget->setReadOnly(true);
	logWidget->setMaximumBlockCount(100);//Can display 100 lines/paragraphs maximum
	splitter->addWidget(logWidget);
	splitter->setCollapsible(1, true);

	//ui->verticalLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	ui->verticalLayout->addWidget(splitter);
	splitter->show();
	
	collapsed = false;
	connect(ui->foldWindowButton, SIGNAL(clicked(bool)),
	        this, SLOT(collapseWindow(bool)));
	
	//TEST
	/*
	logMessage("Does everything look OK?");

	IndiDeviceWidget* deviceWidget = new IndiDeviceWidget("Simulated");
	NumberProperty np("numtest", StateIdle, PermissionReadWrite, "Equatorial EOD Coordinates");
	NumberElement* ne1 = new NumberElement("RA", "15:0:0", "%9.6m", "0", "0", "0", "Right Ascension");
	NumberElement* ne2 = new NumberElement("DEC", "0:0:0", "%9.6m", "0", "0", "0", "Declination");
	np.addElement(ne1);
	np.addElement(ne2);
	deviceWidget->defineProperty(&np);
	SwitchProperty sp("switchtest", StateBusy, PermissionReadWrite, SwitchAtMostOne, "Connection");
	SwitchElement* se1 = new SwitchElement("CONNECTED", "On", "Connected");
	SwitchElement* se2 = new SwitchElement("DISCONNECTED", "Off", "Disconnected");
	sp.addElement(se1);
	sp.addElement(se2);
	deviceWidget->defineProperty(&sp);
	deviceTabWidget->addTab(deviceWidget, "Simulated");*/
}

void DeviceControlPanel::collapseWindow(bool collapse)
{
	if (collapse && !collapsed)
	{
		splitter->setVisible(false);
		// Force re-calculation of the layout-controlled dialog->minimumSize()
		ui->verticalLayout->activate();
		QSize size = dialog->size();
		size.setHeight(ui->TitleBar->height());
		dialog->resize(size);
		collapsed = true;
	}
	else if (collapsed && !collapse)
	{
		QSize size = dialog->size();
		size.setHeight(ui->TitleBar->height() + splitter->height());
		dialog->resize(size);
		splitter->setVisible(true);
		collapsed = false;
	}
}

void DeviceControlPanel::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		Q_UNUSED(gui);
	}
}

void DeviceControlPanel::addIndiClient(IndiClient* client)
{
	Q_ASSERT(client);

	QString clientName = client->getId();
	if (indiClients.contains(clientName))
	{
		//TODO
		return;
	}
	else
	{
		if (!dialog)
		{
			setVisible(true);
			setVisible(false);
		}

		indiClients.insert(clientName, client);
		connect(client, SIGNAL(deviceDefined(QString,DeviceP)),
		        this, SLOT(addIndiDevice(QString,DeviceP)));
		connect(client, SIGNAL(deviceRemoved(QString,QString)),
		        this, SLOT(removeIndiDevice(QString,QString)));
		connect(client, SIGNAL(messageReceived(QString,QDateTime,QString)),
		        this, SLOT(logMessage(QString,QDateTime,QString)));
		client->writeGetProperties();
	}
}

void DeviceControlPanel::removeIndiClient(const QString& clientName)
{
	if (!indiClients.contains(clientName))
		return;

	IndiClient* client = indiClients[clientName];

	//Disconnect
	disconnect(client, SIGNAL(deviceDefined(QString,DeviceP)),
	           this, SLOT(addIndiDevice(QString,DeviceP)));
	disconnect(client, SIGNAL(deviceRemoved(QString,QString)),
	           this, SLOT(removeIndiDevice(QString,QString)));
	disconnect(client, SIGNAL(messageReceived(QString,QDateTime,QString)),
	           this, SLOT(logMessage(QString,QDateTime,QString)));

	//Remove controls
	QHashIterator<DeviceId,IndiDeviceWidget*> i(indiDeviceWidgets);
	while (i.hasNext())
	{
		i.next();
		if (i.key().first == clientName)
		{
			IndiDeviceWidget* deviceWidget = i.value();
			int index = deviceTabWidget->indexOf(deviceWidget);
			deviceTabWidget->removeTab(index);
			indiDeviceWidgets.remove(i.key());
			delete deviceWidget;
		}
	}
}

void DeviceControlPanel::addStelDevice(const QString& id)
{
	//qDebug() << "DeviceControlPanel::addStelDevice:" << id;
	if (id.isEmpty())
		return;
	
	Q_ASSERT(deviceTabWidget);
	
	// Show the window if it hasn't been initialized.
	// TODO: Decide if this is the appropriate behaviour
	// and if this is the right point to show it.
	if (!visible())
	{
		setVisible(true);
		emit visibleChanged(true);//StelDialog doesn't do this
	}
	
	StelDeviceWidget* deviceWidget = new StelDeviceWidget(id, deviceTabWidget);
	// TODO: Use the proper display name instead of the ID?
	deviceTabWidget->addTab(deviceWidget, id);
}

void DeviceControlPanel::removeStelDevice(const QString& id)
{
	if (id.isEmpty() || !deviceTabWidget)
		return;
	
	for (int i = 0; i < deviceTabWidget->count(); i++)
	{
		if (deviceTabWidget->tabText(i) == id)
		{
			QWidget* widget = deviceTabWidget->widget(i);
			deviceTabWidget->removeTab(i);
			delete widget;
			break;
		}
	}
}

void DeviceControlPanel::addIndiDevice(const QString& clientId,
                                       const DeviceP& device)
{
	if (clientId.isEmpty() || device.isNull())
	{
		// TODO: Log?
		return;
	}
	
	// If no such client exists...
	if (!indiClients.contains(clientId))
	{
		qDebug() << "Unrecognized client name";
		return;
	}
	
	// Show the window if it hasn't been initialized.
	// TODO: Decide if this is the appropriate behaviour
	// and if this is the right point to show it.
	if (!visible())
	{
		setVisible(true);
		emit visibleChanged(true);//StelDialog doesn't do this
	}
	
	QString deviceName = device->getName();
	DeviceId deviceId(clientId, deviceName);
	
	//Add a new device widget/tab
	IndiDeviceWidget* deviceWidget = new IndiDeviceWidget(device);
	indiDeviceWidgets.insert(deviceId, deviceWidget);
	deviceTypes.insert(deviceId, IndiDevice);
	deviceTabWidget->addTab(deviceWidget, deviceName);
	
	// TODO: Individual control for accepting BLOBs
	indiClients[clientId]->writeEnableBlob(AlsoSendBlobs, deviceName);
}

void DeviceControlPanel::removeIndiDevice(const QString& clientId,
                                          const QString& deviceName)
{
	DeviceId deviceId(clientId, deviceName);
	if (indiDeviceWidgets.contains(deviceId))
	{
		IndiDeviceWidget* deviceWidget = indiDeviceWidgets[deviceId];
		//if (deviceWidget->isEmpty())
		{
			int index = deviceTabWidget->indexOf(deviceWidget);
			deviceTabWidget->removeTab(index);
			indiDeviceWidgets.remove(deviceId);
			deviceTypes.remove(deviceId);
			delete deviceWidget;
		}
	}
}

void DeviceControlPanel::logMessage(const QString& device,
                                    const QDateTime& timestamp,
                                    const QString& message)
{
	QDateTime time = QDateTime::currentDateTime();
	if (timestamp.isValid())
		time = timestamp.toLocalTime();

	if (device.isEmpty())
	{
		QString string = QString("[%1] %2").arg(time.toString(Qt::ISODate),
		                                        message);
		logMessage(string);
	}
	else
	{
		QString string = QString("[%1] %2: %3").arg(time.toString(Qt::ISODate),
		                                            device,
		                                            message);
		logMessage(string);
	}
}

void DeviceControlPanel::logMessage(const QString& message)
{
	logWidget->appendPlainText(message);
}
