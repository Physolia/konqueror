/*
   Copyright (C) 2001 Dawit Alemayehu <adawit@kde.org>

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

#include <kconfig.h>
#include <kio/ioslave_defaults.h>

#include "ksaveioconfig.h"

void KSaveIOConfig::setReadTimeout( int _timeout )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry("ReadTimeout", QMAX(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setConnectTimeout( int _timeout )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry("ConnectTimeout", QMAX(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setProxyConnectTimeout( int _timeout )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry("ProxyConnectTimeout", QMAX(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setResponseTimeout( int _timeout )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry("ResponseTimeout", QMAX(MIN_TIMEOUT_VALUE,_timeout));
  cfg->sync();
  delete cfg;
}


void KSaveIOConfig::setMarkPartial( bool _mode )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry( "MarkPartial", _mode );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setMinimumKeepSize( int _size )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry( "MinimumKeepSize", _size );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setAutoResume( bool _mode )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry( "AutoResume", _mode );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setPersistentConnections( bool _mode )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( QString::null );
  cfg->writeEntry( "PersistentConnections", _mode );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setUseCache( bool _mode )
{
  KConfig *cfg = new KConfig("kio_httprc", false, false);
  cfg->writeEntry( "UseCache", _mode );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setMaxCacheSize( int cache_size )
{
  KConfig *cfg = new KConfig("kio_httprc", false, false);
  cfg->writeEntry( "MaxCacheSize", cache_size );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setMaxCacheAge( int cache_age )
{
  KConfig *cfg = new KConfig("kio_httprc", false, false);
  cfg->writeEntry( "MaxCacheAge", cache_age );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setUseProxy( bool _mode )
{
  setProxyType( _mode ? KProtocolManager::ManualProxy:
                        KProtocolManager::NoProxy );
}

void KSaveIOConfig::setUseReverseProxy( bool mode )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry("ReversedException", mode);
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setProxyType(KProtocolManager::ProxyType type)
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "ProxyType", static_cast<int>(type) );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setProxyAuthMode(KProtocolManager::ProxyAuthMode mode)
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "AuthMode", static_cast<int>(mode) );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setCacheControl(KIO::CacheControl policy)
{
  KConfig *cfg = new KConfig("kio_httprc", false, false);
  QString tmp = KIO::getCacheControlString(policy);
  cfg->writeEntry("cache", tmp);
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setNoProxyFor( const QString& _noproxy )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "NoProxyFor", _noproxy );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setProxyFor( const QString& protocol,
                                    const QString& _proxy )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( protocol.lower() + "Proxy", _proxy );
  cfg->sync();
  delete cfg;
}

void KSaveIOConfig::setProxyConfigScript( const QString& _url )
{
  KConfig *cfg = new KConfig("kioslaverc", false, false);
  cfg->setGroup( "Proxy Settings" );
  cfg->writeEntry( "Proxy Config Script", _url );
  cfg->sync();
  delete cfg;
}
