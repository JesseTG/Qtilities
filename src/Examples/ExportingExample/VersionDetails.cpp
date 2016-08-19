/****************************************************************************
**
** Copyright (c) 2009-2010, Jaco Naude
**
** This file is part of Qtilities which is released under the following
** licensing options.
**
** Option 1: Open Source
** Under this license Qtilities is free software: you can
** redistribute it and/or modify it under the terms of the GNU General
** Public License as published by the Free Software Foundation, either
** version 3 of the License, or (at your option) any later version.
**
** Qtilities is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Qtilities. If not, see http://www.gnu.org/licenses/.
**
** Option 2: Commercial
** Alternatively, this library is also released under a commercial license
** that allows the development of closed source proprietary applications
** without restrictions on licensing. For more information on this option,
** please see the project website's licensing page:
** http://www.qtilities.org/licensing.html
**
** If you are unsure which license is appropriate for your use, please
** contact support@qtilities.org.
**
****************************************************************************/

#include "VersionDetails.h"
#include <QtilitiesCore>

using namespace QtilitiesCore;

struct Qtilities::Examples::ExportingExample::VersionDetailsPrivateData {
    VersionDetailsPrivateData() { }

    QString         description_brief;
    QString         description_detailed;
    int             version_minor;
    int             version_major;
    QString         new_attribute_storage;
};

namespace Qtilities {
    namespace Examples {
        namespace ExportingExample {
            FactoryItem<QObject, VersionDetails> VersionDetails::factory;
        }
    }
}

Qtilities::Examples::ExportingExample::VersionDetails::VersionDetails(const QString& brief, const QString& detailed, QObject *parent) :
    QObject(parent)
{
    d = new VersionDetailsPrivateData;
    d->version_major = 0;
    d->version_minor = 0;
    d->description_brief = brief;
    d->description_detailed = detailed;
    setObjectName(d->description_brief);
}

Qtilities::Examples::ExportingExample::VersionDetails::~VersionDetails() {
    delete d;
}

QString Qtilities::Examples::ExportingExample::VersionDetails::descriptionBrief() const {
    return d->description_brief;
}

void Qtilities::Examples::ExportingExample::VersionDetails::setDescriptionBrief(const QString& description) {
    d->description_brief = description;
    setObjectName(d->description_brief);
}

QString Qtilities::Examples::ExportingExample::VersionDetails::descriptionDetailed() const {
    return d->description_detailed;
}

void Qtilities::Examples::ExportingExample::VersionDetails::setDescriptionDetailed(const QString& description) {
    d->description_detailed = description;
}

int Qtilities::Examples::ExportingExample::VersionDetails::versionMajor() const {
    return d->version_major;
}

void Qtilities::Examples::ExportingExample::VersionDetails::setVersionMajor(int major) {
    d->version_major = major;
}

int Qtilities::Examples::ExportingExample::VersionDetails::versionMinor() const {
    return d->version_minor;
}

void Qtilities::Examples::ExportingExample::VersionDetails::setVersionMinor(int minor) {
    d->version_minor = minor;
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::Examples::ExportingExample::VersionDetails::supportedFormats() const {
    return IExportable::XML;
}

Qtilities::Core::InstanceFactoryInfo Qtilities::Examples::ExportingExample::VersionDetails::instanceFactoryInfo() const {
    InstanceFactoryInfo factoryData(qti_def_FACTORY_QTILITIES,"Version Details",objectName());
    return factoryData;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Examples::ExportingExample::VersionDetails::exportXml(QXmlStreamWriter* doc) const {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesExportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    // Create a simple node and add our information to it:
    doc->writeStartElement("RevisionInfo");

    // Information in both version 0 and version 1 of our class:
    doc->writeAttribute("DescriptionBrief",d->description_brief);
    doc->writeAttribute("DescriptionDetailed",d->description_detailed);
    doc->writeAttribute("Minor",QString::number(d->version_minor));
    doc->writeAttribute("Major",QString::number(d->version_major));

    // Lets say we add a new parameter in the next version of the class:
    if (applicationExportVersion() == 1)
        doc->writeAttribute("NewAttribute",d->new_attribute_storage);

    doc->writeEndElement();

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Examples::ExportingExample::VersionDetails::importXml(QXmlStreamReader* doc, QList<QPointer<QObject> >& import_list) {
    Q_UNUSED(import_list)

    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesImportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    // Find our RevisionInfo element:
    IExportable::ExportResultFlags result = IExportable::Complete;
    while (doc->readNextStartElement()) {
        QString tag = doc->name().toString();

        if (tag == "RevisionInfo") {
            QXmlStreamAttributes attributes = doc->attributes();

            if (attributes.hasAttribute("DescriptionBrief"))
                d->description_brief = attributes.value("DescriptionBrief").toString();

            if (attributes.hasAttribute("DescriptionDetailed"))
                d->description_detailed = attributes.value("DescriptionDetailed").toString();

            if (attributes.hasAttribute("Minor"))
                d->version_minor = attributes.value("Minor").toInt();

            if (attributes.hasAttribute("Major"))
                d->version_major = attributes.value("Major").toInt();

            if (attributes.hasAttribute("NewAttribute") && applicationExportVersion() == 1)
                d->new_attribute_storage = attributes.value("NewAttribute").toString();
        }
    }

    return result;
}

QDataStream & operator<< (QDataStream& stream, const Qtilities::Examples::ExportingExample::VersionDetails& stream_obj) {
    stream_obj.exportBinary(stream);
    return stream;
}

QDataStream & operator>> (QDataStream& stream, Qtilities::Examples::ExportingExample::VersionDetails& stream_obj) {
    QList<QPointer<QObject> > import_list;
    stream_obj.importBinary(stream,import_list);
    return stream;
}

