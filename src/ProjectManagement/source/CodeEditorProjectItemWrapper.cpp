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

#include "CodeEditorProjectItemWrapper.h"
#include "ProjectManager.h"

#include <IExportable>
#include <ObserverRelationalTable>
#include <QtilitiesCoreApplication>
#include <Logger>
#include <IFactoryProvider>

#include <QApplication>

using namespace Qtilities::Core::Interfaces;
using namespace Qtilities::CoreGui;

struct Qtilities::ProjectManagement::CodeEditorProjectItemWrapperPrivateData {
    CodeEditorProjectItemWrapperPrivateData() : code_editor(0) {}

    QPointer<CodeEditorWidget> code_editor;
};

Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::CodeEditorProjectItemWrapper(CodeEditorWidget* code_editor, QObject *parent) :
    QObject(parent)
{
    d = new CodeEditorProjectItemWrapperPrivateData;
    if (code_editor)
        setCodeEditor(code_editor);
}

void Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::setCodeEditor(CodeEditorWidget* code_editor) {
    if (d->code_editor)
        d->code_editor->disconnect(this);

    d->code_editor = code_editor;
    setObjectName(QString("Code Editor Project Item: \"%1\"").arg(code_editor->objectName()));
    IModificationNotifier* mod_iface = qobject_cast<IModificationNotifier*> (code_editor);
    if (mod_iface)
        connect(mod_iface->objectBase(),SIGNAL(modificationStateChanged(bool)),SLOT(setModificationState(bool)));
}

QString Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::projectItemName() const {
    if (d->code_editor)
        return d->code_editor->objectName() + tr(" Project Item");
    else
        return tr("Uninitialized Code Editor Project Item Wrapper");
}

bool Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::newProjectItem() {
    if (!d->code_editor)
        return false;

    d->code_editor->codeEditor()->clear();
    d->code_editor->setModificationState(false);
    return true;
}

bool Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::closeProjectItem(ITask *task) {
    Q_UNUSED(task)

    if (!d->code_editor)
        return false;

    d->code_editor->codeEditor()->clear();
    d->code_editor->setModificationState(false);
    return true;
}

Qtilities::Core::Interfaces::IExportable::ExportModeFlags Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::supportedFormats() const {
    IExportable::ExportModeFlags flags = 0;
    flags |= IExportable::Binary;
    flags |= IExportable::XML;
    return flags;
}

Qtilities::Core::InstanceFactoryInfo Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::instanceFactoryInfo() const {
    return InstanceFactoryInfo();
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::exportBinary(QDataStream& stream) const {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesExportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    if (!d->code_editor)
        return IExportable::Failed;

    QString current_text = d->code_editor->codeEditor()->toPlainText();
    stream << current_text;

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::importBinary(QDataStream& stream, QList<QPointer<QObject> >& import_list) {
    Q_UNUSED(import_list)

    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesImportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    if (!d->code_editor)
        return IExportable::Failed;

    QString current_text;
    stream >> current_text;
    d->code_editor->codeEditor()->setPlainText(current_text);

    return IExportable::Complete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::exportXml(QXmlStreamWriter* doc) const {
    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesExportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    if (d->code_editor) {
        // Add a new node for this code editor. We don't want it to add its factory data
        // to the ProjectItem node.

        doc->writeStartElement("CodeEditorProjectItemWrapper");

        // Add the text to a CDATA section:
        doc->writeCDATA(d->code_editor->codeEditor()->toPlainText());
        doc->writeEndElement();

        return IExportable::Complete;
    } else
        return IExportable::Incomplete;
}

Qtilities::Core::Interfaces::IExportable::ExportResultFlags Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::importXml(QXmlStreamReader* doc, QList<QPointer<QObject> >& import_list) {
    Q_UNUSED(import_list)

    IExportable::ExportResultFlags version_check_result = IExportable::validateQtilitiesImportVersion(exportVersion(),exportTask());
    if (version_check_result != IExportable::VersionSupported)
        return version_check_result;

    if (d->code_editor) {
        while (doc->readNext() != QXmlStreamReader::EndElement) {
            QStringRef child = doc->name();

            if (child == "CodeEditorProjectItemWrapper") {
                doc->readNext();
                d->code_editor->codeEditor()->setPlainText(doc->text().toString());
                return IExportable::Complete;
            }
        }
    }

    return IExportable::Incomplete;
}

bool Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::isModified() const {
    if (d->code_editor)
        return d->code_editor->isModified();
    else
        return false;
}

void Qtilities::ProjectManagement::CodeEditorProjectItemWrapper::setModificationState(bool new_state, IModificationNotifier::NotificationTargets notification_targets, bool force_notifications) {
    Q_UNUSED(force_notifications)

    if (!d->code_editor)
        return;

    if (notification_targets & IModificationNotifier::NotifyListeners)
        emit modificationStateChanged(new_state);
    if (notification_targets & IModificationNotifier::NotifySubjects)
        d->code_editor->setModificationState(new_state,notification_targets);
}
