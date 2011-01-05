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

#include "Dialog.hpp"
#include "ui_telescopePropertiesWindow.h"
#include "TelescopePropertiesWindow.hpp"
#include "TelescopeControl.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QTimer>

TelescopePropertiesWindow::TelescopePropertiesWindow()
{
	ui = new Ui_widgetTelescopeProperties;
	
	telescopeManager = GETSTELMODULE(TelescopeControl);
	//After the removal of the separate telescope server executables,
	//this list can't be changed after Stellarium has been started:
	deviceModelNames = telescopeManager->getDeviceModels().keys();

	//Test the update for JSON
	clientNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);
	//TODO: Write a proper host/IP regexp?
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);
	circleListValidator = new QRegExpValidator (QRegExp("[0-9,\\.\\s]+"), this);
	#ifdef Q_OS_WIN32
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/dev/.*"), this);
	#endif
}

TelescopePropertiesWindow::~TelescopePropertiesWindow()
{	
	delete ui;
	
	delete clientNameValidator;
	delete hostNameValidator;
	delete circleListValidator;
	delete serialPortValidator;
}

void TelescopePropertiesWindow::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopePropertiesWindow::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()),
	        this, SLOT(discardChanges()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(discardChanges()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->radioButtonDirectConnection, SIGNAL(toggled(bool)),
	        this, SLOT(prepareDirectConnection(bool)));
	connect(ui->radioButtonIndirectConnection, SIGNAL(toggled(bool)),
	        this, SLOT(prepareIndirectConnection(bool)));
	connect(ui->pushButtonSave, SIGNAL(clicked()),
	        this, SLOT(saveChanges()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()),
	        this, SLOT(discardChanges()));
	
	connect(ui->comboBoxDeviceModel,
	        SIGNAL(currentIndexChanged(const QString&)),
	        this,
	        SLOT(deviceModelSelected(const QString&)));

#ifdef Q_OS_WIN32
	connect(ui->pushButtonAscomSelect, SIGNAL(clicked()),
	        this, SLOT(showAscomSelector()));
	connect(ui->pushButtonAscomDeviceSetup, SIGNAL(clicked()),
	        this, SLOT(showAscomDeviceSetup()));
#endif
	
	//Setting validators
	ui->lineEditName->setValidator(clientNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditFovCircleSizes->setValidator(circleListValidator);
	ui->lineEditSerialPort->setValidator(serialPortValidator);
}

void TelescopePropertiesWindow::prepareNewStellariumConfiguration(int slot)
{
	configuredSlot = slot;
	configuredConnectionInterface = ConnectionStellarium;

	ui->stelWindowTitle->setText("New Stellarium Telescope");

	//The user must choose between direct and indirect connection
	ui->stackedWidget->setCurrentWidget(ui->pageType);
	ui->radioButtonDirectConnection->blockSignals(true);
	ui->radioButtonDirectConnection->setChecked(false);
	ui->radioButtonDirectConnection->blockSignals(false);
	ui->radioButtonIndirectConnection->blockSignals(true);
	ui->radioButtonIndirectConnection->setChecked(false);
	ui->radioButtonIndirectConnection->blockSignals(false);

	//Prepare the rest of the window
	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	ui->lineEditName->setText(QString("New Telescope %1").arg(configuredSlot));
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	ui->groupBoxFovCircles->setChecked(false);
	ui->lineEditFovCircleSizes->clear();

	showConnectionTab(true);
#ifdef Q_OS_WIN32
	showAscomTab(false);
#endif
}

void TelescopePropertiesWindow::prepareNewVirtualConfiguration(int slot)
{
	configuredSlot = slot;
	configuredConnectionInterface = ConnectionVirtual;
	configuredConnectionIsRemote = false;

	ui->stelWindowTitle->setText("New Simulated Telescope");

	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(false);
	showSerialTab(false);
	showNetworkTab(false);
#ifdef Q_OS_WIN32
	showAscomTab(false);
#endif

	ui->lineEditName->setText(QString("New Telescope %1").arg(configuredSlot));
	ui->checkBoxConnectAtStartup->setChecked(true);
	ui->groupBoxFovCircles->setChecked(false);
	ui->lineEditFovCircleSizes->clear();

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::prepareNewAscomConfiguration(int slot)
{
	if (!telescopeManager->canUseAscom())
	{
		emit changesDiscarded();
		return;
	}

	configuredSlot = slot;
	configuredConnectionInterface = ConnectionAscom;
	configuredConnectionIsRemote = false;

	ui->stelWindowTitle->setText("New ASCOM Connection");

	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(true);
	showAscomTab(true);
	showSerialTab(false);
	showNetworkTab(false);

	ui->lineEditName->setText(QString("New Telescope %1").arg(configuredSlot));
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	ui->groupBoxFovCircles->setChecked(false);
	ui->lineEditFovCircleSizes->clear();

	ui->lineEditAscomControlId->clear();
	ascomDriverObjectId.clear();

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}
#endif

void TelescopePropertiesWindow::prepareForExistingConfiguration(int slot)
{
	configuredSlot = slot;

	ui->stelWindowTitle->setText("Connection configuration");
	ui->tabWidget->setCurrentWidget(ui->tabGeneral);
	showConnectionTab(true);
	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
	
	//Read the telescope properties
	const QVariantMap& properties = telescopeManager->getTelescopeAtSlot(slot);
	if(properties.isEmpty())
	{
		//TODO: Add debug
		return;
	}

	QString name = properties.value("name").toString();
	ui->lineEditName->setText(name);

	//Properties that are valid for all or have default values:
	int delay = properties.value("delay", DEFAULT_DELAY).toInt();
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));

	if (properties.value("equinox").toString() == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);

	bool connectAtStartup = properties.value("connect_at_startup", false).toBool();
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);

	QStringList circleList = properties.value("circles").toStringList();
	if(!circleList.isEmpty())
	{
		ui->groupBoxFovCircles->setChecked(true);
		ui->lineEditFovCircleSizes->setText(circleList.join(", "));
	}

	//Detecting protocol/interface and connection type
	QString connection = properties.value("connection").toString();

	if (connection == "internal")
	{
		configuredConnectionIsRemote = false;
		showNetworkTab(false);

		QString type = properties.value("type").toString();
#ifdef Q_OS_WIN32
		//TODO: This is mostly a temporary hack.
		if (type == "Ascom")
		{
			if (!telescopeManager->canUseAscom())
			{
				emit changesDiscarded();
				return;
			}

			QString driverId = properties.value("driverId").toString();
			if (driverId.isEmpty())
			{
				emit changesDiscarded();
				return;
			}
			configuredConnectionInterface = ConnectionAscom;

			showAscomTab(true);
			showSerialTab(false);

			ui->lineEditAscomControlId->setText(driverId);
			return;//Normal exit
		}
#endif
		QString deviceModelName = properties.value("device_model").toString();
		if(!deviceModelName.isEmpty())
		{
			//Make the current device model selected in the list
			int index = ui->comboBoxDeviceModel->findText(deviceModelName);
			if(index < 0)
			{
				qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
				emit changesDiscarded();
				return;
			}
			else
			{
				ui->comboBoxDeviceModel->setCurrentIndex(index);
			}
		}
		else if (!type.isEmpty())
		{
			//
		}
		else
		{
			//TODO: Abnormal exit
			emit changesDiscarded();
			return;
		}

		configuredConnectionInterface = ConnectionStellarium;
		showSerialTab(true);

		//Initialize the serial port value
		QString serialPort = properties.value("serial_port").toString();
		ui->lineEditSerialPort->setText(serialPort);
	}
	else if (connection == "remote" || connection == "local")
	{
		configuredConnectionInterface = ConnectionStellarium;
		configuredConnectionIsRemote = true;

		showNetworkTab(true);
		showSerialTab(false);

		//TCP port
		int tcpPort = properties.value("tcp_port", DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot)).toInt();
		ui->spinBoxTcpPort->setValue(tcpPort);

		//Host name/address
		ui->lineEditHostName->setText(properties.value("host_name", "localhost").toString());
	}
	else
	{
		configuredConnectionInterface = ConnectionVirtual;
		configuredConnectionIsRemote = false;

		showConnectionTab(false);
		showAscomTab(false);
		showSerialTab(false);
		showNetworkTab(false);
	}
}

