/*
 * Stellarium Telescope Control Plug-in
 * 
 * Copyright (C) 2009-2011 Bogdan Marinov (this file)
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

#include "Dialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"
#include "TelescopeControl.hpp"
#include "TelescopePropertiesWindow.hpp"
#include "TelescopeControlConfigurationWindow.hpp"
#include "ui_telescopeControlConfigurationWindow.h"
#include "StelGui.hpp"

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItem>
#include <QTimer>

using namespace TelescopeControlGlobals;


TelescopeControlConfigurationWindow::TelescopeControlConfigurationWindow()
{
	ui = new Ui_widgetTelescopeControlConfiguration;
	
	//TODO: Fix this - it's in the same plugin
	deviceManager = GETSTELMODULE(TelescopeControl);
	connectionListModel = new QStandardItemModel(0, ColumnCount);

	connectionCount = 0;
}

TelescopeControlConfigurationWindow::~TelescopeControlConfigurationWindow()
{	
	delete ui;
	
	delete connectionListModel;
}

void TelescopeControlConfigurationWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeControlConfigurationWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(languageChanged()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	//Connect: sender, signal, receiver, method
	//Page: Connection
	connect(ui->pushButtonChangeStatus, SIGNAL(clicked()),
	        this, SLOT(toggleSelectedConnection()));
	connect(ui->pushButtonConfigure, SIGNAL(clicked()),
	        this, SLOT(configureSelectedConnection()));
	connect(ui->pushButtonRemove, SIGNAL(clicked()),
	        this, SLOT(removeSelectedConnection()));

	connect(ui->pushButtonNewStellarium, SIGNAL(clicked()),
	        this, SLOT(createNewStellariumConnection()));
	connect(ui->pushButtonNewIndi, SIGNAL(clicked()),
	        this, SLOT(createNewIndiConnection()));
	connect(ui->pushButtonNewVirtual, SIGNAL(clicked()),
	        this, SLOT(createNewVirtualConnection()));
#ifdef Q_OS_WIN32
	connect(ui->pushButtonNewAscom, SIGNAL(clicked()),
	        this, SLOT(createNewAscomConnection()));
#endif
	
	connect(ui->telescopeTreeView, SIGNAL(clicked (const QModelIndex &)),
	        this, SLOT(selectConnection(const QModelIndex &)));
	//connect(ui->telescopeTreeView, SIGNAL(activated (const QModelIndex &)),
	//        this, SLOT(configureTelescope(const QModelIndex &)));
	
	//Page: Options:
	connect(ui->checkBoxReticles, SIGNAL(toggled(bool)),
	        deviceManager, SLOT(setFlagTelescopeReticles(bool)));
	connect(ui->checkBoxLabels, SIGNAL(toggled(bool)),
	        deviceManager, SLOT(setFlagTelescopeLabels(bool)));
	connect(ui->checkBoxCircles, SIGNAL(toggled(bool)),
	        deviceManager, SLOT(setFlagTelescopeCircles(bool)));
	
	connect(ui->checkBoxEnableLogs, SIGNAL(toggled(bool)),
	        deviceManager, SLOT(setFlagUseTelescopeServerLogs(bool)));
	
	//In other dialogs:
	connect(&propertiesWindow, SIGNAL(changesDiscarded()),
	        this, SLOT(discardChanges()));
	connect(&propertiesWindow, SIGNAL(changesSaved(QString)),
	        this, SLOT(saveChanges(QString)));
	
	//Initialize the style
	updateStyle();

	//Deal with the ASCOM-specific fields
#ifdef Q_OS_WIN32
	if (telescopeManager->canUseAscom())
		ui->labelAscomNotice->setVisible(false);
	else
		ui->pushButtonNewAscom->setEnabled(false);
#else
	ui->pushButtonNewAscom->setVisible(false);
	ui->labelAscomNotice->setVisible(false);
	ui->groupBoxAscom->setVisible(false);
#endif
	
	populateConnectionList();
	
	//Checkboxes
	ui->checkBoxReticles->setChecked(deviceManager->getFlagTelescopeReticles());
	ui->checkBoxLabels->setChecked(deviceManager->getFlagTelescopeLabels());
	ui->checkBoxCircles->setChecked(deviceManager->getFlagTelescopeCircles());
	ui->checkBoxEnableLogs->setChecked(deviceManager->getFlagUseTelescopeServerLogs());
	
	//About page
	//TODO: Expand
	QString htmlPage = "<html><head></head><body>";
	htmlPage += "<h2>Stellarium Telescope Control Plug-in</h2>";
	htmlPage += "<h3>Version " + QString(TELESCOPE_CONTROL_VERSION) + "</h3>";
	htmlPage += "<p>Copyright &copy; 2006 Johannes Gajdosik, Michael Heinz</p>";
	htmlPage += "<p>Copyright &copy; 2009-2010 Bogdan Marinov</p>"
				"<p>This plug-in is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.</p>"
				"<p>This plug-in is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.</p>"
				"<p>You should have received a copy of the GNU General Public License along with this program; if not, write to:</p>"
				"<address>Free Software Foundation, Inc.<br/>"
				"51 Franklin Street, Fifth Floor<br/>"
				"Boston, MA  02110-1301, USA</address>"
				"<p><a href=\"http://www.fsf.org\">http://www.fsf.org/</a></p>";
#ifdef Q_OS_WIN32
	htmlPage += "<h3>QAxContainer Module</h3>";
	htmlPage += "This plug-in is statically linked to Nokia's QAxContainer library, which is distributed under the following license:";
	htmlPage += "<p>Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).<br/>"
	            "All rights reserved.</p>"
	            "<p>Contact: Nokia Corporation (qt-info@nokia.com)</p>"
	            "<p>You may use this file under the terms of the BSD license as follows:</p>"
	            "<blockquote><p>\"Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:</p>"
	            "<p>* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.</p>"
	            "<p>* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.</p>"
	            "<p>* Neither the name of Nokia Corporation and its Subsidiary(-ies) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.</p>"
	            "<p>THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\"</p></blockquote>";
#endif
	htmlPage += "</body></html>";
	ui->textBrowserAbout->setHtml(htmlPage);

	htmlPage = "<html><head></head><body>";
	QFile helpFile(":/telescopeControl/help.utf8");
	helpFile.open(QFile::ReadOnly | QFile::Text);
	htmlPage += helpFile.readAll();
	helpFile.close();
	htmlPage += "</body></html>";
	ui->textBrowserHelp->setHtml(htmlPage);

	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	Q_ASSERT(gui);
	ui->textBrowserAbout->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	ui->textBrowserHelp->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	
	//Everything must be initialized by now, start the updateTimer
	//TODO: Find if it's possible to run it only when the dialog is visible
	QTimer* updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()),
	        this, SLOT(updateConnectionStates()));
	updateTimer->start(200);
}

void TelescopeControlConfigurationWindow::selectConnection(const QModelIndex & index)
{
	int row = index.row();
	QModelIndex nameIndex = connectionListModel->index(row, ColumnName);
	QString selectedId = connectionListModel->data(nameIndex).toString();
	updateStatusButton(selectedId);

	//In all cases
	ui->pushButtonRemove->setEnabled(true);
}

void TelescopeControlConfigurationWindow::configureConnection(const QModelIndex & index)
{
	configuredConnectionIsNew = false;

	int row = index.row();
	QModelIndex nameIndex = connectionListModel->index(row, ColumnName);
	configuredId = connectionListModel->data(nameIndex).toString();
	
	//Stop the connection first if necessary
	if(!deviceManager->stopConnection(configuredId))
		return;

	//Update the status in the list
	updateConnectionStates();
	
	setVisible(false);
	//This should be called first to actually create the dialog content:
	propertiesWindow.setVisible(true);
	
	propertiesWindow.prepareForExistingConfiguration(configuredId);
}

void TelescopeControlConfigurationWindow::toggleSelectedConnection()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	//Extract selected ID
	int row = ui->telescopeTreeView->currentIndex().row();
	const QModelIndex& index = connectionListModel->index(row, ColumnName);
	QString selectedId = connectionListModel->data(index).toString();
	if(deviceManager->isClientConnected(selectedId))
		deviceManager->stopConnection(selectedId);
	else
		deviceManager->startConnection(selectedId);
	updateConnectionStates();
}

void TelescopeControlConfigurationWindow::configureSelectedConnection()
{
	if(ui->telescopeTreeView->currentIndex().isValid())
		configureConnection(ui->telescopeTreeView->currentIndex());
}

void TelescopeControlConfigurationWindow::createNewStellariumConnection()
{
	configuredConnectionIsNew = true;
	configuredId = createDefaultId();
	
	setVisible(false);
	//This should be called first to actually create the dialog content:
	propertiesWindow.setVisible(true);
	propertiesWindow.prepareNewStellariumConfiguration(configuredId);
}

void TelescopeControlConfigurationWindow::createNewIndiConnection()
{
	configuredConnectionIsNew = true;
	configuredId = createDefaultId();

	setVisible(false);
	//This should be called first to actually create the dialog content:
	propertiesWindow.setVisible(true);
	propertiesWindow.prepareNewIndiConfiguration(configuredId);
}

void TelescopeControlConfigurationWindow::createNewVirtualConnection()
{
	configuredConnectionIsNew = true;
	configuredId = createDefaultId();

	setVisible(false);
	//This should be called first to actually create the dialog content:
	propertiesWindow.setVisible(true);
	propertiesWindow.prepareNewVirtualConfiguration(configuredId);
}

#ifdef Q_OS_WIN32
void TelescopeControlConfigurationWindow::createNewAscomConnection()
{
	configuredTelescopeIsNew = true;
	configuredId = createDefaultId();

	setVisible(false);
	//This should be called first to actually create the dialog content:
	propertiesWindow.setVisible(true);
	propertiesWindow.prepareNewAscomConfiguration(configuredId);
}
#endif

void TelescopeControlConfigurationWindow::removeSelectedConnection()
{
	if(!ui->telescopeTreeView->currentIndex().isValid())
		return;
	
	int row = ui->telescopeTreeView->currentIndex().row();
	QModelIndex index = connectionListModel->index(row, ColumnName);
	QString selectedId = connectionListModel->data(index).toString();
	
	if(deviceManager->stopConnection(selectedId))
	{
		//TODO: Update status?
		if(!deviceManager->removeConnection(selectedId))
		{
			//TODO: Add debug
			return;
		}
	}
	else
	{
		//TODO: Add debug
		return;
	}
	
	//Save the changes to file
	deviceManager->saveConnections();
	configuredConnectionIsNew = true;//HACK!
	populateConnectionList();
}

void TelescopeControlConfigurationWindow::saveChanges(QString name)
{
	Q_UNUSED(name);
	//Save the changes to file
	deviceManager->saveConnections();

	populateConnectionList();
	
	configuredConnectionIsNew = false;
	propertiesWindow.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
}

void TelescopeControlConfigurationWindow::discardChanges()
{
	propertiesWindow.setVisible(false);
	setVisible(true);//Brings the current window to the foreground
	
	if (connectionCount == 0)
		ui->pushButtonRemove->setEnabled(false);
	
	configuredConnectionIsNew = false;
}

void TelescopeControlConfigurationWindow::updateConnectionStates()
{
	if (connectionCount == 0)
		return;

	//If the dialog is not visible, don't bother updating the status
	if (!visible())
		return;
	
	QString id;
	for (int i=0; i<(connectionListModel->rowCount()); i++)
	{
		QModelIndex index = connectionListModel->index(i, ColumnName);
		id = connectionListModel->data(index).toString();
		QString newStatusString = getStatusString(id);
		connectionListModel->setData(connectionListModel->index(i, ColumnStatus), newStatusString, Qt::DisplayRole);
	}
	
	if(ui->telescopeTreeView->currentIndex().isValid())
	{
		int row = ui->telescopeTreeView->currentIndex().row();
		QModelIndex index = connectionListModel->index(row, ColumnName);
		QString selectedId = connectionListModel->data(index).toString();
		//Update the ChangeStatus button
		updateStatusButton(selectedId);
	}
}

void TelescopeControlConfigurationWindow::updateStatusButton(const QString& id)
{
	if(deviceManager->isClientConnected(id))
	{
		ui->pushButtonChangeStatus->setText("Disconnect");
		ui->pushButtonChangeStatus->setToolTip("Disconnect from the selected telescope");
		ui->pushButtonChangeStatus->setEnabled(true);
	}
	else
	{
		ui->pushButtonChangeStatus->setText("Connect");
		ui->pushButtonChangeStatus->setToolTip("Connect to the selected telescope");
		ui->pushButtonChangeStatus->setEnabled(true);
	}
}

QString TelescopeControlConfigurationWindow::createDefaultId()
{
	QStringList existingIds = deviceManager->listAllConnectionNames();
	QString id;
	int i = 1;
	do
	{
		//TODO: Localization
		id = QString("New Telescope %1").arg(i++);
	}
	while (existingIds.contains(id));
	return id;
}

void TelescopeControlConfigurationWindow::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		QString style(gui->getStelStyle().htmlStyleSheet);
		ui->textBrowserAbout->document()->setDefaultStyleSheet(style);
		ui->textBrowserHelp->document()->setDefaultStyleSheet(style);
	}
}

void TelescopeControlConfigurationWindow::populateConnectionList()
{
	//Remember the previously selected connection, if any
	int selectedRow = 0;
	if (!configuredConnectionIsNew)
		selectedRow = ui->telescopeTreeView->currentIndex().row();

	//Initializing the list of telescopes
	connectionListModel->clear();
	connectionCount = 0;

	connectionListModel->setColumnCount(ColumnCount);
	QStringList headerStrings;
	//headerStrings << "#";
	//headerStrings << "Start";
	headerStrings <<  "Status";
	headerStrings << "Connection";
	headerStrings << "Interface";
	headerStrings << "Keys";
	headerStrings << "Name";
	connectionListModel->setHorizontalHeaderLabels(headerStrings);

	ui->telescopeTreeView->setModel(connectionListModel);
	ui->telescopeTreeView->header()->setMovable(false);
	ui->telescopeTreeView->header()->setResizeMode(ColumnInterface, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setResizeMode(ColumnStatus, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setResizeMode(ColumnShortcuts, QHeaderView::ResizeToContents);
	ui->telescopeTreeView->header()->setStretchLastSection(true);

	//Populating the list
	QStandardItem * tempItem;

	//Cycle the connections
	QStringList connectionIds = deviceManager->listAllConnectionNames();
	foreach (const QString& id, connectionIds)
	{
		//Read the telescope properties
		//(Minimal validation - this is supposed to have been validated once.)
		const QVariantMap& properties = deviceManager->getConnection(id);
		if(properties.isEmpty())
			continue;

		QString name = properties.value("name").toString();
		if (name.isEmpty())
			continue;

		QString interfaceTypeLabel = properties.value("interface").toString();
		if (interfaceTypeLabel.isEmpty())
			continue;

		bool isRemote = properties.value("isRemoteConnection").toBool();
		QString connectionTypeLabel;
		if (isRemote)
		{
			if (properties.value("host") == "localhost")
			{
				connectionTypeLabel = "local";
			}
			else
			{
				connectionTypeLabel = "remote";
			}
		}
		else
		{
			connectionTypeLabel = "direct";
		}

		int lastRow = connectionListModel->rowCount();
		//TODO: This is not updated, because it was commented out
		//tempItem = new QStandardItem;
		//tempItem->setEditable(false);
		//tempItem->setCheckable(true);
		//tempItem->setCheckState(Qt::Checked);
		//tempItem->setData("If checked, this telescope will start when Stellarium is started", Qt::ToolTipRole);
		//telescopeListModel->setItem(lastRow, ColumnStartup, tempItem);//Start-up checkbox

		//New column on a new row in the list: Connection status
		QString newStatusString = getStatusString(id);
		tempItem = new QStandardItem(newStatusString);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnStatus, tempItem);

		//New column on a new row in the list: Connection type
		tempItem = new QStandardItem(connectionTypeLabel);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnType, tempItem);

		//New column on a new row in the list: Interface type
		tempItem = new QStandardItem(interfaceTypeLabel);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnInterface, tempItem);

		int shortcutNumber = properties.value("shortcutNumber").toInt();
		QString shortcutString;
		if (shortcutNumber > 0 && shortcutNumber < 10)
		{
			shortcutString = QString("Ctrl+%1, Alt+%1").arg(shortcutNumber);
		}
		tempItem = new QStandardItem(shortcutString);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnShortcuts, tempItem);

		//New column on a new row in the list: Telescope name
		tempItem = new QStandardItem(name);
		tempItem->setEditable(false);
		connectionListModel->setItem(lastRow, ColumnName, tempItem);

		//After everything is done, count this as loaded
		connectionCount++;
	}

	//Finished populating the table, let's sort it by name
	//ui->telescopeTreeView->setSortingEnabled(true);//Set in the .ui file
	ui->telescopeTreeView->sortByColumn(ColumnName, Qt::AscendingOrder);
	//(Works even when the table is empty)

	if(connectionCount > 0)
	{
		ui->telescopeTreeView->setFocus();
		ui->telescopeTreeView->setCurrentIndex(connectionListModel->index(selectedRow,0));
		ui->pushButtonChangeStatus->setEnabled(true);
		ui->pushButtonConfigure->setEnabled(true);
		ui->pushButtonRemove->setEnabled(true);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::ResizeToContents);
		ui->labelWarning->setText(LABEL_TEXT_CONTROL_TIP);
	}
	else
	{
		ui->pushButtonChangeStatus->setEnabled(false);
		ui->pushButtonConfigure->setEnabled(false);
		ui->pushButtonRemove->setEnabled(false);
		ui->telescopeTreeView->header()->setResizeMode(ColumnType, QHeaderView::Interactive);
		ui->pushButtonNewStellarium->setFocus();
		if(deviceManager->getDeviceModels().isEmpty())
			ui->labelWarning->setText(LABEL_TEXT_NO_DEVICE_MODELS);
		else
			ui->labelWarning->setText(LABEL_TEXT_ADD_TIP);
	}
}

QString TelescopeControlConfigurationWindow::getStatusString(const QString& id)
{
	if (deviceManager->isClientConnected(id))
	{
		return "Connected";
	}
	else if(deviceManager->doesClientExist(id))
	{
		return "Connecting";
	}
	else
	{
		return "Disconnected";
	}
}
