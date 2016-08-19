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

#include "SubjectTypeFilter.h"
#include "QtilitiesCoreConstants.h"
#include "Observer.h"

#include <Logger.h>

#include <QMutex>
#include <QVariant>

using namespace Qtilities::Core::Constants;

namespace Qtilities {
    namespace Core {
        FactoryItem<QObject, SubjectTypeFilter> SubjectTypeFilter::factory;
    }
}

struct Qtilities::Core::SubjectTypeFilterPrivateData {
    SubjectTypeFilterPrivateData() : inversed_filtering(false),
        is_modified(false) {}

    bool                    inversed_filtering;
    bool                    is_modified;
    QList<SubjectTypeInfo>  known_subject_types;
    QString                 known_objects_group_name;
};

Qtilities::Core::SubjectTypeFilter::SubjectTypeFilter(const QString& known_objects_group_name, QObject* parent) : AbstractSubjectFilter(parent) {
    d = new SubjectTypeFilterPrivateData;
    d->known_objects_group_name = known_objects_group_name;
}

Qtilities::Core::SubjectTypeFilter::~SubjectTypeFilter() {
    delete d;
}

SubjectTypeFilter& Qtilities::Core::SubjectTypeFilter::operator=(const SubjectTypeFilter& ref) {
    if (this==&ref) return *this;

    d->inversed_filtering = ref.inverseFilteringEnabled();
    d->known_subject_types = ref.knownSubjectTypes();
    d->known_objects_group_name = ref.groupName();

    return *this;
}

bool Qtilities::Core::SubjectTypeFilter::operator==(const SubjectTypeFilter& ref) const {
    if (d->inversed_filtering != ref.inverseFilteringEnabled())
        return false;
    if (d->known_subject_types != ref.knownSubjectTypes())
        return false;
    if (d->known_objects_group_name != ref.groupName())
        return false;

    return true;
}

bool Qtilities::Core::SubjectTypeFilter::operator!=(const SubjectTypeFilter& ref) const {
    return !(*this==ref);
}

AbstractSubjectFilter::EvaluationResult Qtilities::Core::SubjectTypeFilter::evaluateAttachment(QObject* obj, QString* rejectMsg, bool silent) const {
    Q_UNUSED(silent)

    #ifndef QT_NO_DEBUG
    Q_ASSERT(observer != 0);
    #endif
    #ifdef QT_NO_DEBUG
    if (!obj) {
        if (rejectMsg)
            *rejectMsg = QString(tr("Subject Type Filter: Invalid object reference received. Attachment cannot be done."));
        return AbstractSubjectFilter::Rejected;
    }
    #endif

    if (!observer) {
        if (rejectMsg)
            *rejectMsg = QString(tr("Subject Type Filter: Cannot evaluate an attachment in a subject filter without an observer context."));
        LOG_TRACE("Cannot evaluate an attachment in a subject filter without an observer context.");
        return AbstractSubjectFilter::Rejected;
    }

    bool is_known_type = false;
    // If inversed and there are no known types is_known_type must be true:
    if (d->inversed_filtering && d->known_subject_types.count() == 0)
        is_known_type = true;

    // Check the obj meta info against the known filter types
    for (int i = 0; i < d->known_subject_types.count(); ++i) {
        QString meta_type = d->known_subject_types.at(i).d_meta_type;
        if (obj->inherits(meta_type.toUtf8().data())) {
            if (!d->inversed_filtering) {
                is_known_type = true;
                break;
            }
        } else {
            if (d->inversed_filtering) {
                is_known_type = true;
                break;
            }
        }
    }

    if (!is_known_type) {
        QString msg = QString(tr("Subject filter \"%1\" rejected attachment of object \"%2\" to observer \"%3\". It is not an allowed type in this context.")).arg(filterName()).arg(obj->objectName()).arg(observer->observerName());
        #ifndef QT_NO_DEBUG
        for (int t = 0; t < d->known_subject_types.count(); t++)
            LOG_TRACE("Allowed types: Meta type: " + d->known_subject_types.at(t).d_meta_type + ", Type description: " + d->known_subject_types.at(t).d_name);
        LOG_TRACE("Attachment type: " + QString(obj->metaObject()->className()));
        if (d->inversed_filtering)
            LOG_TRACE("Inversed filtering status: Enabled");
        else
            LOG_TRACE("Inversed filtering status: Disabled");
        #endif

        if (rejectMsg)
            *rejectMsg = msg;
        LOG_WARNING(msg);
    }

    if (is_known_type)
        return AbstractSubjectFilter::Allowed;
    else
        return AbstractSubjectFilter::Rejected;
}

