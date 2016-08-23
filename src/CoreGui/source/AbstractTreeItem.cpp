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

#include "AbstractTreeItem.h"
#include "TreeNode.h"
#include "NamingPolicyFilter.h"

using namespace Qtilities::Core;

Qtilities::CoreGui::AbstractTreeItem::AbstractTreeItem() {
    baseItemData = new AbstractTreeItemPrivateData;

}

Qtilities::CoreGui::AbstractTreeItem::~AbstractTreeItem() {
    delete baseItemData;
}

QString Qtilities::CoreGui::AbstractTreeItem::getName(TreeNode* parent) const {
    if (!parent)
        return getTreeItemObjectBase()->objectName();
    else {
        if (parent->contains(getTreeItemObjectBase()))
            return parent->subjectNameInContext(getTreeItemObjectBase());
        else
            return QString();
    }
}

bool Qtilities::CoreGui::AbstractTreeItem::setName(const QString& new_name, Observer *parent) {
    Q_ASSERT(getTreeItemObjectBase());
    if (!getTreeItemObjectBase())
        return false;

    if (!parent) {
        // Try to find the parent through the qti_prop_NAME_MANAGER_ID property:
        SharedProperty name_manager_prop = ObjectManager::getSharedProperty(getTreeItemObjectBase(),qti_prop_NAME_MANAGER_ID);
        if (name_manager_prop.isValid()) {
            int id = name_manager_prop.value().toInt();
            Observer* obs = OBJECT_MANAGER->observerReference(id);
            if (obs)
                parent = obs;
        }
    }

    if (!parent) {
        // In this case, just get the qti_prop_NAME property and set it to the new name:
        SharedProperty name_prop = ObjectManager::getSharedProperty(getTreeItemObjectBase(),qti_prop_NAME);
        if (name_prop.isValid()) {
            getTreeItemObjectBase()->setObjectName(new_name);
            name_prop.setValue(new_name);
            return ObjectManager::setSharedProperty(getTreeItemObjectBase(),name_prop);
        } else {
            getTreeItemObjectBase()->setObjectName(new_name);
            return true;
        }
    } else {
        // First check if this item has a name manager, if not we just need to set the object name:
        // If it has a name manager, it will have the qti_prop_NAME property.
        QVariant name_property = parent->getMultiContextPropertyValue(getTreeItemObjectBase(),qti_prop_NAME);
        if (!name_property.isValid()) {
            // Just set the object name, we also let views know that the data in the parent context needs to be updated:
            getTreeItemObjectBase()->setObjectName(new_name);
            if (parent)
                parent->refreshViewsData();
            return true;
        } else {
            // If there is an qti_prop_NAME property we need to check if parent is the name manager or not.
            // Three things can happen in here:
            // 1. Parent is the name manager
            // 2. Parent uses an instance name
            // 3. Parent does not have a naming policy filter, in that case we need to set qti_prop_NAME and the name manager
            //    will take care of the rest.

            // Get the naming policy filter:
            NamingPolicyFilter* filter = 0;
            for (int i = 0; i < parent->subjectFilters().count(); ++i) {
                NamingPolicyFilter* naming_filter = qobject_cast<NamingPolicyFilter*> (parent->subjectFilters().at(i));
                if (naming_filter)
                    filter = naming_filter;
            }

            if (filter) {
                if (filter->isObjectNameManager(getTreeItemObjectBase())) {
                    // Case 1:
                    // We use the qti_prop_NAME property:
                    QVariant object_name_prop;
                    object_name_prop = getTreeItemObjectBase()->property(qti_prop_NAME);
                    if (object_name_prop.isValid() && object_name_prop.canConvert<SharedProperty>()) {
                        SharedProperty name_property(qti_prop_NAME,QVariant(new_name));
                        return ObjectManager::setSharedProperty(getTreeItemObjectBase(),name_property);
                    }
                } else {
                    // We use the qti_prop_ALIAS_MAP property:
                    // Case 2:
                    QVariant instance_names_prop;
                    instance_names_prop = getTreeItemObjectBase()->property(qti_prop_ALIAS_MAP);
                    if (instance_names_prop.isValid() && instance_names_prop.canConvert<MultiContextProperty>()) {
                        MultiContextProperty new_instance_name = instance_names_prop.value<MultiContextProperty>();
                        new_instance_name.setValue(QVariant(new_name),parent->observerID());
                        return ObjectManager::setMultiContextProperty(getTreeItemObjectBase(),new_instance_name);
                    }
                }

            } else {
                // Case 3:
                // We use the qti_prop_NAME property:
                QVariant object_name_prop;
                object_name_prop = getTreeItemObjectBase()->property(qti_prop_NAME);
                if (object_name_prop.isValid() && object_name_prop.canConvert<SharedProperty>()) {
                    SharedProperty name_property(qti_prop_NAME,QVariant(new_name));
                    return ObjectManager::setSharedProperty(getTreeItemObjectBase(),name_property);
                }
            }
        }
    }

    return false;
}

