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
class QPlainTextEdit;
class QSplitter;
class QTabWidget;

//! A control panel window used in the TelescopeControl plug-in.
//! \author Bogdan Marinov
class DeviceControlPanel : public StelDialog
{
	Q_OBJECT
public:
	DeviceControlPanel();
	virtual ~DeviceControlPanel();
	void languageChanged();
	void updateStyle();

public slots:
	//! \param clientName must be unique among client names.
	//! \param client must point to a valid client.
	void addIndiClient(IndiClient* client);
	//! Removes the client and all associated tabs (devices and properties).
	void removeIndiClient(const QString& clientName);
	
	//! 
	void addIndiDevice(const QString& clientId, const DeviceP& device);
	//! 
	void removeIndiDevice(const QString& clientId, const QString& deviceName);
	
	void logMessage(const QString& deviceName,
	                const QDateTime& timestamp,
	                const QString& message);
	//! Adds directly this string to the log.
	void logMessage(const QString& message);
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_deviceControlPanelWidget* ui;
	
private:
	QSplitter* splitter;
	QTabWidget* deviceTabWidget;
	QPlainTextEdit* logWidget;

	//! All INDI clients connected to this control panel.
	QHash<QString, IndiClient*> indiClients;
	//! A device (widget) is identified by its name and the name of the client.
	typedef QPair<QString,QString> DeviceId;
	//! All device widgets displayed in this control panel.
	QHash<DeviceId, IndiDeviceWidget*> deviceWidgets;
	
	//! \todo This should probably go in a parent class for all widgets.
	enum DeviceType {StelDevice, IndiDevice, AscomDevice};
	QHash<DeviceId, DeviceType> deviceTypes;
	
	bool collapsed;
	
private slots:
	//! Collapses the window to its title bar.
	void collapseWindow(bool collapse);
};

#endif //_DEVICE_CONTROL_PANEL_HPP_