bool Qtilities::Core::SubjectTypeFilter::handleMonitoredPropertyChange(QObject* obj, const char* property_name, QDynamicPropertyChangeEvent* propertyChangeEvent) {
    Q_UNUSED(obj)
    Q_UNUSED(property_name)
    Q_UNUSED(propertyChangeEvent)

    // Lock the filter mutex to make this function thread safe.
    if (!filter_mutex.tryLock())
        return false;

    // Handle property changes here...
    // We can change properties on objects here and the filter_mutex tryLock() above will make sure they are not handled.
    // When we handled the property, return true.

    // Unlock the mutex.
    filter_mutex.unlock();
    return true;
}

QString Qtilities::Core::SubjectTypeFilter::groupName() const {
    return d->known_objects_group_name;
}

void Qtilities::Core::SubjectTypeFilter::setGroupName(const QString& group_name) {
    if (d->known_objects_group_name != group_name) {
        d->known_objects_group_name = group_name;
        setModificationState(true);
    }
}

void Qtilities::Core::SubjectTypeFilter::addSubjectType(SubjectTypeInfo subject_type_info) {
    if (observer) {
        if (observer->subjectCount() > 0)
            return;
    }

    d->known_subject_types.append(subject_type_info);
    setModificationState(true);
}

bool Qtilities::Core::SubjectTypeFilter::isKnownType(const QString& meta_type) const {
    for (int i = 0; i < d->known_subject_types.count(); ++i) {
        if (d->known_subject_types.at(i).d_meta_type == meta_type) {
            if (!d->inversed_filtering)
                return true;
        } else {
            if (d->inversed_filtering)
                return true;
        }
    }

    return false;
}

QList<Qtilities::Core::SubjectTypeInfo> Qtilities::Core::SubjectTypeFilter::knownSubjectTypes() const {
    return d->known_subject_types;
}

void Qtilities::Core::SubjectTypeFilter::enableInverseFiltering(bool enabled) {
    if (d->inversed_filtering != enabled) {
        if (observer) {
            if (observer->subjectCount() > 0)
                return;
        }

        d->inversed_filtering = enabled;
        setModificationState(true);
    }
}

bool Qtilities::Core::SubjectTypeFilter::inverseFilteringEnabled() const {
    return d->inversed_filtering;
}

