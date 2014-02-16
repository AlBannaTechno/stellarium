/*
 * Stellarium
 * Copyright (C) 2014 Marcos Cardinot
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

#include <QDateTime>
#include <QFileDialog>
#include <QStringBuilder>

#include "AddOnDialog.hpp"
#include "AddOnTableModel.hpp"
#include "AddOnWidget.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "ui_addonDialog.h"

AddOnDialog::AddOnDialog(QObject* parent)
	: StelDialog(parent)
	, m_pSettingsDialog(new AddOnSettingsDialog(this))
{
	ui = new Ui_addonDialogForm;
}

AddOnDialog::~AddOnDialog()
{
	delete ui;
	ui = NULL;
}

void AddOnDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		updateTabBarListWidgetWidth();
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(updateTableViews()),
		this, SLOT(populateTables()));

	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// settings dialog
	connect(ui->btnSettings, SIGNAL(clicked(bool)), this, SLOT(slotOpenSettings()));

	// naming tables according to the category
	ui->catalogsTableView->setCategory(AddOn::CATALOG);
	ui->landscapeTableView->setCategory(AddOn::LANDSCAPE);
	ui->languageTableView->setCategory(AddOn::LANGUAGEPACK);
	ui->scriptsTableView->setCategory(AddOn::SCRIPT);
	ui->starloreTableView->setCategory(AddOn::STARLORE);
	ui->texturesTableView->setCategory(AddOn::TEXTURE);

	// hashing all tableViews
	m_tableViews.insert(AddOn::CATALOG, ui->catalogsTableView);
	m_tableViews.insert(AddOn::LANDSCAPE, ui->landscapeTableView);
	m_tableViews.insert(AddOn::LANGUAGEPACK, ui->languageTableView);
	m_tableViews.insert(AddOn::SCRIPT, ui->scriptsTableView);
	m_tableViews.insert(AddOn::STARLORE, ui->starloreTableView);
	m_tableViews.insert(AddOn::TEXTURE, ui->texturesTableView);

	// build and populate all tableviews
	populateTables();

	// catalog updates
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// setting up tabs
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
	ui->stackedWidget->setCurrentIndex(AddOn::CATALOG);
	ui->stackListWidget->setCurrentRow(AddOn::CATALOG);

	// buttons: Install and Remove
	connect(ui->btnInstall, SIGNAL(clicked()), this, SLOT(installSelectedRows()));
	connect(ui->btnRemove, SIGNAL(clicked()), this, SLOT(removeSelectedRows()));
	ui->btnInstall->setEnabled(false);
	ui->btnRemove->setEnabled(false);
	for (int itab=0; itab<6; itab++) {
		AddOn::Category tab = (AddOn::Category)itab;
		AddOnTableView* view = m_tableViews.value(tab);
		connect(view, SIGNAL(selectedAddOns(int,int)), this, SLOT(slotUpdateButtons(int,int)));
	}

	// button Install from File
	connect(ui->btnInstallFromFile, SIGNAL(clicked()), this, SLOT(installFromFile()));

	// display the AddOnMgr message
	connect(&StelApp::getInstance().getStelAddOnMgr(), SIGNAL(addOnMgrMsg(StelAddOnMgr::AddOnMgrMsg)),
		this, SLOT(slotUpdateMsg(StelAddOnMgr::AddOnMgrMsg)));

	// fix dialog width
	updateTabBarListWidgetWidth();
}

void AddOnDialog::slotOpenSettings()
{
	m_pSettingsDialog->setVisible(true);
}

void AddOnDialog::slotUpdateButtons(int amountToInstall, int amountToRemove)
{
	ui->btnInstall->setEnabled(amountToInstall > 0);
	ui->btnRemove->setEnabled(amountToRemove > 0);
	QString txtInstall = QString("%1 (%2)").arg(q_("Install")).arg(amountToInstall);
	QString txtRemove = QString("%1 (%2)").arg(q_("Remove")).arg(amountToRemove);
	ui->btnInstall->setText(txtInstall);
	ui->btnRemove->setText(txtRemove);
}

void AddOnDialog::slotUpdateMsg(const StelAddOnMgr::AddOnMgrMsg msg)
{
	QString txt, toolTip;
	switch (msg) {
		case StelAddOnMgr::RestartRequired:
			txt = q_("Stellarium restart requiried!");
			toolTip = q_("You must restart the Stellarium to make some changes take effect.");
			break;
		case StelAddOnMgr::UnableToWriteFiles:
			txt = q_("Unable to write files!");
			toolTip = q_("The user directory is not writable. Please check the permissions on that directory.");
			break;
	}
	ui->msg->setText(txt);
	ui->msg->setToolTip(toolTip);
}

void AddOnDialog::updateTabBarListWidgetWidth()
{
	ui->stackListWidget->setWrapping(false);

	// Update list item sizes after translation
	ui->stackListWidget->adjustSize();

	QAbstractItemModel* model = ui->stackListWidget->model();
	if (!model)
		return;

	int width = 0;
	for (int row = 0; row < model->rowCount(); row++)
	{
		width += ui->stackListWidget->sizeHintForRow(row);
		width += ui->stackListWidget->iconSize().width();
	}

	// Hack to force the window to be resized...
	ui->stackListWidget->setMinimumWidth(width);
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	AddOnTableView* lastView = m_tableViews.value((AddOn::Category)ui->stackedWidget->currentIndex());
	if (lastView)
	{
		lastView->clearSelection();
	}

	current = current ? current : previous;
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void AddOnDialog::populateTables()
{
	for (int itab=0; itab<6; itab++) {
		AddOn::Category tab = (AddOn::Category)itab;
		AddOnTableView* view = m_tableViews.value(tab);
		view->setModel(new AddOnTableModel(tab, StelApp::getInstance().getStelAddOnMgr().getAddOnHash()));
	}
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QNetworkRequest req;
	req.setUrl(QUrl(StelApp::getInstance().getStelAddOnMgr().getUrlForUpdates()));
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void AddOnDialog::downloadFinished()
{
	ui->btnUpdate->setEnabled(true);
	QByteArray result(m_pUpdateCatalogReply->readAll());
	if (m_pUpdateCatalogReply->error() == QNetworkReply::NoError && !result.isEmpty())
	{
		QFile jsonFile(StelApp::getInstance().getStelAddOnMgr().getAddonJsonPath());
		if(jsonFile.exists())
		{
			jsonFile.remove();
		}

		if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text))
		{
			jsonFile.write(result);
			jsonFile.close();

			StelApp::getInstance().getStelAddOnMgr().reloadAddonJsonFile();
			qint64 currentTime = QDateTime::currentMSecsSinceEpoch() / 1000;
			StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
			ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
			populateTables();
		}
		else
		{
			qWarning() << "AddOnDialog : unable to update the database! cannot write json file";
			ui->txtLastUpdate->setText(q_("Database update failed!"));
		}
	}
	else
	{
		qWarning() << "AddOnDialog : unable to update the database!" << m_pUpdateCatalogReply->errorString();
		ui->txtLastUpdate->setText(q_("Database update failed!"));
	}

	m_pUpdateCatalogReply->deleteLater();
	m_pUpdateCatalogReply = NULL;
}

void AddOnDialog::installFromFile()
{
	QString filePath = QFileDialog::getOpenFileName(NULL, q_("Select the add-on"), QDir::homePath());
	if (QFile(filePath).exists())
	{
		StelApp::getInstance().getStelAddOnMgr().installFromFile(filePath);
	}
}

void AddOnDialog::installSelectedRows()
{
	AddOnTableView* view = m_tableViews.value((AddOn::Category)ui->stackedWidget->currentIndex());
	QHashIterator<AddOn*, QStringList> i(view->getSelectedAddonsToInstall());
	while (i.hasNext()) {
		i.next();
		StelApp::getInstance().getStelAddOnMgr().installAddOn(i.key(), i.value());
	}
	view->clearSelection();
}

void AddOnDialog::removeSelectedRows()
{
	AddOnTableView* view = m_tableViews.value((AddOn::Category)ui->stackedWidget->currentIndex());
	QHashIterator<AddOn*, QStringList> i(view->getSelectedAddonsToRemove());
	while (i.hasNext()) {
		i.next();
		StelApp::getInstance().getStelAddOnMgr().removeAddOn(i.key(), i.value());
	}
	view->clearSelection();
}
