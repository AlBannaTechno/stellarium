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
 
#ifndef _DEVICE_CONTROL_PANEL_HPP_
#define _DEVICE_CONTROL_PANEL_HPP_

#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QPair>
#include <QSignalMapper>

#include "StelDialog.hpp"

#include "IndiDevice.hpp"
#include "IndiProperty.hpp"

class Ui_deviceControlPanelWidget;
class IndiClient;
class IndiDeviceWidget;
class StelDeviceWidget;
class TelescopeControl;
class QPlainTextEdit;
class QSplitter;
class QTabWidget;

//! A control panel window used in the TelescopeControl plug-in.
//! \author Bogdan Marinov
//! \todo Split the INDI code as a separate class/widget, to be re-used in a
//! stand-alone telescope server.
class DeviceControlPanel : public StelDialog
{
	Q_OBJECT
public:
	DeviceControlPanel(TelescopeControl* plugin);
	virtual ~DeviceControlPanel();
	void languageChanged();
	void updateStyle();

public slots:
	//! \param clientName must be unique among client names.
	//! \param client must point to a valid client.
	void addIndiClient(IndiClient* client);
	//! Removes the client and all associated tabs (devices and properties).
	void removeIndiClient(const QString& clientName);
	
	void addStelDevice(const QString& id);
	void removeStelDevice(const QString& id);
	
	//! 
	//! \todo Remove the BLOB enabling code!
	void addIndiDevice(const QString& clientId, const DeviceP& device);
	//! 
	void removeIndiDevice(const QString& clientId, const QString& deviceName);
	
	//! Adds the appropriately formatted message to the log box.
	void logMessage(const QString& deviceName,
	                const QDateTime& timestamp,
	                const QString& message);
	//! Adds directly this string to the log.
	void logMessage(const QString& message);
	//! Adds a message from the local indiserver to the log box.
	//! Also outputs it to Stellarium's log.
	void logServerMessage(const QString& message);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_deviceControlPanelWidget* ui;
	
private:
	//! Show the device tabs if necessary (i.e. if there is > 1 device defined).
	void showDeviceTabs();
	//! Hide the device tabs and show a message if there are no devices.
	void hideDeviceTabs();
	
	TelescopeControl* deviceManager;
	
	//! Splitter separating the devices tab widget from the log viewer.
	QSplitter* splitter;
	//! Tabs for devices.
	QTabWidget* deviceTabWidget;
	//! Log area for INDI messages and INDI server output.
	QPlainTextEdit* logWidget;

	//! All INDI clients connected to this control panel.
	QHash<QString, IndiClient*> indiClients;
	//! A device (widget) is identified by its name and the name of the client.
	//! \todo Move to the device class header?
	typedef QPair<QString,QString> DeviceId;
	//! All device widgets displayed in this control panel.
	QHash<DeviceId, IndiDeviceWidget*> indiDeviceWidgets;
	
	//! \todo This should probably go in a parent class for all widgets.
	enum DeviceType {StelDevice, IndiDevice, AscomDevice};
	QHash<DeviceId, DeviceType> deviceTypes;
	
	bool collapsed;
	
private slots:
	//! Collapses the window to its title bar.
	void collapseWindow(bool collapse);
	
	//! Opens the plug-in's configuration window.
	void openConfiguration();
	
	//! Clear the log.
	void clearLog();
};

#endif //_DEVICE_CONTROL_PANEL_HPP_
