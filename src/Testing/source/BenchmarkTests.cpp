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

#include "BenchmarkTests.h"

#include <QXmlStreamWriter>

#include <QtilitiesCoreGui>
using namespace QtilitiesCoreGui;

int Qtilities::Testing::BenchmarkTests::execTest(int argc, char ** argv) {
    return QTest::qExec(this,argc,argv);
}

void Qtilities::Testing::BenchmarkTests::benchmarkObserverExport_1_0_1_0_data() {
    QTest::addColumn<int>("TreeDepth");
    QTest::newRow("small tree") << 5;
    QTest::newRow("big tree") << 50;
}

void Qtilities::Testing::BenchmarkTests::benchmarkObserverExport_1_0_1_0() {
    QFETCH(int, TreeDepth);

    // ---------------------------------------------------
    // Test export only with categories:
    // ---------------------------------------------------
    TreeNode* obj_source = new TreeNode("Root Node");

    // Build a tree:
    for (int i = 0; i < TreeDepth; ++i) {
        TreeNode* child_node = obj_source->addNode("TestNode" + QString::number(i));
        for (int r = 0; r < TreeDepth; r++) {
            child_node->addItem("TestChild_" + QString::number(i) + "_" + QString::number(r));
        }
    }

    // Do the export:
    QFile file("testObserverDataOnlyWithCategoriesBenchmark_0_3_0_3.xml");
    file.open(QIODevice::WriteOnly);

    QXmlStreamWriter doc(&file);
    doc.setAutoFormatting(true);
    doc.setAutoFormattingIndent(2);
    doc.writeComment("<!--Created by " + QApplication::applicationName() + " v" + QApplication::applicationVersion() + " on " + QDateTime::currentDateTime().toString() + "-->\n");
    doc.writeStartDocument("1.0");
    doc.writeAttribute("encoding", "UTF-8");
    doc.writeStartElement("QtilitiesTesting");
    doc.writeStartElement("object_node");

    obj_source->setExportVersion(Qtilities::Qtilities_1_0);


    QBENCHMARK {
        QVERIFY(obj_source->exportXml(&doc) == IExportable::Complete);
    }

    file.close();

    // Don't delete it here since deletion will make the test slower. Thus we don't care about the memory leaks.
    // delete obj_source;
}

void Qtilities::Testing::BenchmarkTests::benchmarkObserverImport_1_0_1_0() {
    // ---------------------------------------------------
    // Test export only with categories:
    // ---------------------------------------------------
    TreeNode* obj_import_xml = new TreeNode;

    QFile file("testObserverDataOnlyWithCategoriesBenchmark_0_3_0_3.xml");
    file.open(QIODevice::ReadWrite);

    QXmlStreamReader doc(&file);

    // Do the import:
    obj_import_xml->setExportVersion(Qtilities::Qtilities_1_0);
    QList<QPointer<QObject> > import_list;

    QVERIFY(obj_import_xml->importXml(&doc,import_list) == IExportable::Complete);

    file.close();
    delete obj_import_xml;
}