IExportable::ExportResultFlags Qtilities::CoreGui::AbstractTreeItem::saveFormattingToXML(QXmlStreamWriter* doc, Qtilities::ExportVersion version) const {
    Q_UNUSED(version)

    bool hasAlignment_ = hasAlignment();
    bool hasBackgroundRole_ = hasBackgroundRole();
    bool hasForegroundRole_ = hasForegroundRole();
    bool hasStatusTip_ = hasStatusTip();
    bool hasToolTip_ = hasToolTip();
    bool hasWhatsThis_ = hasWhatsThis();
    bool hasSizeHint_ = hasSizeHint();
    bool hasFont_ = hasFont();

    if (hasAlignment_ || hasBackgroundRole_ || hasForegroundRole_ || hasStatusTip_ || hasToolTip_ || hasWhatsThis_ || hasSizeHint_ || hasFont_) {
        doc->writeStartElement("Formatting");

        if (hasAlignment_)
            doc->writeAttribute("Alignment", QString::number((int) getAlignment()));

        if (hasBackgroundRole_)
            doc->writeAttribute("BackgroundColor",getBackgroundRole().color().name());

        if (hasForegroundRole_)
            doc->writeAttribute("ForegroundColor",getForegroundRole().color().name());

        if (hasStatusTip_)
            doc->writeAttribute("StatusTip",getStatusTip());

        if (hasToolTip_)
            doc->writeAttribute("ToolTip",getToolTip());

        if (hasWhatsThis_)
            doc->writeAttribute("WhatsThis",getWhatsThis());

        if (hasSizeHint_) {
            doc->writeEmptyElement("Size");
            QSize size = getSizeHint();

            doc->writeAttribute("Width",QString::number(size.width()));
            doc->writeAttribute("Height",QString::number(size.height()));
        }

        if (hasFont_) {
            doc->writeEmptyElement("Font");

            QFont font = getFont();
            doc->writeAttribute("Family",font.family());
            doc->writeAttribute("PointSize",QString::number(font.pointSize()));
            doc->writeAttribute("Weight",QString::number(font.weight()));

            doc->writeAttribute("Bold", (font.bold() ? "True" : "False"));
            doc->writeAttribute("Italic", (font.italic() ? "True" : "False"));
        }

        doc->writeEndElement(/* Formatting */);
    }

    return IExportable::Complete;
}

