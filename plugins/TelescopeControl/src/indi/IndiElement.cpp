/*
 * Qt-based INDI wire protocol client
 * 
 * Copyright (C) 2010-2011 Bogdan Marinov
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

#include "IndiElement.hpp"

#include <cmath>
#include <QBuffer>
#include <QDebug>
#include <QRegExp>
#include <QStringList>
#include "kzip.h"

// If it turns out that I have to make a separate class for attribute parsing,
// I'll kick myself...
const char* Element::A_NAME = "name";
const char* Element::A_LABEL = "label";
const char* Element::A_FORMAT = "format";
const char* Element::A_MINIMUM = "min";
const char* Element::A_MAXIMUM = "max";
const char* Element::A_STEP = "step";
const char* Element::A_SIZE = "size";

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark Element Methods
#endif
/* ********************************************************************* */
Element::Element(const QString &elementName, const QString &elementLabel)
{
	name = elementName;
	if (elementLabel.isEmpty())
		label = name;
	else
		label = elementLabel;
}

Element::Element(const QXmlStreamAttributes& attributes) :
    valid (false),
    used (false)
{
	name = attributes.value(Element::A_NAME).toString();
	if (name.isEmpty())
		return; //TODO: Probably a debug is necessary here...
	valid = true;
	
	label = attributes.value(Element::A_LABEL).toString();
	if (label.isEmpty())
		label = name;
}

const QString& Element::getName() const
{
	return name;
}

const QString& Element::getLabel() const
{
	return label;
}

TextElement::TextElement(const QString& elementName,
                         const QString& initialValue,
                         const QString& label)
	: Element(elementName, label),
	value(initialValue)
{
	//
}

TextElement::TextElement(const QXmlStreamAttributes &attributes) :
    Element(attributes)
{
	//
}

QString TextElement::getValue() const
{
	return value;
}

void TextElement::setValue(const QString& stringValue)
{
	//TODO: Validation?
	value = stringValue;
	used = true;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark NumberElement Methods
#endif
/* ********************************************************************* */
NumberElement::NumberElement(const QString& elementName,
									  const double initialValue,
									  const QString& format,
									  const double minimumValue,
									  const double maximumValue,
									  const double step,
									  const QString& label) :
Element(elementName, label),
value(initialValue),
maxValue(maximumValue),
minValue(minimumValue),
step(step)
{
	//TODO: Validation and other stuff
	formatString = format;
}

NumberElement::NumberElement(const QString& elementName,
                             const QString& initialValue,
                             const QString& format,
                             const QString& minimalValue,
                             const QString& maximalValue,
                             const QString& incrementStep,
                             const QString& elementLabel) :
Element(elementName, elementLabel)
{
	//TODO: Validation and other stuff
	formatString = format;

	minValue = readDoubleFromString(minimalValue);
	maxValue = readDoubleFromString(maximalValue);
	step = readDoubleFromString(incrementStep);
	value = readDoubleFromString(initialValue);
}

NumberElement::NumberElement(const QXmlStreamAttributes &attributes) :
    Element(attributes)
{
	if (!valid)
		return;
	
	// Required attributes
	formatString = attributes.value(Element::A_FORMAT).toString();
	QString minString = attributes.value(Element::A_MINIMUM).toString();
	QString maxString = attributes.value(Element::A_MAXIMUM).toString();
	QString stepString = attributes.value(Element::A_STEP).toString();
	if (formatString.isEmpty() ||
	    minString.isEmpty() ||
	    maxString.isEmpty() ||
	    stepString.isEmpty())
	{
		qDebug() << "Invalid required attribute(s) in defNumber:"
		         << "format, min, max, step"
		         << formatString << minString << maxString << stepString;
		valid = false;
		return;
	}
	
	// TODO: Validation of the format string
	
	minValue = readDoubleFromString(minString);
	maxValue = readDoubleFromString(maxString);
	step = readDoubleFromString(stepString);
}

double NumberElement::readDoubleFromString(const QString& string)
{
	if (string.isEmpty())
		return 0.0;

	bool isDecimal = false;
	double decimalNumber = string.toDouble(&isDecimal);
	if (isDecimal)
		return decimalNumber;

	double degrees = 0.0, minutes = 0.0, seconds = 0.0;
	QRegExp separator("[\\ \\: \\;]");
	QStringList components = string.split(separator);
	if (components.count() < 2 || components.count() > 3)
		return 0.0;

	degrees = components.at(0).toDouble();
	minutes = components.at(1).toDouble();
	if (components.count() == 3)
		seconds = components.at(2).toDouble();

	//TODO: Smarter way of calculating a degree fraction.
	//TODO: And dealing with the sign.
	double sign = 1.0;
	if (degrees < 0)
	{
		sign = -1.0;
		degrees = -degrees;
	}
	degrees += (minutes/60.0);
	degrees += (seconds/3600.0);
	degrees *= sign;

	return degrees;
}

QString NumberElement::getFormattedValue() const
{
	if (!(valid && used))
		return QString();
	
	//TODO: Optimize
	QString formattedValue;
	QRegExp indiFormat("^\\%(\\d+)\\.(\\d)\\m$");
	if (indiFormat.exactMatch(formatString))
	{
		int width = indiFormat.cap(1).toInt();
		int precision = indiFormat.cap(2).toInt();
		if (width < 1)
			return formattedValue;

		double seconds = value * 3600.0;
		int degrees = seconds / 3600;
		seconds = fabs(seconds);
		seconds = fmod(seconds, 3600);
		double minutes = seconds / 60.0;
		seconds = fmod(seconds, 60);

		switch (precision)
		{
		case 3:
			formattedValue = QString("%1:%2").arg(degrees).arg(minutes, 2, 'f', 0, '0');
			break;
		case 5:
			formattedValue = QString("%1:%2").arg(degrees).arg(minutes, 4, 'f', 1, '0');
			break;
		case 6:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 2, 'f', 0, '0');
			break;
		case 8:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 4, 'f', 1, '0');
			break;
		case 9:
		default:
			formattedValue = QString("%1:%2:%3").arg(degrees).arg(minutes, 2, 'f', 0, '0').arg(seconds, 5, 'f', 2, '0');
		}
		formattedValue = formattedValue.rightJustified(width, ' ', true);
	}
	else
	{
		//TODO: Use the standard function for C-formatted output?
		formattedValue.sprintf(formatString.toAscii(), value);
	}
	return formattedValue;
}

