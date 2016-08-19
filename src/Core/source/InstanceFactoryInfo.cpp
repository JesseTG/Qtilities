/****************************************************************************
**
** Copyright (c) 2009-2013, Jaco Naudé
**
** This file is part of Qtilities.
**
** For licensing information, please see
** http://jpnaude.github.io/Qtilities/page_licensing.html
**
****************************************************************************/

#include "IFactoryProvider.h"
#include "QtilitiesCoreConstants.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

using namespace Qtilities::Core::Constants;

Qtilities::Core::InstanceFactoryInfo::InstanceFactoryInfo(QXmlStreamReader* doc,Qtilities::ExportVersion version) {
    importXml(doc,version);
}

bool Qtilities::Core::InstanceFactoryInfo::isValid() {
    if (d_factory_tag.isEmpty())
        return false;
    if (d_instance_tag.isEmpty())
        return false;
    return true;
}

quint32 MARKER_IFI_CLASS_SECTION = 0xBAADF00D;

bool Qtilities::Core::InstanceFactoryInfo::exportBinary(QDataStream& stream, Qtilities::ExportVersion version) const {
    Q_UNUSED(version)

    stream << MARKER_IFI_CLASS_SECTION;
    stream << d_factory_tag;
    stream << d_instance_tag;
    stream << d_instance_name;
    stream << MARKER_IFI_CLASS_SECTION;
    return true;
}

bool Qtilities::Core::InstanceFactoryInfo::importBinary(QDataStream& stream, Qtilities::ExportVersion version) {
    Q_UNUSED(version)

    // We don't do a version check here. Observer will do it for us.

    quint32 ui32;
    stream >> ui32;
    if (ui32 != MARKER_IFI_CLASS_SECTION) {
        LOG_ERROR("InstanceFactoryInfo binary import failed to detect start marker. Import will fail: " + QString(Q_FUNC_INFO));
        return false;
    }
    stream >> d_factory_tag;
    stream >> d_instance_tag;
    stream >> d_instance_name;
    stream >> ui32;
    if (ui32 != MARKER_IFI_CLASS_SECTION) {
        LOG_ERROR("InstanceFactoryInfo binary import failed to detect end marker. Import will fail: " + QString(Q_FUNC_INFO));
        return false;
    }
    return true;
}

bool Qtilities::Core::InstanceFactoryInfo::exportXml(QXmlStreamWriter* doc, Qtilities::ExportVersion version) const {
    Q_UNUSED(version)

    doc->writeAttribute("InstanceFactoryInfo", d_instance_tag);

    if (d_factory_tag != QString(qti_def_FACTORY_QTILITIES))
        doc->writeAttribute("FactoryTag", d_factory_tag);

    if (d_instance_tag != d_instance_name)
        doc->writeAttribute("Name", d_instance_name);

    return true;
}

bool Qtilities::Core::InstanceFactoryInfo::importXml(QXmlStreamReader* doc, Qtilities::ExportVersion version) {
    Q_UNUSED(version)

    // We don't do a version check here. Observer will do it for us.

    QXmlStreamAttributes attributes = doc->attributes();

    if (attributes.hasAttribute("FactoryTag"))
        d_factory_tag = attributes.value("FactoryTag").toString();
    else
        d_factory_tag = QString(qti_def_FACTORY_QTILITIES);

    d_instance_tag = attributes.value("InstanceFactoryInfo").toString();

    if (!attributes.hasAttribute("Name"))
        d_instance_name = d_instance_tag;
    else
        d_instance_name = attributes.value("Name").toString();

    return true;
}
