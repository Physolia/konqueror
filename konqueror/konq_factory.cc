/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "konq_factory.h"
#include "konq_part.h"
#include "konq_misc.h"
#include "konq_run.h"
#include "browser.h"

#include <kdebug.h>
#include <kinstance.h>
#include <kstddirs.h>

KInstance *KonqFactory::s_instance = 0L;

extern "C"
{
  void *init_libkonqueror()
  {
    return new KonqFactory;
  }
};

KonqFactory::KonqFactory()
{
  s_instance = 0L;
  QString path = instance()->dirs()->saveLocation("data", "kfm/bookmarks", true);
  m_bookmarkManager = new KonqBookmarkManager( path );
  m_fileManager = new KonqFileManager;
}

KonqFactory::~KonqFactory()
{
  delete m_bookmarkManager;
  delete m_fileManager;

  if ( s_instance )
    delete s_instance;

  s_instance = 0L;
}

BrowserView *KonqFactory::createView( const QString &serviceType,
                                      const QString &serviceName,
				      KService::Ptr *serviceImpl,
				      KTrader::OfferList *serviceOffers )
{
  kdebug(0,1202,"trying to create view for \"%s\"", serviceType.ascii());

  QString constraint = "('Browser/View' in ServiceTypes)";

  KTrader::OfferList offers = KTrader::self()->query( serviceType, constraint );

  if ( offers.count() == 0 ) //no results?
    return 0L;

  KService::Ptr service = offers.first();

  if ( !serviceName.isEmpty() )
  {
    KTrader::OfferList::ConstIterator it = offers.begin();
    KTrader::OfferList::ConstIterator end = offers.end();
    for (; it != end; ++it )
      if ( (*it)->name() == serviceName )
      {
        service = *it;
        break;
      }
  }
  else
  {
    KTrader::OfferList::ConstIterator it = offers.begin();
    KTrader::OfferList::ConstIterator end = offers.end();
    for (; it != end; ++it )
    {
      KService::PropertyPtr prop = (*it)->property( "X-KDE-BrowserView-AllowAsDefault" );
      if ( !!prop && prop->boolValue() )
      {
        service = *it;
        break;
      }
    }
  }

  KLibFactory *factory = KLibLoader::self()->factory( service->library() );

  if ( !factory )
    return 0L;

  if ( serviceOffers )
    (*serviceOffers) = offers;

  if ( serviceImpl )
    (*serviceImpl) = service;

  QStringList args;

  KService::PropertyPtr prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop )
  {
    QString argStr = prop->stringValue();
    args = QStringList::split( " ", argStr );
  }

  return (BrowserView *)factory->create( 0, 0, "QObject", args );
}

QObject* KonqFactory::create( QObject* parent, const char* name, const char* /*classname*/, const QStringList & )
{
//  if ( !parent || !parent->inherits( "Part" ) )
//    return 0L;

  return new KonqPart( parent, name );
}

KInstance *KonqFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( "konqueror" );

  return s_instance;
}

#include "konq_factory.moc"
