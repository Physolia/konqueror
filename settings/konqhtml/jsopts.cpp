/*
    SPDX-FileCopyrightText: 1996 Martin R. Jones
    SPDX-FileCopyrightText: 1998 Bernd Wuebben

    SPDX-FileCopyrightText: 1998 Torben Weis
    KControl port & modifications

    SPDX-FileCopyrightText: 1998 David Faure
    End of the KControl port, added 'kfmclient configure' call.

    SPDX-FileCopyrightText: 2000 Kalle Dalheimer
    New configuration scheme for JavaScript

    SPDX-FileCopyrightText: 2000 Daniel Molkentin
    Major cleanup & Java/JS settings split

    SPDX-FileCopyrightText: 2002-2003 Leo Savernik
    Big changes to accommodate per-domain settings

*/

// Own
#include "jsopts.h"

// Qt
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
// KDE
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kurlrequester.h>
#include <KLocalizedString>

// Local
#include "htmlopts.h"
#include "policydlg.h"
#include <htmlextension.h>

// == class KJavaScriptOptions =====
KJavaScriptOptions::KJavaScriptOptions(KSharedConfig::Ptr config, const QString &group, QWidget *parent) :
    KCModule(parent),
    _removeJavaScriptDomainAdvice(false),
    m_pConfig(config), m_groupname(group),
    js_global_policies(config, group, true, QString()),
    _removeECMADomainSettings(false)
{
    QVBoxLayout *toplevel = new QVBoxLayout(this);

    enableJavaScriptGloballyCB = new QCheckBox(i18n("Ena&ble JavaScript globally"));
    enableJavaScriptGloballyCB->setToolTip(i18n("Enables the execution of scripts written in ECMA-Script "
            "(also known as JavaScript) that can be contained in HTML pages. "
            "Note that, as with any browser, enabling scripting languages can be a security problem."));
    connect(enableJavaScriptGloballyCB, &QAbstractButton::clicked, this, &KJavaScriptOptions::markAsChanged);
    connect(enableJavaScriptGloballyCB, &QAbstractButton::clicked, this, &KJavaScriptOptions::slotChangeJSEnabled);
    toplevel->addWidget(enableJavaScriptGloballyCB);

    // the global checkbox
    QGroupBox *globalGB = new QGroupBox(i18n("Debugging"));
    QHBoxLayout *hbox = new QHBoxLayout(globalGB);
    toplevel->addWidget(globalGB);

    jsDebugWindow = new QCheckBox(i18n("Enable debu&gger"));
    jsDebugWindow->setToolTip(i18n("Enables builtin JavaScript debugger."));
    connect(jsDebugWindow, &QAbstractButton::clicked, this, &KJavaScriptOptions::markAsChanged);
    hbox->addWidget(jsDebugWindow);

    reportErrorsCB = new QCheckBox(i18n("Report &errors"));
    reportErrorsCB->setToolTip(i18n("Enables the reporting of errors that occur when JavaScript "
                                      "code is executed."));
    connect(reportErrorsCB, &QAbstractButton::clicked, this, &KJavaScriptOptions::markAsChanged);
    hbox->addWidget(reportErrorsCB);

    // the domain-specific listview
    domainSpecific = new JSDomainListView(m_pConfig, m_groupname, this, this);
    connect(domainSpecific, &DomainListView::changed, this, &KJavaScriptOptions::markAsChanged);
    toplevel->addWidget(domainSpecific, 2);

    domainSpecific->setToolTip(i18n("Here you can set specific JavaScript policies for any particular "
                                      "host or domain. To add a new policy, simply click the <i>New...</i> "
                                      "button and supply the necessary information requested by the "
                                      "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                      "button and choose the new policy from the policy dialog box. Clicking "
                                      "on the <i>Delete</i> button will remove the selected policy causing the default "
                                      "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                      "button allows you to easily share your policies with other people by allowing "
                                      "you to save and retrieve them from a zipped file."));

    QString wtstr = i18n("<p>This box contains the domains and hosts you have set "
                         "a specific JavaScript policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                         "domains or hosts.</p><p>Select a policy and use the controls on "
                         "the right to modify it.</p>");
    domainSpecific->listView()->setToolTip(wtstr);

    domainSpecific->importButton()->setToolTip(i18n("Click this button to choose the file that contains "
            "the JavaScript policies. These policies will be merged "
            "with the existing ones. Duplicate entries are ignored."));
    domainSpecific->exportButton()->setToolTip(i18n("Click this button to save the JavaScript policy to a zipped "
            "file. The file, named <b>javascript_policy.tgz</b>, will be "
            "saved to a location of your choice."));

    // the frame containing the JavaScript policies settings
    js_policies_frame = new JSPoliciesFrame(&js_global_policies,
                                            i18n("Global JavaScript Policies"), this);
    toplevel->addWidget(js_policies_frame);
    connect(js_policies_frame, &JSPoliciesFrame::changed, this, &KJavaScriptOptions::markAsChanged);

}