double NumberElement::getValue() const
{
	return value;
}

void NumberElement::setValue(const QString& stringValue)
{
	double newValue = readDoubleFromString(stringValue);
	if (newValue < minValue)
		return;

	//If maxValue == minValue, it should be ignored per the INDI specification.
	if (maxValue > minValue && newValue > maxValue)
		return;

	//If step is 0, it should be ignored per the INDI specification.
	//TODO: Doesn't work very well for non-integers.
	//Alternative: (fmod((newValue-minValue), step) - step) > 0.001,
	//but that doesn't work very well, too.
	if (step > 0.0)
	{
		double remainder = fmod((newValue - minValue), step);
		if (qFuzzyCompare(remainder+1, 0.0+1))
			return;
	}

	value = newValue;
	used = true;
}

QString NumberElement::getFormatString() const
{
	return formatString;
}

double NumberElement::getMinValue() const
{
	return minValue;
}

double NumberElement::getMaxValue() const
{
	return maxValue;
}

double NumberElement::getStep() const
{
	return step;
}

/* ********************************************************************* */
#if 0
#pragma mark -
#pragma mark SwitchElement Methods
#endif
/* ********************************************************************* */
SwitchElement::SwitchElement(const QString& elementName,
                             const QString& initialValue,
                             const QString& label) :
	Element(elementName, label)
{
	state = false;
	setValue(initialValue);
}

SwitchElement::SwitchElement(const QXmlStreamAttributes &attributes) :
    Element(attributes),
    state(false)
{
	//
}

bool SwitchElement::isOn()
{
	return state;
}

void SwitchElement::setValue(const QString& string)
{
	if (string == "On")
	{
		state = true;
		used = true;
	}
	else if (string == "Off")
	{
		state = false;
		used = true;
	}
	else
		return;
	//TODO: Output?
}