void TelescopePropertiesWindow::prepareDirectConnection(bool prepare)
{
	if(!prepare)
		return;

	configuredConnectionIsRemote = false;
	showConnectionTab(true);
	showSerialTab(true);
	showAscomTab(false);
	showNetworkTab(false);

	ui->lineEditSerialPort->clear();
	//TODO: FIX THIS:
	ui->lineEditSerialPort->setCompleter(new QCompleter(SERIAL_PORT_NAMES, this));
	ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
	//Populate the list of available devices
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

void TelescopePropertiesWindow::prepareIndirectConnection(bool prepare)
{
	if(!prepare)
		return;

	configuredConnectionIsRemote = true;
	showConnectionTab(true);
	showNetworkTab(true);
	showAscomTab(false);
	showSerialTab(false);

	ui->lineEditHostName->setText("localhost");
	ui->spinBoxTcpPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

	ui->stackedWidget->setCurrentWidget(ui->pageProperties);
}

void TelescopePropertiesWindow::saveChanges()
{
	//Main telescope properties
	QString name = ui->lineEditName->text().trimmed();
	if(name.isEmpty())
		return;

	QVariantMap newTelescopeProperties;
	newTelescopeProperties.insert("name", name);

	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	newTelescopeProperties.insert("connect_at_startup", connectAtStartup);

	//Circles
	QString rawCirclesString = ui->lineEditFovCircleSizes->text().trimmed();
	QStringList rawFovCircles;
	if(ui->groupBoxFovCircles->isChecked() && !(rawCirclesString.isEmpty()))
	{
		rawFovCircles = rawCirclesString.simplified().remove(' ').split(',', QString::SkipEmptyParts);
		rawFovCircles.removeDuplicates();
		rawFovCircles.sort();
		
		QVariantList fovCircles;
		for(int i = 0; i < rawFovCircles.size(); i++)
		{
			if(i >= MAX_CIRCLE_COUNT)
				break;
			double circle = rawFovCircles.at(i).toDouble();
			if(circle > 0.0)
				fovCircles.append(circle);
		}
		if (!fovCircles.isEmpty())
			newTelescopeProperties.insert("circles", fovCircles);
	}
	
	//Type and server properties
	//TODO: When adding, check for success!
	QString connection = "virtual";

	if (configuredConnectionInterface != ConnectionVirtual)
	{
		//All non-virtual client types require equinox information.
		QString equinox("J2000");
		if (ui->radioButtonJNow->isChecked())
			equinox = "JNow";
		newTelescopeProperties.insert("equinox", equinox);

		int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxDelay->value());
		newTelescopeProperties.insert("delay", delay);
	}

	if (configuredConnectionInterface == ConnectionStellarium)
	{
		if (configuredConnectionIsRemote)
		{
			int tcpPort = ui->spinBoxTcpPort->value();
			newTelescopeProperties.insert("tcp_port", tcpPort);

			QString hostName = ui->lineEditHostName->text().trimmed();
			if(hostName.isEmpty())//TODO: Validate host
				return;
			if(hostName == "localhost")
			{
				connection = "local";
			}
			else
			{
				connection = "remote";
				newTelescopeProperties.insert("host_name", hostName);
			}
		}
		else
		{
			//Read the serial port
			QString serialPortName = ui->lineEditSerialPort->text();
			if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
				return;//TODO: Add more validation!
			newTelescopeProperties.insert("serial_port", serialPortName);

			connection = "internal";

			QString deviceModel = ui->comboBoxDeviceModel->currentText();
			newTelescopeProperties.insert("device_model", deviceModel);

			QString type = telescopeManager->getDeviceModels().value(deviceModel).server;
			newTelescopeProperties.insert("type", type);
		}
	}
#ifdef Q_OS_WIN32
	else if (configuredConnectionInterface == ConnectionAscom)
	{
		QString ascomControlId = ui->lineEditAscomControlId->text();
		if (ascomControlId.isEmpty())
			return;
		newTelescopeProperties.insert("driverId", ascomControlId);

		connection = "internal";
		newTelescopeProperties.insert("type", "Ascom");
	}
#endif

	newTelescopeProperties.insert("connection", connection);
	telescopeManager->addTelescopeAtSlot(configuredSlot, newTelescopeProperties);
	
	emit changesSaved(name);
}

