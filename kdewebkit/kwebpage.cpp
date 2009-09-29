/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "kwebpage.h"
#include "kwebview.h"

#include "kwebpluginfactory.h"
#include "settings/webkitsettings.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KIO/Job>
#include <KDE/KUrl>
#include <KIO/AccessManager>

#include <QtCore/QPair>
#include <QtCore/QPointer>
#include <QtGui/QTextDocument>
#include <QtGui/QPaintEngine>
#include <QtWebKit/QWebFrame>
#include <QtUiTools/QUiLoader>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkCookieJar>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>


/* Null network reply */
class NullNetworkReply : public QNetworkReply
{
public:
    NullNetworkReply() {
        setHeader(QNetworkRequest::ContentLengthHeader, 0);
        setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
        QTimer::singleShot(0, this, SIGNAL(finished()));
    }
    virtual void abort() {}
    virtual qint64 bytesAvailable() const {
        return 0;
    }
protected:
    virtual qint64 readData(char* data, qint64) {
        qMemCopy(data, "\0", 1); return 0;
    }
};

/* Re-implementation of QNetworkAccessManager for integration with KIO. */
class NetworkAccessManager : public KIO::AccessManager
{
public:
    NetworkAccessManager(QObject *parent) : KIO::AccessManager(parent) {}
    KIO::MetaData& sessionMetaData() {
        return m_sessionMetaData;
    }

    KIO::MetaData& requestMetaData() {
        return m_requestMetaData;
    }

protected:
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0) {

        if (WebKitSettings::self()->isAdFilterEnabled() && WebKitSettings::self()->isAdFiltered(req.url().toString())) {
            kDebug() << "*** AD FILTER BLOCKED => " << req.url();
            return new NullNetworkReply();
        }

        QNetworkRequest request(req);
        KIO::MetaData metaData = m_sessionMetaData;
        metaData += m_requestMetaData;

        QVariant attr = req.attribute(QNetworkRequest::User);
        if (attr.isValid() && attr.type() == QVariant::Map) {
            metaData += attr.toMap();
        }

        if (!metaData.isEmpty()) {
            attr = metaData.toVariant();
            request.setAttribute(QNetworkRequest::User, attr);
        }

        // Clear the per request meta data...
        m_requestMetaData.clear();
        return KIO::AccessManager::createRequest(op, request, outgoingData);
    }
private:
    KIO::MetaData m_sessionMetaData;
    KIO::MetaData m_requestMetaData;
};

/* Re-implementation of QNetworkCookieJar for integration with KCookieJar */
class CookieJar : public QNetworkCookieJar
{
public:
    CookieJar(QObject* parent = 0) : QNetworkCookieJar(parent), m_windowId(-1) {}
    virtual ~CookieJar() {}

    virtual QList<QNetworkCookie> cookiesForUrl(const QUrl & url) const {
        QList<QNetworkCookie> cookieList;

        if (WebKitSettings::self()->isCookieJarEnabled()) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            QDBusReply<QString> reply = kcookiejar.call("findDOMCookies", url.toString(), m_windowId);

            if (reply.isValid()) {
                cookieList << reply.value().toUtf8();
                //kDebug() << url.host() << reply.value();
            } else {
                kWarning() << "Unable to communicate with the cookiejar!";
            }
        }

        return cookieList;
    }

    virtual bool setCookiesFromUrl(const QList<QNetworkCookie> & cookieList, const QUrl & url) {

        if (WebKitSettings::self()->isCookieJarEnabled()) {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");

            QByteArray cookieHeader;
            Q_FOREACH(const QNetworkCookie& cookie, cookieList) {
                cookieHeader = "Set-Cookie: ";
                cookieHeader += cookie.toRawForm();
                kcookiejar.call("addCookies", url.toString(), cookieHeader, m_windowId);
                //kDebug() << "url: " << url.host() << ", cookie: " << cookieHeader;
            }

            return !kcookiejar.lastError().isValid();
        }

        return false;
    }

    void setWindowId(qlonglong id) {
        m_windowId = id;
    }

private:
    qlonglong m_windowId;
};

class KWebPage::KWebPagePrivate
{
public:
    KWebPagePrivate() {}

    QString getFileNameForDownload(const QNetworkRequest &request, QNetworkReply *reply) const;
    QPointer<NetworkAccessManager> accessManager;
};

QString KWebPage::KWebPagePrivate::getFileNameForDownload(const QNetworkRequest &request, QNetworkReply *reply) const
{
    QString fileName = KUrl(request.url()).fileName();
    if (reply && reply->hasRawHeader("Content-Disposition")) { // based on code from arora, downloadmanger.cpp
        const QString value = QLatin1String(reply->rawHeader("Content-Disposition"));
        const int pos = value.indexOf(QLatin1String("filename="));
        if (pos != -1) {
            QString name = value.mid(pos + 9);
            if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
                name = name.mid(1, name.size() - 2);
            fileName = name;
        }
    }
    return fileName;
}