LightElement::LightElement(const QString& elementName,
                           const QString& initialValue,
                           const QString& label)
	: Element(elementName, label)
{
	setValue(initialValue);
}

LightElement::LightElement(const QXmlStreamAttributes& attributes) :
    Element(attributes),
    state(StateIdle)
{
	//
}

State LightElement::getValue() const
{
	return state;
}

void LightElement::setValue(const QString& stringValue)
{
	if (stringValue == "Idle")
	{
		state = StateIdle;
		used = true;
	}
	else if (stringValue == "Ok")
	{
		state = StateOk;
		used = true;
	}
	else if (stringValue == "Busy")
	{
		state = StateBusy;
		used = true;
	}
	else if (stringValue == "Alert")
	{
		state = StateAlert;
		used = true;
	}
	else
		return;
}

BlobElement::BlobElement(const QString& elementName,
                         const QString& initialValue,
                         const QString& label)
	: Element(elementName, label),
	compressed(false)
{
	Q_UNUSED(initialValue);
}

BlobElement::BlobElement(const QXmlStreamAttributes &attributes) :
    Element(attributes),
    compressed(false)
{
	//
}

#include "zlib.h"
#define ZLIB_CHUNK (16*1024)

void BlobElement::setValue(const QString& blobSize,
                           const QString& blobFormat,
                           const QString& blobDataString)
{
	//TODO: This is just a sketch.
	//TODO: What happens if the device deliberately sends empty data?
	int size = blobSize.toInt();
	if (size <= 0)
	{
		//TODO: Debug
		return;
	}
	//Length is the length of the binary file *after decompression*.

	if (blobFormat.isEmpty())
	{
		//TODO: Debug
		return;
	}
	format = blobFormat;

	if (blobDataString.isEmpty())
	{
		//TODO: Debug
		return;
	}
	QByteArray rawBinaryData = QByteArray::fromBase64(blobDataString.toAscii());

	if (format.endsWith(".z"))
	{
		qDebug() << "Compressed blob";
		QByteArray decompressedData;

		int returnCode;
		unsigned int deflatedBytes;
		char outputBuffer[ZLIB_CHUNK];
		z_stream zstream;
		zstream.zalloc = Z_NULL;
		zstream.zfree = Z_NULL;
		zstream.opaque = Z_NULL;
		zstream.avail_in = 0;
		zstream.next_in = Z_NULL;

		returnCode = inflateInit(&zstream);
		if (returnCode != Z_OK)
		{
			qDebug() << "BlobElement::setValue(): inflateInit failed with code"
			         << returnCode;
			return;
		}

		zstream.avail_in = rawBinaryData.size();//+1 ?
		zstream.next_in = (Bytef*)rawBinaryData.data();
		do
		{
			zstream.avail_out = ZLIB_CHUNK;
			zstream.next_out = (Bytef*)outputBuffer;

			returnCode = inflate(&zstream, Z_NO_FLUSH);
			Q_ASSERT(zstream.state != Z_STREAM_ERROR);//TODO!
			switch (returnCode)
			{
				case Z_NEED_DICT:
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&zstream);
					qDebug() << "BlobElement::setValue(): inflate failed with code"
					         << returnCode;
					return;
				default:
					;
			}

			deflatedBytes = ZLIB_CHUNK - zstream.avail_out;
			decompressedData.append(outputBuffer, deflatedBytes);
		}
		while (zstream.avail_out == 0);

		if (returnCode != Z_STREAM_END)
		{
			qDebug() << "WTF? The z-compressed stream hasn't finished yet?" << returnCode;
		}
		(void)inflateEnd(&zstream);

		binaryData = decompressedData;
		compressed = true;
		//Remove the ".z" suffix, as the format strung will be used for the file extension
		format.chop(2);
	}
	else
		binaryData = rawBinaryData;

	if (binaryData.size() != size)
	{
		qDebug() << "BLOB size mismatch: From XML:" << size
		         << "Actual data length:" << binaryData.size();
		binaryData.clear();
	}
}

void BlobElement::setValue(const QString &){;}

const QByteArray& BlobElement::getValue() const
{
	return binaryData;
}

QString BlobElement::getFormat() const
{
	return format;
}

int BlobElement::getSize() const
{
	return binaryData.size();
}
