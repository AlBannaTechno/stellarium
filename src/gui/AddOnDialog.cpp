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

#include <QCheckBox>
#include <QDateTime>
#include <QSqlQueryModel>
#include <QStringBuilder>

#include "AddOnDialog.hpp"
#include "ui_addonDialog.h"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"
#include "StelUtils.hpp"

AddOnDialog::AddOnDialog(QObject* parent) : StelDialog(parent)
{
    ui = new Ui_addonDialogForm;
    m_StelAddOn = StelApp::getInstance().getStelAddOn();
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
	}
}

void AddOnDialog::styleChanged()
{
}

void AddOnDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()),this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));
	connect(ui->stackListWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

	// catalog updates
	ui->txtLastUpdate->setText(m_StelAddOn.getLastUpdateString());
	connect(ui->btnUpdate, SIGNAL(clicked()), this, SLOT(updateCatalog()));

	// default tab
	ui->stackedWidget->setCurrentIndex(0);
	ui->stackListWidget->setCurrentRow(0);

	populateTables();
}

void AddOnDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
	if (!current)
	{
		current = previous;
	}
	ui->stackedWidget->setCurrentIndex(ui->stackListWidget->row(current));
}

void AddOnDialog::setUpTableView(QTableView* tableView)
{
	tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	tableView->verticalHeader()->setVisible(false);
	tableView->setAlternatingRowColors(true);
	tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	tableView->setEditTriggers(false);
}

void AddOnDialog::initModel(QTableView* tableView, Category category)
{
	QSqlQueryModel* model = new QSqlQueryModel;
	QString table, query;
	switch (category)
	{
		case CATALOG:
			query = "SELECT title, version, installed "
				"FROM addon INNER JOIN plugin"
				" ON addon.id = plugin.addon UNION ";
			table = "star";
			break;
		case LANDSCAPE:
			table = "landscape";
			break;
		case LANGUAGEPACK:
			table = "language_pack";
			break;
		case SCRIPT:
			table = "script";
			break;
		case STARLORE:
			table = "starlore";
			break;
		case TEXTURE:
			table = "texture";
			break;
	}

	query = query % "SELECT title, version, installed, NULL "
		"FROM addon INNER JOIN " % table %
		" ON addon.id = " % table % ".addon";
	model->setQuery(query);
	model->setHeaderData(0, Qt::Horizontal, q_("Title"));
	model->setHeaderData(1, Qt::Horizontal, q_("Last Version"));
	model->setHeaderData(2, Qt::Horizontal, q_("Installed Version"));
	model->setHeaderData(3, Qt::Horizontal, "");
	tableView->setModel(model);

	for(int i=0; i<tableView->model()->rowCount(); ++i)
	{
		QCheckBox *cbox = new QCheckBox();
		tableView->setIndexWidget(tableView->model()->index(i, 3), cbox);
		cbox->setStyleSheet("QCheckBox { padding-left: 8px; }");
	}
}

void AddOnDialog::populateTables() {
	// CATALOGS
	initModel(ui->catalogsTableView, CATALOG);
	setUpTableView(ui->catalogsTableView);

	// LANDSCAPES
	initModel(ui->landscapeTableView, LANDSCAPE);
	setUpTableView(ui->landscapeTableView);

	// LANGUAGE PACK
	initModel(ui->languageTableView, LANGUAGEPACK);
	setUpTableView(ui->languageTableView);

	// SCRIPTS
	initModel(ui->scriptsTableView, SCRIPT);
	setUpTableView(ui->scriptsTableView);

	// STARLORE
	initModel(ui->starloreTbleView, STARLORE);
	setUpTableView(ui->starloreTbleView);

	// TEXTURES
	initModel(ui->texturesTableView, TEXTURE);
	setUpTableView(ui->texturesTableView);
}

void AddOnDialog::updateCatalog()
{
	ui->btnUpdate->setEnabled(false);
	ui->txtLastUpdate->setText(q_("Updating catalog..."));

	QNetworkRequest req(QUrl("http://cardinot.sourceforge.net/getUpdates.php?time="
				 % QString::number(m_StelAddOn.getLastUpdate())));
	req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);
	req.setAttribute(QNetworkRequest::RedirectionTargetAttribute, false);
	req.setRawHeader("User-Agent", StelUtils::getApplicationName().toLatin1());
	m_pUpdateCatalogReply = StelApp::getInstance().getNetworkAccessManager()->get(req);
	m_pUpdateCatalogReply->setReadBufferSize(1024*1024*2);
	connect(m_pUpdateCatalogReply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(m_pUpdateCatalogReply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void AddOnDialog::downloadError(QNetworkReply::NetworkError)
{
	Q_ASSERT(m_pUpdateCatalogReply);
	qWarning() << "Error updating database catalog!" << m_pUpdateCatalogReply->errorString();
	ui->btnUpdate->setEnabled(true);
	ui->txtLastUpdate->setText(q_("Database update failed!"));
}

void AddOnDialog::downloadFinished()
{
	Q_ASSERT(m_pUpdateCatalogReply);
	if (m_pUpdateCatalogReply->error()!=QNetworkReply::NoError)
	{
		m_pUpdateCatalogReply->deleteLater();
		m_pUpdateCatalogReply = NULL;
		return;
	}

	QByteArray data=m_pUpdateCatalogReply->readAll();
	QString result(data);

	if (!result.isEmpty())
	{
		m_StelAddOn.updateDatabase(result);
	}

	qint64 currentTime = QDateTime::currentMSecsSinceEpoch()/1000;
	ui->btnUpdate->setEnabled(true);
	m_StelAddOn.setLastUpdate(currentTime);
	ui->txtLastUpdate->setText(m_StelAddOn.getLastUpdateString());
	populateTables();
}
