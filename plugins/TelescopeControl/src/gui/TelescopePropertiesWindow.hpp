/*
 * Stellarium TelescopeControl plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
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
 
#ifndef _TELESCOPE_PROPERTIES_WINDOW_HPP_
#define _TELESCOPE_PROPERTIES_WINDOW_HPP_

#include <QObject>
#include <QHash>
#include <QIntValidator>
#include <QStringList>
#ifdef Q_OS_WIN32
#include <QAxObject>
#endif

//#include "StelDialog.hpp"
#include "StelDialogTelescopeControl.hpp"
#include "TelescopeControlGlobals.hpp"

using namespace TelescopeControlGlobals;

class Ui_widgetTelescopeProperties;
class TelescopeControl;
struct StelStyle;

class TelescopePropertiesWindow : public StelDialogTelescopeControl
{
	Q_OBJECT
public:
	TelescopePropertiesWindow();
	virtual ~TelescopePropertiesWindow();
	void languageChanged();
	
	void prepareForExistingConfiguration(int slot);
	void prepareNewStellariumConfiguration(int slot);
	//void prepareNewIndiConfiguration(int slot);
	void prepareNewVirtualConfiguration(int slot);
#ifdef Q_OS_WIN32
	void prepareNewAscomConfiguration(int slot);
#endif
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_widgetTelescopeProperties* ui;
	
private:
	void showConnectionTab(bool show);
	void showAscomTab(bool show);
	void showIndiTab(bool show);
	void showSerialTab(bool show);
	void showNetworkTab(bool show);
	void showTab(QWidget* tab, const QString& label);
	void hideTab(QWidget* tab);
	void populateDeviceModelList();
	void populateIndiDeviceModelList();
	
private slots:
	void saveChanges();
	void discardChanges();
	
	void prepareDirectConnection(bool);
	void prepareIndirectConnection(bool);
	
	void deviceModelSelected(const QString&);
	//void indiDeviceModelSelected(const QString&);

#ifdef Q_OS_WIN32
	void showAscomSelector();
	void showAscomDeviceSetup();
#endif

signals:
	void changesSaved(QString name);
	void changesDiscarded();
	
private:
	QStringList deviceModelNames;
	
	QRegExpValidator * clientNameValidator;
	QRegExpValidator * hostNameValidator;
	QRegExpValidator * circleListValidator;
	QRegExpValidator * serialPortValidator;
	
	int configuredSlot;
	bool configuredConnectionIsRemote;
	enum ConnectionInterface {
		ConnectionVirtual = 0,//!<
		ConnectionStellarium,
		ConnectionIndi,
#ifdef Q_OS_WIN32
		ConnectionAscom,
#endif
	} configuredConnectionInterface;
	
	TelescopeControl * telescopeManager;

#ifdef Q_OS_WIN32
	QString ascomDriverObjectId;
#endif
};

#endif // _TELESCOPE_PROPERTIES_WINDOW_
