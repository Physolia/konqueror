/* This file is part of the KDE project
   Copyright (C) 1998-2000 David Faure <faure@kde.org>
                 2003      Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __knewmenu_h
#define __knewmenu_h

#include <qintdict.h>
#include <qstringlist.h>

#include <kaction.h>
#include <kdialogbase.h>
#include <kurl.h>

namespace KIO { class Job; }

class KDirWatch;
class KLineEdit;
class KURLRequester;
class QPopupMenu;

/**
 * The 'New' submenu, both for the File menu and the RMB popup menu.
 * (The same instance can be used by both).
 * Fills it with 'Folder' and one item per Template.
 * For this you need to connect aboutToShow() of the File menu with slotCheckUpToDate()
 * and to call slotCheckUpToDate() before showing the RMB popupmenu.
 *
 * KNewMenu automatically updates the list of templates if templates are
 * added/updated/deleted.
 *
 * @author David Faure <faure@kde.org>
 * Ideas and code for the new template handling mechanism ('link' desktop files)
 * from Christoph Pickart <pickart@iam.uni-bonn.de>
 */
class KNewMenu : public KActionMenu
{
  Q_OBJECT
public:

    /**
     * Constructor
     */
    KNewMenu( KActionCollection * _collec, const char *name=0L );
    KNewMenu( KActionCollection * _collec, QWidget *parentWidget, const char *name=0L );
    virtual ~KNewMenu();

    /**
     * Set the files the popup is shown for
     * Call this before showing up the menu
     */
    void setPopupFiles(KURL::List & _files) {
        popupFiles = _files;
    }
    void setPopupFiles(const KURL & _file) {
        popupFiles.clear();
        popupFiles.append( _file );
    }

public slots:
    /**
     * Checks if updating the list is necessary
     * IMPORTANT : Call this in the slot for aboutToShow.
     */
    void slotCheckUpToDate( );

protected slots:
    /**
     * Called when New->Directory... is clicked
     */
    void slotNewDir();

    /**
     * Called when New->* is clicked
     */
    void slotNewFile();

    /**
     * Fills the templates list.
     */
    void slotFillTemplates();

    void slotResult( KIO::Job * );
    // Special case (filename conflict when creating a link=url file)
    void slotRenamed( KIO::Job *, const KURL&, const KURL& );

private:

    /**
     * Fills the menu from the templates list.
     */
    void fillMenu();

    /**
     * Opens the desktop files and completes the Entry list
     * Input: the entry list. Output: the entry list ;-)
     */
    void parseFiles();

    /**
     * Make the main menus on the startup.
     */
    void makeMenus();

    /**
     * For entryType
     * LINKTOTEMPLATE: a desktop file that points to a file or dir to copy
     * TEMPLATE: a real file to copy as is (the KDE-1.x solution)
     * SEPARATOR: to put a separator in the menu
     * 0 means: not parsed, i.e. we don't know
     */
    enum { LINKTOTEMPLATE = 1, TEMPLATE, SEPARATOR };

    struct Entry {
        QString text;
        QString filePath; // empty for SEPARATOR
        QString templatePath; // same as filePath for TEMPLATE
        QString icon;
        int entryType;
        QString comment;
    };
    // NOTE: only filePath is known before we call parseFiles

    /**
     * List of all template files. It is important that they are in
     * the same order as the 'New' menu.
     */
    static QValueList<Entry> * s_templatesList;

    class KNewMenuPrivate;
    KNewMenuPrivate* d;

    /**
     * Is increased when templatesList has been updated and
     * menu needs to be re-filled. Menus have their own version and compare it
     * to templatesVersion before showing up
     */
    static int s_templatesVersion;

    /**
     * Set back to false each time new templates are found,
     * and to true on the first call to parseFiles
     */
    static bool s_filesParsed;

    int menuItemsVersion;

    /**
     * When the user pressed the right mouse button over an URL a popup menu
     * is displayed. The URL belonging to this popup menu is stored here.
     */
    KURL::List popupFiles;

    /**
     * True when a desktop file with Type=URL is being copied
     */
    bool m_isURLDesktopFile;
    QString m_linkURL; // the url to put in the file

    static KDirWatch * s_pDirWatch;
};

/**
 * @internal
 * Dialog to ask for a filename and a URL, when creating a link to a URL.
 * Basically a merge of KLineEditDlg and KURLRequesterDlg ;)
 * @author David Faure <faure@kde.org>
 */
class KURLDesktopFileDlg : public KDialogBase
{
    Q_OBJECT
public:
    KURLDesktopFileDlg( const QString& textFileName, const QString& textUrl );
    KURLDesktopFileDlg( const QString& textFileName, const QString& textUrl, QWidget *parent );
    virtual ~KURLDesktopFileDlg() {}

    /**
     * @return the filename the user entered (no path)
     */
    QString fileName() const;
    /**
     * @return the URL the user entered
     */
    QString url() const;

protected slots:
    void slotClear();
    void slotNameTextChanged( const QString& );
    void slotURLTextChanged( const QString& );
private:
    void initDialog( const QString& textFileName, const QString& defaultName, const QString& textUrl, const QString& defaultUrl );

    /**
     * The line edit widget for the fileName
     */
    KLineEdit *m_leFileName;
    /**
     * The URL requester for the URL :)
     */
    KURLRequester *m_urlRequester;

    /**
     * True if the filename was manually edited.
     */
    bool m_fileNameEdited;
};

#endif
