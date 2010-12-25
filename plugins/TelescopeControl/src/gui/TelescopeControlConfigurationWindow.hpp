/*
 * Stellarium Telescope Control Plug-in
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
 
#ifndef _TELESCOPE_CONTROL_CONFIGURATION_WINDOW_HPP_
#define _TELESCOPE_CONTROL_CONFIGURATION_WINDOW_HPP_

#include <QObject>
#include <QHash>
#include <QIntValidator>
#include <QModelIndex>
#include <QStandardItemModel>

//#include "StelDialog.hpp"
#include "StelDialogTelescopeControl.hpp"
#include "TelescopeControlGlobals.hpp"
#include "TelescopePropertiesWindow.hpp"

using namespace TelescopeControlGlobals;

class Ui_widgetTelescopeControlConfiguration;
class TelescopePropertiesWindow;
class TelescopeControl;

//! The configuration window of the TelescopeControl plug-in.
//! \author Bogdan Marinov
class TelescopeControlConfigurationWindow : public StelDialogTelescopeControl
{
	Q_OBJECT
public:
	TelescopeControlConfigurationWindow();
	virtual ~TelescopeControlConfigurationWindow();
	void languageChanged();
	void updateStyle();
	
protected:
	//! Initialize the dialog widgets and connect the signals/slots
	virtual void createDialogContent();
	Ui_widgetTelescopeControlConfiguration* ui;
	
private:
	void populateConnectionList();

	//! Update the text and the tooltip of the ChangeStatus button
	void updateStatusButtonForSlot(int slot);

	int findFirstUnoccupiedSlot();
	QString getStatusStringForSlot(int slot);
	
private slots:
	//! Connects or disconnects the connection selected in the list.
	//! Called when the "Connect/Disconnect" button is clicked.
	void toggleSelectedConnection();
	//! Called when the "Configure" button is clicked.
	void configureSelectedConnection();
	void removeSelectedConnection();
	void createNewStellariumConnection();
	void createNewVirtualConnection();
#ifdef Q_OS_WIN32
	void createNewAscomConnection();
#endif
	
	//! Slot for receiving information from TelescopeConfigurationDialog
	void saveChanges(QString name, ConnectionType type);
	//! Slot for receiving information from TelescopeConfigurationDialog
	void discardChanges();
	
	//! Called when a connection is selected in the list.
	void selectConnection(const QModelIndex&);
	//! Used only by configureSelectedConnection().
	//! The reason for a separate function was that once upon a time
	//! a connection could be edited by double clicking on it.
	void configureConnection(const QModelIndex&);
	
	//! Updates the connection states (connected/disconnected) in the list.
	void updateConnectionStates();

private:
	//! @enum ModelColumns This enum defines the number and the order of the columns in the table that lists active telescopes
	enum ModelColumns {
		ColumnSlot = 0, //!< slot number column
		//ColumnStartup, //!< startup checkbox column
		ColumnStatus, //!< connection status (connected/disconnected)
		ColumnType, //!< connection type column
		ColumnInterface, //!< connection interface column
		ColumnName, //!< telescope name column
		ColumnCount //!< total number of columns
	};
	
	TelescopePropertiesWindow propertiesWindow;
	
	QStandardItemModel * connectionListModel;
	
	TelescopeControl * telescopeManager;

	ConnectionType connectionType[SLOT_NUMBER_LIMIT];
	
	int telescopeCount;
	int configuredSlot;
	bool configuredTelescopeIsNew;
};

#endif // _TELESCOPE_CONTROL_CONFIGURATION_WINDOW_