KWebPage::KWebPage(QObject *parent)
        : QWebPage(parent), d(new KWebPage::KWebPagePrivate())
{
    d->accessManager = new NetworkAccessManager(this);
    setNetworkAccessManager(d->accessManager);

    CookieJar* cookiejar = new CookieJar(this);
    KWebView * webView = qobject_cast<KWebView*>(view());

    if (webView) {
        const qlonglong winId = webView->window()->winId();
        cookiejar->setWindowId(winId);
        d->accessManager->sessionMetaData().insert("window-id", QString::number(winId));
    }

    d->accessManager->setCookieJar(cookiejar);

    // TODO: Determine if and for what our own KParts implementation
    // of QWebPluginFactory is necessary...
    //setPluginFactory(new KWebPluginFactory(pluginFactory(), this));

    action(Back)->setIcon(KIcon("go-previous"));
    action(Back)->setShortcut(KStandardShortcut::back().primary());

    action(Forward)->setIcon(KIcon("go-next"));
    action(Forward)->setShortcut(KStandardShortcut::forward().primary());

    action(Reload)->setIcon(KIcon("view-refresh"));
    action(Reload)->setShortcut(KStandardShortcut::reload().primary());

    action(Stop)->setIcon(KIcon("process-stop"));
    action(Stop)->setShortcut(Qt::Key_Escape);

    action(Cut)->setIcon(KIcon("edit-cut"));
    action(Cut)->setShortcut(KStandardShortcut::cut().primary());

    action(Copy)->setIcon(KIcon("edit-copy"));
    action(Copy)->setShortcut(KStandardShortcut::copy().primary());

    action(Paste)->setIcon(KIcon("edit-paste"));
    action(Paste)->setShortcut(KStandardShortcut::paste().primary());

    action(Undo)->setIcon(KIcon("edit-undo"));
    action(Undo)->setShortcut(KStandardShortcut::undo().primary());

    action(Redo)->setIcon(KIcon("edit-redo"));
    action(Redo)->setShortcut(KStandardShortcut::redo().primary());

    action(InspectElement)->setIcon(KIcon("view-process-all"));
    action(OpenLinkInNewWindow)->setIcon(KIcon("window-new"));
    action(OpenFrameInNewWindow)->setIcon(KIcon("window-new"));
    action(OpenImageInNewWindow)->setIcon(KIcon("window-new"));
    action(CopyLinkToClipboard)->setIcon(KIcon("edit-copy"));
    action(CopyImageToClipboard)->setIcon(KIcon("edit-copy"));
    action(ToggleBold)->setIcon(KIcon("format-text-bold"));
    action(ToggleItalic)->setIcon(KIcon("format-text-italic"));
    action(ToggleUnderline)->setIcon(KIcon("format-text-underline"));
    action(DownloadLinkToDisk)->setIcon(KIcon("document-save"));
    action(DownloadImageToDisk)->setIcon(KIcon("document-save"));

    settings()->setWebGraphic(QWebSettings::MissingPluginGraphic, KIcon("preferences-plugin").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::MissingImageGraphic, KIcon("image-missing").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::DefaultFrameIconGraphic, KIcon("applications-internet").pixmap(32, 32));

    if (webView)
        WebKitSettings::self()->computeFontSizes(webView->logicalDpiY());

    //const QString host = mainFrame()->url().host();

    setForwardUnsupportedContent(true);

    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(slotDownloadRequested(const QNetworkRequest &)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotHandleUnsupportedContent(QNetworkReply *)));
}

KWebPage::~KWebPage()
{
    delete d;
}

void KWebPage::setAllowExternalContent(bool allow)
{
    d->accessManager->setExternalContentAllowed(allow);
}

bool KWebPage::isExternalContentAllowed() const
{
    return d->accessManager->isExternalContentAllowed();
}

QString KWebPage::chooseFile(QWebFrame *frame, const QString &suggestedFile)
{
    return KFileDialog::getOpenFileName(suggestedFile, QString(), frame->page()->view());
}

void KWebPage::javaScriptAlert(QWebFrame *frame, const QString &msg)
{
    KMessageBox::error(frame->page()->view(), msg, i18n("JavaScript"));
}

bool KWebPage::javaScriptConfirm(QWebFrame *frame, const QString &msg)
{
    return (KMessageBox::warningYesNo(frame->page()->view(), msg, i18n("JavaScript"),
                                      KStandardGuiItem::ok(), KStandardGuiItem::cancel())
            == KMessageBox::Yes);
}

bool KWebPage::javaScriptPrompt(QWebFrame *frame, const QString &msg, const QString &defaultValue, QString *result)
{
    bool ok = false;

    QString text = KInputDialog::getText(i18n("JavaScript"), msg, defaultValue, &ok, frame->page()->view());

    if (result)
      *result = text;

    return ok;
}