void TelescopePropertiesWindow::discardChanges()
{
	emit changesDiscarded();
}

void TelescopePropertiesWindow::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

void TelescopePropertiesWindow::showTab(QWidget *tab, const QString& label)
{
	if (ui->tabWidget->indexOf(tab) < 0)
		ui->tabWidget->addTab(tab, label);
}

void TelescopePropertiesWindow::hideTab(QWidget *tab)
{
	ui->tabWidget->removeTab(ui->tabWidget->indexOf(tab));
}


void TelescopePropertiesWindow::showConnectionTab(bool show)
{
	if (show)
		showTab(ui->tabConnection, "Connection");
	else
		hideTab(ui->tabConnection);
}

void TelescopePropertiesWindow::showAscomTab(bool show)
{
	if (show)
	{
#ifdef Q_OS_WIN32
		showTab(ui->tabAscom, "ASCOM");
#endif
	}
	else
		hideTab(ui->tabAscom);
}

void TelescopePropertiesWindow::showSerialTab(bool show)
{
	if (show)
		showTab(ui->tabSerial, "Serial");
	else
		hideTab(ui->tabSerial);
}

void TelescopePropertiesWindow::showNetworkTab(bool show)
{
	if (show)
		showTab(ui->tabNetwork, "Network");
	else
		hideTab(ui->tabNetwork);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::showAscomSelector()
{
	if (!telescopeManager->canUseAscom())
		return;

	QAxObject ascomChooser(this);
	if (!ascomChooser.setControl("DriverHelper.Chooser"))
	{
		emit changesDiscarded();
		return;
	}

	//TODO: Switch to windowed mode
	ascomDriverObjectId = ascomChooser.dynamicCall("Choose(const QString&)", ascomDriverObjectId).toString();
	ui->lineEditAscomControlId->setText(ascomDriverObjectId);
}

void TelescopePropertiesWindow::showAscomDeviceSetup()
{
	if (!telescopeManager->canUseAscom() || ascomDriverObjectId.isEmpty())
		return;

	QAxObject ascomDriver(this);
	if (!ascomDriver.setControl(ascomDriverObjectId))
	{
		ascomDriverObjectId.clear();
		return;
	}
	ascomDriver.dynamicCall("SetupDialog()");
}
#endif
