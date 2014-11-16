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
#include "AddOnWidget.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "ui_addonDialog.h"
#include <addons/JsonTableModel.hpp>

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
	ui->catalogsTableView->setObjectName(CATEGORY_CATALOG);
	ui->landscapeTableView->setObjectName(CATEGORY_LANDSCAPE);
	ui->languageTableView->setObjectName(CATEGORY_LANGUAGE_PACK);
	ui->scriptsTableView->setObjectName(CATEGORY_SCRIPT);
	ui->starloreTableView->setObjectName(CATEGORY_SKY_CULTURE);
	ui->texturesTableView->setObjectName(CATEGORY_TEXTURE);

	// hashing all tableViews
	m_tableViews.insert(AddOn::CATALOG, ui->catalogsTableView);
	m_tableViews.insert(AddOn::LANDSCAPE, ui->landscapeTableView);
	m_tableViews.insert(AddOn::LANGUAGEPACK, ui->languageTableView);
	m_tableViews.insert(AddOn::SCRIPT, ui->scriptsTableView);
	m_tableViews.insert(AddOn::STARLORE, ui->starloreTableView);
	m_tableViews.insert(AddOn::TEXTURE, ui->texturesTableView);

	// mapping enum_tab to table names
	m_tabToTableName.insert(AddOn::CATALOG, TABLE_CATALOG);
	m_tabToTableName.insert(AddOn::LANDSCAPE, TABLE_LANDSCAPE);
	m_tabToTableName.insert(AddOn::LANGUAGEPACK, TABLE_LANGUAGE_PACK);
	m_tabToTableName.insert(AddOn::SCRIPT, TABLE_SCRIPT);
	m_tabToTableName.insert(AddOn::STARLORE, TABLE_SKY_CULTURE);
	m_tabToTableName.insert(AddOn::TEXTURE, TABLE_TEXTURE);

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
		view->setModel(new JsonTableModel(tab, StelApp::getInstance().getStelAddOnMgr().getAddOnHash()));
	}
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QUrl url(StelApp::getInstance().getStelAddOnMgr().getUrlForUpdates());
	url.setQuery(QString("time=%1").arg(StelApp::getInstance().getStelAddOnMgr().getLastUpdate()));

	QNetworkRequest req(url);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pUpdateCatalogReply->setReadBufferSize(1024*1024*2);

	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
}

void AddOnDialog::downloadFinished()
{
	ui->btnUpdate->setEnabled(true);
	if (m_pUpdateCatalogReply->error() != QNetworkReply::NoError)
	{
		qWarning() << "AddOnDialog : unable to update the database!" << m_pUpdateCatalogReply->errorString();
		ui->txtLastUpdate->setText(q_("Database update failed!"));

		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QString result(m_pUpdateCatalogReply->readAll());
	m_pUpdateCatalogReply->deleteLater();
	m_pUpdateCatalogReply = NULL;

	if (!result.isEmpty())
	{
		if(!StelApp::getInstance().getStelAddOnMgr().updateCatalog(result))
		{
			ui->txtLastUpdate->setText(q_("Database update failed!"));
			return;
		}
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	StelApp::getInstance().getStelAddOnMgr().setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(StelApp::getInstance().getStelAddOnMgr().getLastUpdateString());
	populateTables();
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
