/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#ifndef __konq_mainview_h__
#define __konq_mainview_h__

#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kmenubar.h>
#include <kaccel.h>
#include <kservices.h>
#include <kmimetypes.h>

#include "konqueror.h"
#include "kfmpopup.h"
#include "kfmguiprops.h"
#include "kfmrun.h"
#include "konq_frame.h"

#include <opPart.h>
#include <opMainWindow.h>
#include <opFrame.h>
#include <openparts_ui.h>
#include <opMenu.h>
#include <opToolBar.h>
#include <opStatusBar.h>

#include <qpixmap.h>
#include <qlist.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qsplitter.h>

#include <string>
#include <list>
#include <map>

class KBookmarkMenu;
class KURLCompletion;

class KonqMainView : public QWidget,
                     virtual public OPPartIf,
		     virtual public Konqueror::MainView_skel
{
  Q_OBJECT
public:
  //C++
  KonqMainView( QWidget *_parent = 0L );
  ~KonqMainView();

  // two friends which need to access m_bStackLock :)
  friend class KonqMainWindow;
  friend class KonqApplicationIf;
  
  //inherited
  virtual void init();
  virtual void cleanUp();

  virtual bool event( const char* event, const CORBA::Any& value );
  bool mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar );
  bool mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory );
  bool mappingChildGotFocus( OpenParts::Part_ptr child );
  bool mappingParentGotFocus( OpenParts::Part_ptr child );
  bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  //IDL
  // Position is relative to activeView(); above and below create a new row
  virtual void insertView( Konqueror::View_ptr view, Konqueror::NewViewPosition newViewPosition);
  virtual void setActiveView( OpenParts::Id id );
  virtual Konqueror::View_ptr activeView();
  virtual Konqueror::ViewList *viewList();
  virtual void removeView( OpenParts::Id id );

  virtual void openURL( const Konqueror::URLRequest &url );
  virtual void openURL( const char * _url, CORBA::Boolean _reload );
  
  virtual void setStatusBarText( const char *_text );
  virtual void setLocationBarURL( const char *_url );
  virtual void setUpURL( const char *_url );
  
  virtual void createNewWindow( const char *url );
  virtual void popupMenu( const Konqueror::View::MenuPopupRequest &popup );

  virtual char *currentTitle() { return ""; }
  virtual char *currentURL() { return ""; }

  void openDirectory( const char *url );
  void openHTML( const char *url );

public slots:  
  /////////////////////////
  // MenuBar
  /////////////////////////
  virtual void slotSplitView();
  virtual void slotRowAbove();
  virtual void slotRowBelow();
  virtual void slotShowDot();
  virtual void slotLargeIcons();
  virtual void slotSmallIcons();
  virtual void slotTreeView();
  virtual void slotHTMLView();
  virtual void slotSaveGeometry();
  virtual void slotShowCache();
  virtual void slotShowHistory();
  virtual void slotOpenLocation();
  virtual void slotConfigureKeys();
  virtual void slotAboutApp();

  /////////////////////////
  // Location Bar
  /////////////////////////
  virtual void slotURLEntered();
  
  /////////////////////////
  // ToolBar
  /////////////////////////
  virtual void slotStop();
  virtual void slotNewWindow();
  virtual void slotUp();
  virtual void slotHome();
  virtual void slotBack();
  virtual void slotForward();
  virtual void slotReload();

  virtual void slotFileNewActivated( CORBA::Long id );
  virtual void slotFileNewAboutToShow();
  
  virtual void slotBookmarkSelected( CORBA::Long id );
  virtual void slotEditBookmarks();  
  
  virtual void slotURLStarted( const char *url );
  
  /////////////////////////
  // Accel
  /////////////////////////
  void slotFocusLeftView();
  void slotFocusRightView();
  
  /////////////////////////
  // Animated Logo
  /////////////////////////
  void slotAnimatedLogoTimeout();
  void slotStartAnimation();
  virtual void slotStopAnimation();

  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupCopy();
  void slotPopupPaste();
  void slotPopupTrash();
  void slotPopupDelete();
  void slotPopupOpenWith();
  void slotPopupBookmarks();
  void slotPopup( int id );
  void slotPopupProperties();
  
protected:

  ///////////// protected methods ///////////

  virtual void resizeEvent( QResizeEvent *e );

  void initConfig();
  void initGui();
  void initPanner();
  void initView();

  /* Changes the view mode of the current view, if different from viewName*/
  void changeViewMode( const char *viewName );
  /* Create a view
   * @param viewName the type of view to be created (e.g. "KonqKfmIconView") */
  Konqueror::View_var createViewByName( const char *viewName );
  /* Create a new view from the current view (same URL, same view type) */
  void splitView ( Konqueror::NewViewPosition newViewPosition );

  void createViewMenu();
  void setupViewMenus();
  
  ///////// protected members //////////////

  OpenPartsUI::Menu_var m_vMenuFile;
  OpenPartsUI::Menu_var m_vMenuFileNew;
  OpenPartsUI::Menu_var m_vMenuEdit;
  OpenPartsUI::Menu_var m_vMenuView;
  OpenPartsUI::Menu_var m_vMenuBookmarks;
  OpenPartsUI::Menu_var m_vMenuOptions;

  OpenPartsUI::ToolBar_var m_vToolBar;
  OpenPartsUI::ToolBar_var m_vLocationBar;
  
  OpenPartsUI::StatusBar_var m_vStatusBar;

  KBookmarkMenu* m_pBookmarkMenu;

  //////// View storage //////////////

  typedef QSplitter Row;

  struct InternalHistoryEntry
  {
    Konqueror::View::HistoryEntry entry;
    CORBA::String_var viewName;
  };
    
  struct View
  {
    View();
    Konqueror::View_var m_vView;
    KonqFrame *m_pFrame;
    QString m_strUpURL;
    list<InternalHistoryEntry> m_lstBack;
    list<InternalHistoryEntry> m_lstForward;
    Row * row;
  };

  /* The list of rows */
  QList<Row> m_lstRows;
  /* The main, vertical, QSplitter, which holds the rows */
  QSplitter* m_pMainSplitter;

  /* Dual storage of View * instances : mapped by Id */
  map<OpenParts::Id,View*> m_mapViews;
  
  View *m_currentView;
  // current row is currentView->row, no need for a member

  Row * newRow( bool append );

  ////////////////////
    
  /**
   * The menu "New" in the "File" menu.
   * Since the items of this menu are not connected themselves
   * we need a pointer to this menu to get information about the
   * selected menu item.
   */
  KNewMenu *m_pMenuNew;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;

  unsigned int m_animatedLogoCounter;
  QTimer m_animatedLogoTimer;

  int m_iStackLock;
  bool m_bBack;
  bool m_bForward;

  KAccel* m_pAccel;

  KfmGuiProps *m_Props;

  QStrList m_lstPopupURLs;
  map<int,KService*> m_mapPopup;
  map<int,KDELnkMimeType::Service> m_mapPopup2;

  KfmRun *m_pRun;
      
  static QList<OpenPartsUI::Pixmap>* s_lstAnimatedLogo;
  static QList<KonqMainView>* s_lstWindows;
};

#endif