Qtilities::Core::InstanceFactoryInfo Qtilities::Core::SubjectTypeFilter::instanceFactoryInfo() const {
    InstanceFactoryInfo instanceFactoryInfo(qti_def_FACTORY_QTILITIES,qti_def_FACTORY_TAG_SUBJECT_TYPE_FILTER,qti_def_FACTORY_TAG_SUBJECT_TYPE_FILTER);
    return instanceFactoryInfo;
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::Core::SubjectTypeFilter::supportedFormats() const {
    IExportable::ExportModeFlags flags = 0;
    flags |= IExportable::Binary;
    flags |= IExportable::XML;
    return flags;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Core::SubjectTypeFilter::exportBinary(QDataStream& stream) const {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesExportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    stream << d->inversed_filtering;
    stream << d->known_objects_group_name;
    stream << (quint32) d->known_subject_types.count();
    for (int i = 0; i < d->known_subject_types.count(); ++i) {
        stream << d->known_subject_types.at(i).d_meta_type;
        stream << d->known_subject_types.at(i).d_name;
    }

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Core::SubjectTypeFilter::importBinary(QDataStream& stream, QList<QPointer<QObject> >& import_list) {
    Q_UNUSED(import_list)

    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesImportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    stream >> d->inversed_filtering;
    stream >> d->known_objects_group_name;
    quint32 known_type_count;
    stream >> known_type_count;
    int known_type_count_int = known_type_count;
    d->known_subject_types.clear();
    for (int i = 0; i < known_type_count_int; ++i) {
        QString meta_type;
        QString name;
        stream >> meta_type;
        stream >> name;
        SubjectTypeInfo new_type(meta_type, name);
        d->known_subject_types.append(new_type);
    }

    if (d->known_subject_types.count() == (int) known_type_count)
        return IExportable::Complete;
    else
        return IExportable::Failed;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Core::SubjectTypeFilter::exportXml(QXmlStreamWriter* doc) const {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesExportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;
     
    if (d->inversed_filtering)
        doc->writeAttribute("InversedFiltering","True");

    if (!d->known_objects_group_name.isEmpty())
        doc->writeAttribute("GroupName",d->known_objects_group_name);

    doc->writeAttribute("TypeCount",QString::number(d->known_subject_types.count()));

    // Categories:
    if (d->known_subject_types.count() > 0) {
        doc->writeStartElement("KnownTypes");
        for (int i = 0; i < d->known_subject_types.count(); ++i) {
            doc->writeEmptyElement("Type_" + QString::number(i));
            doc->writeAttribute("MetaType",d->known_subject_types.at(i).d_meta_type);
            doc->writeAttribute("Name",d->known_subject_types.at(i).d_name);
        }
        doc->writeEndElement();
    }

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::Core::SubjectTypeFilter::importXml(QXmlStreamReader* doc, QList<QPointer<QObject> >& import_list) {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesImportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    QXmlStreamAttributes attributes = doc->attributes();

    if (attributes.hasAttribute("InversedFiltering")) {
        d->inversed_filtering = (attributes.value("InversedFiltering") == "True");
    }

    if (attributes.hasAttribute("GroupName"))
        d->known_objects_group_name = attributes.value("GroupName").toString();

    int count_readback = 0;
    if (attributes.hasAttribute("TypeCount"))
        count_readback = attributes.value("TypeCount").toInt();

    // Known types stuff:
    while (doc->readNext() != QXmlStreamReader::EndElement) {
        QString childName = doc->name().toString();

        if (childName == "KnownTypes") {
            while (doc->readNext() != QXmlStreamReader::EndElement) {
                QString name2 = doc->name().toString();

                if (name2.startsWith("Type")) {
                    QXmlStreamAttributes attr2 = doc->attributes();

                    QString metaType = attr2.value("MetaType").toString();
                    QString name = attr2.value("Name").toString();

                    if (metaType.isEmpty() || name.isEmpty()) {
                        LOG_ERROR(tr("Invalid subject type filter parameters detected. This filter will not be included in the parsed tree."));
                            return IExportable::Failed;
                    }

                    SubjectTypeInfo new_type(metaType, name);
                    d->known_subject_types << new_type;

                    doc->skipCurrentElement();
                }
            }

            doc->skipCurrentElement();
        }
    }

    if (d->known_subject_types.count() == count_readback)
        return IExportable::Complete;
    else
        return IExportable::Failed;
}

bool Qtilities::Core::SubjectTypeFilter::isModified() const {
    return d->is_modified;
}

void Qtilities::Core::SubjectTypeFilter::setModificationState(bool new_state, IModificationNotifier::NotificationTargets notification_targets, bool force_notifications) {
    Q_UNUSED(force_notifications)

    d->is_modified = new_state;
    if (notification_targets & IModificationNotifier::NotifyListeners) {
        emit modificationStateChanged(new_state);
    }
}

QDataStream & operator<< (QDataStream& stream, const Qtilities::Core::SubjectTypeFilter& stream_obj) {
    stream_obj.exportBinary(stream);
    return stream;
}

QDataStream & operator>> (QDataStream& stream, Qtilities::Core::SubjectTypeFilter& stream_obj) {
    QList<QPointer<QObject> > import_list;
    stream_obj.importBinary(stream,import_list);
    return stream;
}
