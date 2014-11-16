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

#include <QSettings>

#include "AOTexture.hpp"
#include "StelAddOnMgr.hpp"

AOTexture::AOTexture()
	: m_sTexturesInstallDir(StelFileMgr::getUserDir() % "/textures/")
	, m_pInstalledTextures(new QSettings(m_sTexturesInstallDir % "installedTextures.ini", QSettings::IniFormat))
{
	StelFileMgr::makeSureDirExistsAndIsWritable(m_sTexturesInstallDir);
}

AOTexture::~AOTexture()
{
}

// It relies on the installedTextures.ini file
QStringList AOTexture::checkInstalledAddOns() const
{
	QStringList res;
	foreach (QString texture, m_pInstalledTextures->allKeys())
	{
		// removing non-existent textures
		if (!QFile(m_sTexturesInstallDir % texture).exists())
		{
			m_pInstalledTextures->remove(texture);
			continue;
		}
		QString installId = m_pInstalledTextures->value(texture).toString();
		res.append(installId % "/" % texture);
	}
	res.sort();
	return res;
}

AddOn::Status AOTexture::installFromFile(const QString& idInstall,
					 const QString& downloadedFilepath,
					 const QStringList& selectedFiles) const
{
	AddOn::Status status;
	QString suffix = QFileInfo(downloadedFilepath).suffix();
	if (suffix == "zip")
	{
		status = installFromZip(idInstall, downloadedFilepath, selectedFiles);
	}
	else if (suffix == "png")
	{
		status = installFromImg(idInstall, downloadedFilepath);
	}
	else
	{
		qWarning() << "Add-On Texture: Unable to intall" << idInstall
			   << "The file found is not a .zip or .png";
		status = AddOn::InvalidFormat;
	}
	return status;
}

AddOn::Status AOTexture::installFromZip(QString idInstall, QString downloadedFilepath, QStringList selectedFiles) const
{
	QZipReader reader(downloadedFilepath);
	if (reader.status() != QZipReader::NoError)
	{
		qWarning() << "Add-On Texture: Unable to open the ZIP archive:"
			   << QDir::toNativeSeparators(downloadedFilepath);
		return AddOn::UnableToRead;
	}

	AddOn::Status status = AddOn::FullyInstalled;
	QList<QZipReader::FileInfo> infoList = reader.fileInfoList();
	foreach(QZipReader::FileInfo info, infoList)
	{
		if (!info.isFile)
		{
			continue;
		}

		QFile file(m_sTexturesInstallDir % info.filePath);

		// when selectedFiles is empty, intall all files
		if (!selectedFiles.isEmpty())
		{
			if (!selectedFiles.contains(info.filePath) && !file.exists())
			{
				status = AddOn::PartiallyInstalled;
				continue;
			}
		}

		file.remove(); // overwrite
		QByteArray data = reader.fileData(info.filePath);
		file.open(QIODevice::WriteOnly);
		file.write(data);
		file.close();

		m_pInstalledTextures->setValue(info.filePath, idInstall);

		qDebug() << "Add-On Texture: New texture installed:" << info.filePath;
	}
	return status;
}

AddOn::Status AOTexture::installFromImg(QString idInstall, QString downloadedFilepath) const
{
	QString filename = QFileInfo(downloadedFilepath).fileName();
	QString destination = m_sTexturesInstallDir % filename;
	QFile(destination).remove();
	if (!QFile(downloadedFilepath).copy(destination))
	{
		qWarning() << "Add-On Texture: Unable to install" << filename;
		return AddOn::UnableToWrite;
	}

	m_pInstalledTextures->setValue(filename, idInstall);

	qDebug() << "Add-On Texture: New texture installed:" << filename;
	return AddOn::FullyInstalled;
}

AddOn::Status AOTexture::uninstallAddOn(const QString& idInstall,
					const QStringList& selectedFiles) const
{
	int filesRemoved = 0;
	foreach (QString texture, m_pInstalledTextures->allKeys())
	{
		if (idInstall != m_pInstalledTextures->value(texture))
		{
			continue;
		}

		if (selectedFiles.isEmpty() || selectedFiles.contains(texture))
		{
			QFile file(m_sTexturesInstallDir % texture);
			if (file.remove())
			{
				qDebug() << "Add-On Texture : texture removed" << texture;
				m_pInstalledTextures->remove(texture);
				filesRemoved++;
			}
			else
			{
				qWarning() << "Add-On Texture : unable to remove" << texture;
			}
		}
	}

	if (filesRemoved == selectedFiles.count())
	{
		qDebug() << "Add-On Textures : Successfully removed" << idInstall;
		return AddOn::NotInstalled;
	}
	else if (filesRemoved > 0)
	{
		qDebug() << "Add-On Textures : Partially removed" << idInstall;
		return AddOn::PartiallyInstalled;
	}

	return AddOn::UnableToRemove; // failed!
}