IExportable::ExportResultFlags Qtilities::CoreGui::AbstractTreeItem::loadFormattingFromXML(QXmlStreamReader* doc, Qtilities::ExportVersion version) {
    Q_UNUSED(version)

    // First get all attributes:
    QXmlStreamAttributes attributes = doc->attributes();

    if (attributes.hasAttribute("Alignment")) {
        setAlignment((Qt::AlignmentFlag) (attributes.value("Alignment").toInt()));
    }

    if (attributes.hasAttribute("BackgroundColor")) {
        QStringRef color_name = attributes.value("BackgroundColor");
        setBackgroundRole(QBrush(QColor(color_name.toString())));
    }

    if (attributes.hasAttribute("ForegroundColor")) {
        QStringRef color_name = attributes.value("ForegroundColor");
        setForegroundRole(QBrush(QColor(color_name.toString())));
    }

    if (attributes.hasAttribute("StatusTip")) {
        setStatusTip(attributes.value("StatusTip").toString());
    }

    if (attributes.hasAttribute("ToolTip")) {
        setToolTip(attributes.value("ToolTip").toString());
    }

    if (attributes.hasAttribute("WhatsThis")) {
        setWhatsThis(attributes.value("WhatsThis").toString());
    }

    // Next get all child nodes:
    while (doc->readNext() != QXmlStreamReader::EndElement) {
        QStringRef tag = doc->name();
        QXmlStreamAttributes attr2 = doc->attributes();

        if (tag == "Size") {
            int width = attr2.value("Width").toInt();
            int height = attr2.value("Height").toInt();
            QSize size(width,height);
            setSizeHint(size);

            doc->skipCurrentElement();
        }
        else if (tag == "Font") {
            if (attr2.hasAttribute("Family")) {
                QStringRef family = attr2.value("Family");
                int point_size = -1;
                int weight = -1;
                bool italic = false;

                if (attr2.hasAttribute("PointSize")) {
                    point_size = attr2.value("PointSize").toInt();
                }

                if (attr2.hasAttribute("Weight")) {
                    weight = attr2.value("Weight").toInt();
                }

                if (attr2.hasAttribute("Italic")) {
                    italic = (attr2.value("Italic") == "True");
                }

                QFont font(family.toString(),point_size,weight,italic);
                if (attr2.hasAttribute("Bold")) {
                    font.setBold(attr2.value("Bold") == QLatin1String("True"));
                }

                setFont(font);
            }

            doc->skipCurrentElement();
        }
    }

    return IExportable::Complete;
}

bool Qtilities::CoreGui::AbstractTreeItem::setCategory(const QtilitiesCategory& category, TreeNode* tree_node) {
    if (!category.isValid() || !tree_node)
        return false;

    return setCategory(category,tree_node->observerID());
}

Qtilities::Core::QtilitiesCategory Qtilities::CoreGui::AbstractTreeItem::getCategory(TreeNode* tree_node) const {
    if (!tree_node)
        return QtilitiesCategory();
    else
        return getCategory(tree_node->observerID());
}

QString Qtilities::CoreGui::AbstractTreeItem::getCategoryString(const QString& sep, int observer_id) const {
    return getCategory(observer_id).toString(sep);
}

bool Qtilities::CoreGui::AbstractTreeItem::setCategoryString(const QString& category_string, const QString& sep) {
    return setCategory(QtilitiesCategory(category_string,sep));
}

bool Qtilities::CoreGui::AbstractTreeItem::removeCategory(int observer_id) {
    if (observer_id == -1) {
        if (ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_CATEGORY_MAP)) {
            getTreeItemObjectBase()->setProperty(qti_prop_CATEGORY_MAP,QVariant());
            return true;
        } else
            return false;
    }

    // Remove all the contexts first.
    MultiContextProperty prop = ObjectManager::getMultiContextProperty(getTreeItemObjectBase(), qti_prop_CATEGORY_MAP);
    if (prop.isValid()) {
        // If it exists, we remove this observer context:
        prop.removeContext(observer_id);
        ObjectManager::setMultiContextProperty(getTreeItemObjectBase(), prop);
        return true;
    } else
        return false;
}

