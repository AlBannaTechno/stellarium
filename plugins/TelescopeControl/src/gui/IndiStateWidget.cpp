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

#include "IndiStateWidget.hpp"

IndiStateWidget::IndiStateWidget(State initialState, QWidget* parent)
	: QFrame(parent)
{
	this->setMaximumWidth(50);
	this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	label = new QLabel();
	label->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	setState(initialState);
	layout->addWidget(label);

	this->setLayout(layout);
}

void IndiStateWidget::setState(State newState)
{
	state = newState;
	switch (state)
	{
		case StateIdle:
			label->setText("Idle");
			break;
		case StateOk:
			label->setText("OK");
			break;
		case StateBusy:
			label->setText("Busy");
			break;
		case StateAlert:
			label->setText("Alert");
			break;
	}
}
