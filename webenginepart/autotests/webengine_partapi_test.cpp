/*
    SPDX-FileCopyrightText: 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <webenginepart.h>
#include "webengine_testutils.h"

#include <KIO/Job>
#include <KParts/BrowserExtension>
#include <KPluginMetaData>

#include <QTest>
#include <QObject>
#include <QSignalSpy>
#include <QWebEngineView>
#include <QJsonDocument>

namespace {
KPluginMetaData dummyMetaData()
{
    QJsonObject jo = QJsonDocument::fromJson(
            "{ \"KPlugin\": {\n"
            " \"Id\": \"webenginepart\",\n"
            " \"Name\": \"WebEngine\",\n"
            " \"Version\": \"0.1\"\n"
            "}\n}").object();
    return KPluginMetaData(jo, QString());
}
}

class WebEnginePartApiTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void shouldHaveBrowserExtension();
    void shouldEmitStartedAndCompleted();
    void shouldEmitStartAndCompleteWithPendingAction();
    void shouldEmitSetWindowCaption();
    void shouldEmitOpenUrlNotifyOnClick();

};

void WebEnginePartApiTest::initTestCase()
{
    qRegisterMetaType<KIO::Job *>(); // for the KParts started signal
}

void WebEnginePartApiTest::shouldHaveBrowserExtension()
{
    // GIVEN
    WebEnginePart part(nullptr, nullptr, dummyMetaData());

    // WHEN
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);

    // THEN
    QVERIFY(ext);
}

void WebEnginePartApiTest::shouldEmitStartedAndCompleted()
{
    // GIVEN
    WebEnginePart part(nullptr, nullptr, dummyMetaData());
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed()));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);
    QSignalSpy spyOpenUrlNotify(ext, &KParts::BrowserExtension::openUrlNotify);
    const QUrl url(QStringLiteral("data:text/html, <p>Hello World</p>"));

    // WHEN
    part.openUrl(url);

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spySetWindowCaption.wait());
    QCOMPARE(spySetWindowCaption.at(0).at(0).toUrl().toString(), url.toString());
    QVERIFY(spyCompleted.wait());
    QVERIFY(spyOpenUrlNotify.isEmpty());
}

void WebEnginePartApiTest::shouldEmitStartAndCompleteWithPendingAction()
{
    // GIVEN
    WebEnginePart part(nullptr, nullptr, dummyMetaData());
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completedWithPendingAction()));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);
    QSignalSpy spyOpenUrlNotify(ext, &KParts::BrowserExtension::openUrlNotify);
    const QUrl url(QStringLiteral("data:text/html, <html><head><meta http-equiv=\"refresh\"><body><p>Hello World</p></body></html>"));

    // WHEN
    part.openUrl(url);

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spySetWindowCaption.wait());
    QCOMPARE(spySetWindowCaption.at(0).at(0).toUrl().toString(), url.toString());
    QVERIFY(spyCompleted.wait());
    QVERIFY(spyOpenUrlNotify.isEmpty());
}


void WebEnginePartApiTest::shouldEmitSetWindowCaption()
{
    // GIVEN
    WebEnginePart part(nullptr, nullptr, dummyMetaData());
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed()));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);

    // WHEN opening a URL with a title tag
    part.openUrl(QUrl(QStringLiteral("data:text/html, <title>Custom Title</title><p>Hello World</p>")));

    // THEN
    QVERIFY(spyStarted.wait());
    QVERIFY(spyCompleted.wait());
    QCOMPARE(spySetWindowCaption.count(), 2);
    QCOMPARE(spySetWindowCaption.at(1).at(0).toUrl().toString(), QStringLiteral("Custom Title"));
}

void WebEnginePartApiTest::shouldEmitOpenUrlNotifyOnClick()
{
    // GIVEN
    WebEnginePart part(nullptr, nullptr, dummyMetaData());
    QSignalSpy spyStarted(&part, &KParts::ReadOnlyPart::started);
    QSignalSpy spyCompleted(&part, SIGNAL(completed()));
    QSignalSpy spySetWindowCaption(&part, &KParts::ReadOnlyPart::setWindowCaption);
    KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(&part);
    QSignalSpy spyOpenUrlNotify(ext, &KParts::BrowserExtension::openUrlNotify);
    const QString file = QFINDTESTDATA("data/page-with-link.html");
    QVERIFY(!file.isEmpty());
    const QUrl url = QUrl::fromLocalFile(file);
    part.openUrl(url);
    QVERIFY(spyCompleted.wait());
    QVERIFY(spyOpenUrlNotify.isEmpty());
    QWebEnginePage *page = part.view()->page();
    const QPoint pos = elementCenter(page, QStringLiteral("linkid")); // doesn't seem fully correct...
    part.widget()->show();
    spyCompleted.clear();

    // WHEN clicking on the link
    QTest::mouseClick(part.view()->focusProxy(), Qt::LeftButton, Qt::KeyboardModifiers(), pos);

    // THEN
    QVERIFY(spyCompleted.wait());
    QCOMPARE(spyOpenUrlNotify.count(), 1);
    QCOMPARE(part.url().fileName(), QStringLiteral("hello.html"));
}

QTEST_MAIN(WebEnginePartApiTest)
#include "webengine_partapi_test.moc"