bool Qtilities::CoreGui::AbstractTreeItem::setCategory(const QtilitiesCategory& category, int observer_id) {
    // When observer_id = -1, the function will find the parent in the following way:t in the following way:
    // - It will check if the item has only one parent. If that is the case, the category in this parent will be used. that the item only has 1 parent and it will check if it has only one
    //   parent and if so, return the category for that parent.
    // - If the item has more than one parent, the function will check if the item has a specific parent (thus it was attached using Observer::SpecificObserverOwnership). If it does have
    //   a specific parent, the category in this parent will be used.

    // If none of the above conditions were met the function will print an error message in release mode and assert in debug mode.
    if (observer_id == -1) {
        Observer* parent_observer = 0;

        // Do the above checks:
        // (1) Check if it has only one parent:
        int parent_count = Observer::parentCount(getTreeItemObjectBase());
        if (parent_count == 1) {
            MultiContextProperty prop = ObjectManager::getMultiContextProperty(getTreeItemObjectBase(),qti_prop_OBSERVER_MAP);
            if (prop.isValid()) {
                observer_id = prop.contextMap().keys().at(0);
                parent_observer = OBJECT_MANAGER->observerReference(observer_id);
            }
        } else if (parent_count > 1 && getTreeItemObjectBase()) {
            // (2) Check if we have a specific observer parent:
            parent_observer = qobject_cast<Observer*> (getTreeItemObjectBase()->parent());
        }

        if (parent_observer != 0) {
            QObject* obj = getTreeItemObjectBase();
            if (obj) {
                // Check if the category changed, if not we don't set it again.
                // Setting it again will trigger a view refresh which should be avoided if possible.
                QVariant category_variant = parent_observer->getMultiContextPropertyValue(obj,qti_prop_CATEGORY_MAP);
                if (category_variant.isValid()) {
                    QtilitiesCategory old_category = category_variant.value<QtilitiesCategory>();
                    if (old_category == category)
                        return true;
                }

                // Ok it changed, thus set it again:
                if (ObjectManager::propertyExists(obj,qti_prop_CATEGORY_MAP)) {
                    MultiContextProperty category_property = ObjectManager::getMultiContextProperty(obj,qti_prop_CATEGORY_MAP);
                    category_property.setValue(qVariantFromValue(category),parent_observer->observerID());
                    ObjectManager::setMultiContextProperty(obj,category_property);
                } else {
                    MultiContextProperty category_property(qti_prop_CATEGORY_MAP);
                    category_property.setValue(qVariantFromValue(category),parent_observer->observerID());
                    ObjectManager::setMultiContextProperty(obj,category_property);
                }
                return true;
            }
        } else {
            LOG_DEBUG(QString("setCategory(-1) on item \"%1\" failed, the item has %2 parent(s) and/or a specific parent could not be found.").arg(getTreeItemObjectBase()->objectName()).arg(parent_count));
        }
    } else {
        QObject* obj = getTreeItemObjectBase();
        Observer* obs = OBJECT_MANAGER->observerReference(observer_id);
        if (obj && obs) {
            // Check if the category changed, if not we don't set it again.
            // Setting it again will trigger a view refresh which should be avoided if possible.
            QVariant category_variant = obs->getMultiContextPropertyValue(obj,qti_prop_CATEGORY_MAP);
            if (category_variant.isValid()) {
                QtilitiesCategory old_category = category_variant.value<QtilitiesCategory>();
                if (old_category == category)
                    return true;
            }

            // Ok it changed, thus set it again:
            if (ObjectManager::propertyExists(obj,qti_prop_CATEGORY_MAP)) {
                MultiContextProperty category_property = ObjectManager::getMultiContextProperty(obj,qti_prop_CATEGORY_MAP);
                category_property.setValue(qVariantFromValue(category),observer_id);
                ObjectManager::setMultiContextProperty(obj,category_property);
            } else {
                MultiContextProperty category_property(qti_prop_CATEGORY_MAP);
                category_property.setValue(qVariantFromValue(category),observer_id);
                ObjectManager::setMultiContextProperty(obj,category_property);
            }
            return true;
        }
    }

    return false;
}

