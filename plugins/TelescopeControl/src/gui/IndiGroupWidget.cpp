/*
 * Device Control plug-in for Stellarium
 * 
 * Copyright (C) 2011 Bogdan Marinov
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IndiGroupWidget.hpp"

IndiGroupWidget::IndiGroupWidget(QWidget* parent) : QScrollArea(parent)
{
	setBackgroundRole(QPalette::Dark);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setWidgetResizable(true);

	layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);

	QWidget* innerWidget = new QWidget(this);
	innerWidget->setLayout(layout);

	setWidget(innerWidget);
}

IndiGroupWidget::~IndiGroupWidget()
{
	//
}

void IndiGroupWidget::addPropertyWidget(IndiPropertyWidget* widget)
{
	layout->addWidget(widget);
}

void IndiGroupWidget::removePropertyWidget(IndiPropertyWidget* widget)
{
	layout->removeWidget(widget);
}

int IndiGroupWidget::propertyWidgetsCount() const
{
	return layout->count();
}
