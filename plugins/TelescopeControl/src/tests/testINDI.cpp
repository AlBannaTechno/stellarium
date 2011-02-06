/*
 * Stellarium
 * Copyright (C) 2011 Timothy Reaves
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "IndiProperty.hpp"
#include "tests/testINDI.hpp"

#include <QTest>

QTEST_MAIN(TestNumberElement);

void TestNumberElement::testReadDoubleFromString_EmptyString_Returns0()
{
	NumberElement element("TestNumberElement");
	
	double expectedValue = 0.0;
	double actualValue = element.readDoubleFromString("");
	QCOMPARE(expectedValue, actualValue);
}

void TestNumberElement::testReadDoubleFromString_NonNumber_Returns0()
{
	NumberElement element("TestNumberElement");
	
	double expectedValue = 0.0;
	double actualValue = element.readDoubleFromString("a");
	QCOMPARE(expectedValue, actualValue);
}

void TestNumberElement::testReadDoubleFromString_ReturnsNumber()
{
	NumberElement element("TestNumberElement");
	
	double expectedValue = 101.101;
	double actualValue = element.readDoubleFromString("101.101");
	QCOMPARE(expectedValue, actualValue);
}

void TestNumberElement::testReadDoubleFromString_SexagesimalInputTwoElementWithSpaces()
{
	QString sexagesimalValueWithSpaces = "-10 30.3";
	double expectedValue = -10.505;

	NumberElement element("test", sexagesimalValueWithSpaces, "%9.6m", "0", "0", "0");
	double actualValue = element.getValue();
	QCOMPARE(expectedValue, actualValue);
}

void TestNumberElement::testReadDoubleFromString_SexagesimalInputThreeElementsWithColons()
{
	QString sexagesimalValueWithColons = "-10:30:18";
	double expectedValue = -10.505;
	NumberElement element("test", sexagesimalValueWithColons, "%9.6m", "0", "0", "0");

	double actualValue = element.getValue();
	QCOMPARE(expectedValue, actualValue);
}

void TestNumberElement::testReadDoubleFromString_SexagesimalConsistence()
{
	QString sexagesimalValueWithColons = "-10:30:18";
	NumberElement element("test", sexagesimalValueWithColons, "%9.6m", "0", "0", "0");

	QString actualValue = element.getFormattedValue();
	QCOMPARE(sexagesimalValueWithColons, actualValue);
}

void TestNumberElement::testReadDoubleFromString_SexagesimalConsistenceHighPrecision()
{
	QString sexagesimalValueWithColons = "-10:30:18.05";
	NumberElement element("test", sexagesimalValueWithColons, "%12.9m", "0", "0", "0");

	QString actualValue = element.getFormattedValue();
	QCOMPARE(sexagesimalValueWithColons, actualValue);
}

void TestNumberElement::testNumberElementSetValue_MoreThanMaximumShouldBeIgnored()
{
	NumberElement element("test", "00:00:00", "%d", "00:00:00", "180:00:00", "0");
	
	element.setValue("192:00:00");
	QCOMPARE(element.getValue(), 0.0);
}

void TestNumberElement::testNumberElementSetValue_LessThanMinimumShouldBeIgnored()
{
	NumberElement element("test", "00:00:00", "%d", "90:00:00", "180:00:00", "0");
	
	element.setValue("10");
	QCOMPARE(element.getValue(), 0.0);
}

void TestNumberElement::testNumberElementSetValue_CorrectStepIncrement()
{
	//The example is straight out of Gerry Rozema's e-mail
	NumberElement element("test", "0", "%d", "-180", "180", "0.01");
	
	element.setValue("1.01");
	QCOMPARE(element.getValue(), 1.01);
}

void TestNumberElement::testNumberElementSetValue_NotStepIncrement_ShouldIgnoreValue()
{

	//The example is straight out of Gerry Rozema's e-mail
	NumberElement element("test", "0", "%d", "-180", "180", "0.01");
	
	element.setValue("10.012");
	QCOMPARE(element.getValue(), 0.0);
}