QtilitiesCategory Qtilities::CoreGui::AbstractTreeItem::getCategory(int observer_id) const {
    // When observer_id = -1, the function will find the parent in the following way:t in the following way:
    // - It will check if the item has only one parent. If that is the case, the category in this parent will be used. that the item only has 1 parent and it will check if it has only one
    //   parent and if so, return the category for that parent.
    // - If the item has more than one parent, the function will check if the item has a specific parent (thus it was attached using Observer::SpecificObserverOwnership). If it does have
    //   a specific parent, the category in this parent will be used.

    // If none of the above conditions were met the function will print an error message in release mode and assert in debug mode.
    if (observer_id == -1) {
        Observer* parent_observer = 0;

        // Do the above checks:
        // (1) Check if it has only one parent:
        int parent_count = Observer::parentCount(getTreeItemObjectBase());
        if (parent_count == 1) {
            MultiContextProperty prop = ObjectManager::getMultiContextProperty(getTreeItemObjectBase(),qti_prop_OBSERVER_MAP);
            if (prop.isValid()) {
                observer_id = prop.contextMap().keys().at(0);
                parent_observer = OBJECT_MANAGER->observerReference(observer_id);
            }
        } else if (parent_count > 1 && getTreeItemObjectBase()) {
            // (2) Check if we have a specific observer parent:
            parent_observer = qobject_cast<Observer*> (getTreeItemObjectBase()->parent());
        }

        if (parent_observer != 0) {
            QVariant category_variant = parent_observer->getMultiContextPropertyValue(getTreeItemObjectBase(),qti_prop_CATEGORY_MAP);
            if (category_variant.isValid())
                return category_variant.value<QtilitiesCategory>();
        } else {
            LOG_DEBUG(QString("getCategory(-1) on item \"%1\" failed, the item has %2 parent(s) and/or a specific parent could not be found.").arg(getTreeItemObjectBase()->objectName()).arg(parent_count));
        }
    } else {
        const QObject* obj = getTreeItemObjectBase();
        Observer* obs = OBJECT_MANAGER->observerReference(observer_id);
        if (obj && obs) {
            QVariant category_variant = obs->getMultiContextPropertyValue(obj,qti_prop_CATEGORY_MAP);
            if (category_variant.isValid()) {
                return category_variant.value<QtilitiesCategory>();
            }
        }
    }

    return QtilitiesCategory();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasCategory() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_CATEGORY_MAP);
}

void Qtilities::CoreGui::AbstractTreeItem::setToolTip(const QString& tooltip) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_TOOLTIP,tooltip);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QString Qtilities::CoreGui::AbstractTreeItem::getToolTip() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj)
        return ObjectManager::getSharedProperty(obj,qti_prop_TOOLTIP).value().toString();

    return QString();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasToolTip() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_TOOLTIP);
}

void Qtilities::CoreGui::AbstractTreeItem::setIcon(const QIcon& icon) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        if (icon.isNull()) {
            obj->setProperty(qti_prop_DECORATION,QVariant());
        } else {
            SharedProperty property(qti_prop_DECORATION,icon);
            ObjectManager::setSharedProperty(obj,property);
        }
    }
}

QIcon Qtilities::CoreGui::AbstractTreeItem::getIcon() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj) {
        QVariant variant = ObjectManager::getSharedProperty(obj,qti_prop_DECORATION).value();
        return variant.value<QIcon>();
    }

    return QIcon();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasIcon() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_DECORATION);
}

void Qtilities::CoreGui::AbstractTreeItem::setWhatsThis(const QString& whats_this) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_WHATS_THIS,whats_this);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QString Qtilities::CoreGui::AbstractTreeItem::getWhatsThis() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj)
        return ObjectManager::getSharedProperty(obj,qti_prop_WHATS_THIS).value().toString();

    return QString();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasWhatsThis() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_WHATS_THIS);
}

void Qtilities::CoreGui::AbstractTreeItem::setStatusTip(const QString& status_tip) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_STATUSTIP,status_tip);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QString Qtilities::CoreGui::AbstractTreeItem::getStatusTip() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj)
        return ObjectManager::getSharedProperty(obj,qti_prop_STATUSTIP).value().toString();

    return QString();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasStatusTip() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_STATUSTIP);
}

