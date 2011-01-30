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
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	
	//Initialize the style
	updateStyle();

	splitter = new QSplitter(dialog);
	splitter->setOrientation(Qt::Vertical);
	splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	//Main tabs
	deviceTabWidget = new QTabWidget(dialog);
	QWidget* fakeTab = new QWidget();
	deviceTabWidget->addTab(fakeTab, "Device!");
	splitter->addWidget(deviceTabWidget);

	//Log widget
	logWidget = new QPlainTextEdit(dialog);
	logWidget->setReadOnly(true);
	logWidget->setMaximumBlockCount(100);//Can displa 100 lines/paragraphs maximum
	splitter->addWidget(logWidget);
	splitter->setCollapsible(1, true);

	//ui->verticalLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	ui->verticalLayout->addWidget(splitter);
	splitter->show();

	logWidget->appendPlainText("Does everything look OK?");
}



void DeviceControlPanel::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
	}
}


