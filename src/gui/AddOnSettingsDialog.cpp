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

#include <QStringBuilder>

#include "AddOnSettingsDialog.hpp"
#include "ui_addonSettingsDialog.h"

#include "StelAddOnMgr.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelTranslator.hpp"

AddOnSettingsDialog::AddOnSettingsDialog()
	: m_iUpdateFrequency(0)
	, m_iUpdateTime(0)
{
	ui = new Ui_addonSettingsDialogForm;
}

AddOnSettingsDialog::~AddOnSettingsDialog()
{
	delete ui;
	ui = NULL;
}

void AddOnSettingsDialog::retranslate()
{
	if (dialog)
	{
		ui->retranslateUi(dialog);
		setAboutHtml();
	}
}

void AddOnSettingsDialog::createDialogContent()
{
	ui->setupUi(dialog);
	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(retranslate()));
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(close()));

	// General tab
	ui->updateFrequency->addItem(q_("Every day"), 1);
	ui->updateFrequency->addItem(q_("Every three days"), 3);
	ui->updateFrequency->addItem(q_("Every week"), 7);

	connect(ui->autoUpdate, SIGNAL(clicked(bool)), this, SLOT(setAutoUpdate(bool)));
	connect(ui->updateFrequency, SIGNAL(currentIndexChanged(int)), this, SLOT(setUpdateFrequency(int)));
	connect(ui->updateTime, SIGNAL(timeChanged(QTime)), this, SLOT(setUpdateTime(QTime)));

	// About tab
	setAboutHtml();
	StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
	if (gui != NULL)
	{
		ui->txtAbout->document()->setDefaultStyleSheet(QString(gui->getStelStyle().htmlStyleSheet));
	}
}

void AddOnSettingsDialog::setAutoUpdate(bool enabled)
{
	ui->updateFrequency->setEnabled(enabled);
	ui->updateTime->setEnabled(enabled);
}

void AddOnSettingsDialog::setUpdateFrequency(int index)
{
	m_iUpdateFrequency = ui->updateFrequency->itemData(index).toInt();
}

void AddOnSettingsDialog::setUpdateTime(QTime time)
{
	m_iUpdateTime = time.hour();
}

void AddOnSettingsDialog::setAboutHtml()
{
	QString html = "<html><head></head><body>";
	html += "<h2>" + q_("Add-On Manager") + "</h2><table width=\"90%\">";
	html += "<tr width=\"30%\"><td><strong>" + q_("Version") + ":</strong></td><td>" + ADDON_MANAGER_VERSION + "</td></tr>";
	html += "<tr><td><strong>" + q_("Author") + ":</strong></td><td>Marcos Cardinot &lt;mcardinot@gmail.com&gt;</td></tr>";
	html += "</table>";

	html += "<h3>" + q_("Links") + "</h3>";
	html += "<p>" + QString(q_("Support is provided via the Launchpad website.  Be sure to put \"%1\" in the subject when posting.")).arg("Add-On Manager") + "</p>";
	html += "<ul>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("If you have a question, you can %1get an answer here%2").arg("<a href=\"https://answers.launchpad.net/stellarium\">")).arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + QString(q_("Bug reports can be made %1here%2.")).arg("<a href=\"https://bugs.launchpad.net/stellarium\">").arg("</a>") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you would like to make a feature request, you can create a bug report, and set the severity to \"wishlist\".") + "</li>";
	// TRANSLATORS: The numbers contain the opening and closing tag of an HTML link
	html += "<li>" + q_("If you want to read full information about the plugin, its history and format of the catalog you can %1get info here%2.").arg("<a href=\"http://stellarium.org/wiki/index.php/Meteor_Showers_plugin\">").arg("</a>") + "</li>";
	html += "</ul></body></html>";

	ui->txtAbout->setHtml(html);
}