void Qtilities::CoreGui::AbstractTreeItem::setSizeHint(const QSize& size) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        if (size.isValid()) {
            SharedProperty property(qti_prop_SIZE_HINT,size);
            ObjectManager::setSharedProperty(obj,property);
        }
    }
}

QSize Qtilities::CoreGui::AbstractTreeItem::getSizeHint() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj)
        return ObjectManager::getSharedProperty(obj,qti_prop_SIZE_HINT).value().toSize();

    return QSize();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasSizeHint() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_SIZE_HINT);
}

void Qtilities::CoreGui::AbstractTreeItem::setFont(const QFont& font) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_FONT,font);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QFont Qtilities::CoreGui::AbstractTreeItem::getFont() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj) {
        QVariant variant = ObjectManager::getSharedProperty(obj,qti_prop_FONT).value();
        return variant.value<QFont>();
    }

    return QFont();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasFont() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_FONT);
}

void Qtilities::CoreGui::AbstractTreeItem::setAlignment(const Qt::AlignmentFlag& alignment) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_TEXT_ALIGNMENT,(int) alignment);
        ObjectManager::setSharedProperty(obj,property);
    }
}

Qt::AlignmentFlag Qtilities::CoreGui::AbstractTreeItem::getAlignment() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj) {
        QVariant variant = ObjectManager::getSharedProperty(obj,qti_prop_TEXT_ALIGNMENT).value();
        return (Qt::AlignmentFlag) variant.toInt();
    }

    return Qt::AlignLeft;
}

bool Qtilities::CoreGui::AbstractTreeItem::hasAlignment() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_TEXT_ALIGNMENT);
}

void Qtilities::CoreGui::AbstractTreeItem::setForegroundRole(const QBrush& foreground_role) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_FOREGROUND,foreground_role);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QBrush Qtilities::CoreGui::AbstractTreeItem::getForegroundRole() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj) {
        QVariant variant = ObjectManager::getSharedProperty(obj,qti_prop_FOREGROUND).value();
        return variant.value<QBrush>();
    }

    return QBrush();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasForegroundRole() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_FOREGROUND);
}

void Qtilities::CoreGui::AbstractTreeItem::setForegroundColor(const QColor& color) {
    if (hasForegroundRole()) {
        QBrush brush = getForegroundRole();
        brush.setColor(color);
        setForegroundRole(brush);
    } else
        setForegroundRole(QBrush(color));
}

QColor Qtilities::CoreGui::AbstractTreeItem::getForegroundColor() const {
    if (hasForegroundRole()) {
        QBrush brush = getForegroundRole();
        return brush.color();
    } else
        return QColor();
}

void Qtilities::CoreGui::AbstractTreeItem::setBackgroundRole(const QBrush& background_role) {
    QObject* obj = getTreeItemObjectBase();
    if (obj) {
        SharedProperty property(qti_prop_BACKGROUND,background_role);
        ObjectManager::setSharedProperty(obj,property);
    }
}

QBrush Qtilities::CoreGui::AbstractTreeItem::getBackgroundRole() const {
    const QObject* obj = getTreeItemObjectBase();
    if (obj) {
        QVariant variant = ObjectManager::getSharedProperty(obj,qti_prop_BACKGROUND).value();
        return variant.value<QBrush>();
    }

    return QBrush();
}

bool Qtilities::CoreGui::AbstractTreeItem::hasBackgroundRole() const {
    return ObjectManager::propertyExists(getTreeItemObjectBase(),qti_prop_BACKGROUND);
}

void Qtilities::CoreGui::AbstractTreeItem::setBackgroundColor(const QColor& color) {
    if (hasBackgroundRole()) {
        QBrush brush = getBackgroundRole();
        brush.setColor(color);
        setBackgroundRole(brush);
    } else
        setBackgroundRole(QBrush(color));
}

QColor Qtilities::CoreGui::AbstractTreeItem::getBackgroundColor() const {
    if (hasBackgroundRole()) {
        QBrush brush = getBackgroundRole();
        return brush.color();
    } else
        return QColor(Qt::white);
}
