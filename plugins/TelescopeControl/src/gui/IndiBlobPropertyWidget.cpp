/*
 * Qt-based INDI wire protocol client
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

#include "IndiBlobPropertyWidget.hpp"

IndiBlobPropertyWidget::IndiBlobPropertyWidget(BlobProperty *property,
                                               const QString& title,
                                               QWidget* parent)
	: IndiPropertyWidget(title, parent)
{
	Q_ASSERT(property);

	setGroup(property->getGroup());

	mainLayout = new QHBoxLayout();
	mainLayout->setContentsMargins(0, 0, 0, 0);

	//TODO

	this->setLayout(mainLayout);
}

void IndiBlobPropertyWidget::updateProperty(Property* property)
{
	Q_UNUSED(property);
}
