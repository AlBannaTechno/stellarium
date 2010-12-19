/*
 * Stellarium TelescopeControl Plug-in
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

	telescopeNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);//Test the update for JSON
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);//TODO: Write a proper host/IP regexp?
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
	
	delete telescopeNameValidator;
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
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->radioButtonTelescopeLocalNative, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocalNative(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	connect(ui->radioButtonTelescopeVirtual, SIGNAL(toggled(bool)), this, SLOT(toggleTypeVirtual(bool)));
#ifdef Q_OS_WIN32
	connect(ui->radioButtonTelescopeLocalAscom, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocalAscom(bool)));
#endif
	
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	
	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(deviceModelSelected(const QString&)));

#ifdef Q_OS_WIN32
	connect(ui->pushButtonAscomSelect, SIGNAL(clicked()), this, SLOT(showAscomSelector()));
	connect(ui->pushButtonAscomDeviceSetup, SIGNAL(clicked()), this, SLOT(showAscomDeviceSetup()));
#endif
	
	//Setting validators
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	ui->lineEditSerialPort->setValidator(serialPortValidator);
}

//Set the configuration panel in a predictable state
void TelescopePropertiesWindow::initConfigurationDialog()
{
	//Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();
	
	//Type
	ui->radioButtonTelescopeLocalNative->setEnabled(true);
	ui->groupBoxType->setVisible(true);

	ui->groupBoxVirtualTelescope->setVisible(false);
#ifdef Q_OS_WIN32
	ui->groupBoxAscomSettings->setVisible(false);
#endif
	
	//Telescope properties
	ui->lineEditTelescopeName->clear();
	ui->labelEquinox->setVisible(false);
	ui->frameEquinox->setVisible(false);
	ui->frameDelay->setVisible(true);
	ui->radioButtonJ2000->setChecked(true);
	ui->checkBoxConnectAtStartup->setChecked(false);
	
	//Device settings
	ui->groupBoxDeviceSettings->setVisible(true);
	ui->lineEditSerialPort->clear();
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

	//Connection settings
	ui->groupBoxConnectionSettings->setVisible(true);
	
	//FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->lineEditCircleList->clear();

	//ASCOM GUI
#ifdef Q_OS_WIN32
	if (!telescopeManager->canUseAscom())
#endif
	{
		ui->radioButtonTelescopeLocalAscom->setVisible(false);
		ui->groupBoxAscomSettings->setVisible(false);
	}
}

void TelescopePropertiesWindow::initNewStellariumTelescope(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New Stellarium Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));

	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
	
	//If there are no drivers, only remote connections are possible
	//(Relic from the time when all drivers were external.)
	if(deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocalNative->setEnabled(false);
		if (ui->radioButtonTelescopeConnection->isChecked())
			toggleTypeConnection(true);
		else
			ui->radioButtonTelescopeConnection->setChecked(true);
	}
	else
	{
		ui->radioButtonTelescopeLocalNative->setEnabled(true);
		if (ui->radioButtonTelescopeLocalNative->isChecked())
			toggleTypeLocalNative(true);
		else
			ui->radioButtonTelescopeLocalNative->setChecked(true);
	}
}

void TelescopePropertiesWindow::initNewVirtualTelescope(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New Simulated Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));

	if (ui->radioButtonTelescopeVirtual->isChecked())
		toggleTypeVirtual(true);
	else
		ui->radioButtonTelescopeVirtual->setChecked(true);
	ui->groupBoxType->setVisible(false);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::initNewAscomTelescope(int slot)
{
	if (!telescopeManager->canUseAscom())
	{
		emit changesDiscarded();
		return;
	}

	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("New ASCOM Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));

	if (ui->radioButtonTelescopeLocalAscom->isChecked())
		toggleTypeLocalAscom(true);
	else
		ui->radioButtonTelescopeLocalAscom->setChecked(true);
	ui->groupBoxType->setVisible(false);
}
#endif

void TelescopePropertiesWindow::initExistingTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Configure Telescope");
	
	//Read the telescope properties
	const QVariantMap& properties = telescopeManager->getTelescopeAtSlot(slot);
	if(properties.isEmpty())
	{
		//TODO: Add debug
		return;
	}

	QString name = properties.value("name").toString();
	ui->lineEditTelescopeName->setText(name);

	QString connection = properties.value("connection").toString();
	QString deviceModelName = properties.value("device_model").toString();
	if(!deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocalNative->setChecked(true);
		
		ui->lineEditHostName->setText("localhost");//TODO: Remove magic word!
		
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
		//Initialize the serial port value
		QString serialPort = properties.value("serial_port").toString();
		ui->lineEditSerialPort->setText(serialPort);
	}
	else if (connection == "remote" || connection == "local")
	{
		ui->radioButtonTelescopeConnection->setChecked(true);//Calls toggleTypeConnection(true)
		ui->lineEditHostName->setText(properties.value("host_name", "localhost").toString());
	}
	else
	{
		if (ui->radioButtonTelescopeVirtual->isChecked())
			toggleTypeVirtual(true);
		else
			ui->radioButtonTelescopeVirtual->setChecked(true);
	}

	//Equinox
	if (properties.value("equinox").toString() == "JNow")
		ui->radioButtonJNow->setChecked(true);
	else
		ui->radioButtonJ2000->setChecked(true);
	
	//Circles
	QStringList circleList = properties.value("circles").toStringList();
	if(!circleList.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);
		ui->lineEditCircleList->setText(circleList.join(", "));
	}
	
	//TCP port
	int portTCP = properties.value("tcp_port", DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot)).toInt();
	ui->spinBoxTCPPort->setValue(portTCP);
	
	//Delay
	int delay = properties.value("delay", DEFAULT_DELAY).toInt();
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));//Microseconds to seconds
	
	//Connect at startup
	bool connectAtStartup = properties.value("connect_at_startup", false).toBool();
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopePropertiesWindow::toggleTypeLocalNative(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->comboBoxDeviceModel->setCurrentIndex(0);
		ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		//Enable/disable controls
		ui->labelHost->setEnabled(false);
		ui->lineEditHostName->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setEnabled(true);
	}
}

void TelescopePropertiesWindow::toggleTypeConnection(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->groupBoxDeviceSettings->setEnabled(false);

		ui->scrollArea->ensureWidgetVisible(ui->groupBoxTelescopeProperties);
	}
	else
	{
		ui->groupBoxDeviceSettings->setEnabled(true);
	}
}

void TelescopePropertiesWindow::toggleTypeVirtual(bool isChecked)
{
	//TODO: This really should be done in the GUI
	ui->groupBoxVirtualTelescope->setVisible(isChecked);
	ui->groupBoxDeviceSettings->setVisible(!isChecked);
	ui->groupBoxConnectionSettings->setVisible(!isChecked);
	ui->labelEquinox->setVisible(!isChecked);
	ui->frameEquinox->setVisible(!isChecked);
	ui->frameDelay->setVisible(!isChecked);

	if (isChecked)
		ui->scrollArea->ensureWidgetVisible(ui->groupBoxVirtualTelescope);
}

#ifdef Q_OS_WIN32
void TelescopePropertiesWindow::toggleTypeLocalAscom(bool isChecked)
{
	if (!telescopeManager->canUseAscom())
		return;

	ui->groupBoxAscomSettings->setVisible(isChecked);
	ui->groupBoxDeviceSettings->setVisible(!isChecked);
	ui->groupBoxConnectionSettings->setVisible(!isChecked);
	ui->frameDelay->setVisible(!isChecked);
}
#endif

void TelescopePropertiesWindow::buttonSavePressed()
{
	//Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();
	if(name.isEmpty())
		return;

	QVariantMap newTelescopeProperties;
	newTelescopeProperties.insert("name", name);

	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	newTelescopeProperties.insert("connect_at_startup", connectAtStartup);

	//Circles
	QString rawCirclesString = ui->lineEditCircleList->text().trimmed();
	QStringList rawFovCircles;
	if(ui->checkBoxCircles->isChecked() && !(rawCirclesString.isEmpty()))
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
	ConnectionType type = ConnectionNA;
	QString connection = "virtual";

	if (ui->radioButtonTelescopeVirtual->isChecked())
	{
		type = ConnectionVirtual;
	}
	else
	{
		//All other client types require equinox information.
		QString equinox("J2000");
		if (ui->radioButtonJNow->isChecked())
			equinox = "JNow";
		newTelescopeProperties.insert("equinox", equinox);

		int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value());
		newTelescopeProperties.insert("delay", delay);

		int tcpPort = ui->spinBoxTCPPort->value();
		newTelescopeProperties.insert("tcp_port", tcpPort);
	}

	if(ui->radioButtonTelescopeLocalNative->isChecked())
	{
		//Read the serial port
		QString serialPortName = ui->lineEditSerialPort->text();
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
			return;//TODO: Add more validation!
		newTelescopeProperties.insert("serial_port", serialPortName);
		
		type = ConnectionInternal;
		connection = "internal";

		QString deviceModel = ui->comboBoxDeviceModel->currentText();
		newTelescopeProperties.insert("device_model", deviceModel);

		QString type = telescopeManager->getDeviceModels().value(deviceModel).server;
		newTelescopeProperties.insert("type", type);
	}

	if (ui->radioButtonTelescopeConnection->isChecked())
	{
		QString hostName = ui->lineEditHostName->text().trimmed();
		if(hostName.isEmpty())//TODO: Validate host
			return;
		if(hostName == "localhost")
		{
			type = ConnectionLocal;
			connection = "local";
		}
		else
		{
			type = ConnectionRemote;
			connection = "remote";
			newTelescopeProperties.insert("host_name", hostName);
		}
	}

	newTelescopeProperties.insert("connection", connection);
	telescopeManager->addTelescopeAtSlot(configuredSlot, newTelescopeProperties);
	
	emit changesSaved(name, type);
}

void TelescopePropertiesWindow::buttonDiscardPressed()
{
	emit changesDiscarded();
}

void TelescopePropertiesWindow::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
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
