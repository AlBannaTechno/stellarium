/*
 * Stellarium 
 * Copyright (C) 2015 Alexander Wolf
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

#include <QObject>
#include <QtDebug>
#include <QVariantList>
#include <QtTest>

#include "tests/testAberration.hpp"

QTEST_MAIN(TestAberration)

void TestAberration::initTestCase()
{
}

void TestAberration::testAberration()
{
	// J. Meeus "Astronomical Algorithms" (2nd ed., with corrections as of August 10, 2009) p.156-157.
	double RA  = 41.0540613; //  2h44m12.9747s
	double Dec = 49.2277489; // 49d13m39.896s
	double JDE = 2462088.6900125; // 2028 Nov. 13.19 TD

	// convert to radians
	RA  *= M_PI/180.;
	Dec *= M_PI/180.;

	double cRA, cDec, rRA, rDec;
	rRA  = 41.0623836;
	rDec = 49.2296238;

	getAberration(JDE, RA, Dec, &cRA, &cDec);

	// convert from radiants
	cRA  *= 180./M_PI;
	cDec *= 180./M_PI;

	QVERIFY2(fabs(cRA  - rRA )<=0.000001, QString("JD %1: Computed RA : %2 Difference: %3").arg(JDE).arg(cRA).arg(cRA - rRA).toUtf8());
	QVERIFY2(fabs(cDec - rDec)<=0.000001, QString("JD %1: Computed Dec: %2 Difference: %3").arg(JDE).arg(cDec).arg(cDec - rDec).toUtf8());
}