QString KWebPage::userAgentForUrl(const QUrl& _url) const
{
    const KUrl url(_url);
    QString userAgent = KProtocolManager::userAgentForHost((url.isLocalFile() ? "localhost" : url.host()));

    if (userAgent == KProtocolManager::defaultUserAgent())
        userAgent = QWebPage::userAgentForUrl(_url);
    else {
        const int index = userAgent.indexOf("KHTML/");
        if (index > -1) {
          QString webKitUserAgent = QWebPage::userAgentForUrl(_url);
          userAgent = userAgent.left(index);
          webKitUserAgent = webKitUserAgent.mid(webKitUserAgent.indexOf("AppleWebKit/"));
          webKitUserAgent = webKitUserAgent.left(webKitUserAgent.indexOf(')') + 1);
          userAgent += webKitUserAgent;
          userAgent.remove("compatible; ");
        }
    }

    //kDebug() << userAgent;
    return userAgent;
}

void KWebPage::setSessionMetaData(const QString& key, const QString& value)
{
    Q_ASSERT(d->accessManager);
    d->accessManager->sessionMetaData()[key] = value;
}

void KWebPage::setRequestMetaData(const QString& key, const QString& value)
{
    Q_ASSERT(d->accessManager);
    d->accessManager->requestMetaData()[key] = value;
}

bool KWebPage::acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type)
{
    kDebug() << "url: " << request.url() << ", type: " << type << ", frame: " << frame;   
    /*
      QWebPage calls acceptNavigationRequest when:
        ** a load url operation is requested... (e.g. user types in the url)
        ** a link on a web page is clicked...
        ** a "location.href" javascript command is executed...
        ** a load url operation is requested from a frame within the main frame...

        If the navigation request is from the main frame, set the cross-domain meta-data
        value to the current url when for proper integration with KCookieJar...
    */
    if (frame && frame == mainFrame()) {
        QUrl url(request.url());
        if (url.host().toLower() != QString::fromUtf8("blank")) {
            d->accessManager->sessionMetaData()["cross-domain"] = url.toString();
        }
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

QObject *KWebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    kDebug() << "create Plugin requested:";
    kDebug() << "classid:" << classId;
    kDebug() << "url:" << url;
    kDebug() << "paramNames:" << paramNames << " paramValues:" << paramValues;

    QUiLoader loader;
    return loader.createWidget(classId, view());
}

void KWebPage::slotHandleUnsupportedContent(QNetworkReply *reply)
{
    kDebug() << "url:" << reply->url();
    kDebug() << "location:" << reply->header(QNetworkRequest::LocationHeader).toString();
    kDebug() << "error:" << reply->error();

    if (reply->url().isValid()) {
        KParts::BrowserRun::AskSaveResult res = KParts::BrowserRun::askEmbedOrSave(
                                                    reply->url(),
                                                    reply->header(QNetworkRequest::ContentTypeHeader).toString(),
                                                    d->getFileNameForDownload(reply->request(), reply));
        switch (res) {
        case KParts::BrowserRun::Save:
            slotDownloadRequested(reply->request(), reply);
            return;
        case KParts::BrowserRun::Cancel:
            return;
        default: // Open
            break;
        }
    }
}

void KWebPage::slotDownloadRequested(const QNetworkRequest &request)
{
    slotDownloadRequested(request, 0);
}

void KWebPage::slotDownloadRequested(const QNetworkRequest &request, QNetworkReply *reply)
{
    const KUrl url(request.url());
    kDebug() << url;

    const QString fileName = d->getFileNameForDownload(request, reply);

    // parts of following code are based on khtml_ext.cpp
    // DownloadManager <-> konqueror integration
    // find if the integration is enabled
    // the empty key  means no integration
    // only use download manager for non-local urls!
    bool downloadViaKIO = true;
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        const QString downloadManger = cfg.readPathEntry("DownloadManager", QString());
        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The Download Manager (%1) could not be found in your $PATH.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it. \n\nThe integration with Konqueror will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync();
            } else {
                downloadViaKIO = false;
                cmd += ' ' + KShell::quoteArg(url.url());
                kDebug() << "Calling command" << cmd;
                KRun::runCommand(cmd, view());
            }
        }
    }

    if (downloadViaKIO) {
        const QString destUrl = KFileDialog::getSaveFileName(url.fileName(), QString(), view());
        if (destUrl.isEmpty()) return;
        KIO::Job *job = KIO::file_copy(url, KUrl(destUrl), -1, KIO::Overwrite);

        QVariant attr = request.attribute(QNetworkRequest::User);
        if (attr.isValid() && attr.type() == QVariant::Map) {
            KIO::MetaData metaData (attr.toMap());
            job->setMetaData(metaData);
        }
        job->addMetaData("MaxCacheSize", "0"); // Don't store in http cache.
        job->addMetaData("cache", "cache"); // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

KWebPage *KWebPage::createWindow(WebWindowType type)
{
    return newWindow(type);
}

KWebPage *KWebPage::newWindow(WebWindowType type)
{
    Q_UNUSED(type);
    return 0;
}