void KJavaScriptOptions::load()
{
    // *** load ***
    KConfigGroup cg(m_pConfig, m_groupname);
    if (cg.hasKey("ECMADomains")) {
        domainSpecific->initialize(cg.readEntry("ECMADomains", QStringList()));
    } else if (cg.hasKey("ECMADomainSettings")) {
        domainSpecific->updateDomainListLegacy(cg.readEntry("ECMADomainSettings", QStringList()));
        _removeECMADomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(cg.readEntry("JavaScriptDomainAdvice", QStringList()));
        _removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(
        js_global_policies.isFeatureEnabled());
    reportErrorsCB->setChecked(cg.readEntry("ReportJavaScriptErrors", false));
    jsDebugWindow->setChecked(cg.readEntry("EnableJavaScriptDebug", false));
//    js_popup->setButton( m_pConfig->readUnsignedNumEntry("WindowOpenPolicy", 0) );
    emit changed(false);
}

void KJavaScriptOptions::defaults()
{
    js_policies_frame->defaults();
    enableJavaScriptGloballyCB->setChecked(
        js_global_policies.isFeatureEnabled());
    reportErrorsCB->setChecked(false);
    jsDebugWindow->setChecked(false);
    emit changed(true);
}

void KJavaScriptOptions::save()
{
    KConfigGroup cg(m_pConfig, m_groupname);
    cg.writeEntry("ReportJavaScriptErrors", reportErrorsCB->isChecked());
    cg.writeEntry("EnableJavaScriptDebug", jsDebugWindow->isChecked());

    domainSpecific->save(m_groupname, QStringLiteral("ECMADomains"));
    js_policies_frame->save();

    if (_removeECMADomainSettings) {
        cg.deleteEntry("ECMADomainSettings");
        _removeECMADomainSettings = false;
    }

    // sync moved to KJSParts::save
//    cg.sync();
    emit changed(false);
}

void KJavaScriptOptions::slotChangeJSEnabled()
{
    js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

// == class JSDomainListView =====

JSDomainListView::JSDomainListView(KSharedConfig::Ptr config, const QString &group,
                                   KJavaScriptOptions *options, QWidget *parent)
    : DomainListView(config, i18nc("@title:group", "Do&main-Specific"), parent),
      group(group), options(options)
{
}

JSDomainListView::~JSDomainListView()
{
}

void JSDomainListView::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(config, group, false);
    pol.defaults();
    for (QStringList::ConstIterator it = domainConfig.begin();
            it != domainConfig.end(); ++it) {
        QString domain;
        HtmlSettingsInterface::JavaScriptAdvice javaAdvice;
        HtmlSettingsInterface::JavaScriptAdvice javaScriptAdvice;
        HtmlSettingsInterface::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
        if (javaScriptAdvice != HtmlSettingsInterface::JavaScriptDunno) {
            QTreeWidgetItem *index =
                new QTreeWidgetItem(domainSpecificLV, QStringList() << domain <<
                                    i18n(HtmlSettingsInterface::javascriptAdviceToText(javaScriptAdvice)));

            pol.setDomain(domain);
            pol.setFeatureEnabled(javaScriptAdvice != HtmlSettingsInterface::JavaScriptReject);
            domainPolicies[index] = new JSPolicies(pol);
        }
    }
}

void JSDomainListView::setupPolicyDlg(PushButton trigger, PolicyDialog &pDlg,
                                      Policies *pol)
{
    JSPolicies *jspol = static_cast<JSPolicies *>(pol);
    QString caption;
    switch (trigger) {
    case AddButton:
        caption = i18nc("@title:window", "New JavaScript Policy");
        jspol->setFeatureEnabled(!options->enableJavaScriptGloballyCB->isChecked());
        break;
    case ChangeButton: caption = i18nc("@title:window", "Change JavaScript Policy"); break;
    default:; // inhibit gcc warning
    }/*end switch*/
    pDlg.setWindowTitle(caption);
    pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
    pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                         "the above host or domain."));
    JSPoliciesFrame *panel = new JSPoliciesFrame(jspol, i18n("Domain-Specific "
            "JavaScript Policies"), &pDlg);
    panel->refresh();
    pDlg.addPolicyPanel(panel);
    pDlg.refresh();
}

JSPolicies *JSDomainListView::createPolicies()
{
    return new JSPolicies(config, group, false);
}

JSPolicies *JSDomainListView::copyPolicies(Policies *pol)
{
    return new JSPolicies(*static_cast<JSPolicies *>(pol));
}

