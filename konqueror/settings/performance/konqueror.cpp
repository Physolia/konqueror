/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "konqueror.h"

#include <dcopref.h>
#include <kconfig.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <klocale.h>

namespace KCMPerformance
{

Konqueror::Konqueror( QWidget* parent_P )
    : Konqueror_ui( parent_P )
    {
    QWhatsThis::add( rb_never_reuse,
        i18n( "Disables the minimization of memory usage and allows you "
              "to make each browsing activity independent from the others" ));
    QWhatsThis::add( rb_file_browsing_reuse,
        i18n( "With this option activated, only one instance of Konqueror "
              "used for file browsing will exist in the memory of your computer "
              "at any moment, "
              "no matter how many file browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your file browsing windows will be closed simultaneously" ));
    QWhatsThis::add( rb_always_reuse,
        i18n( "With this option activated, only one instance of Konqueror "
              "will exist in the memory of your computer at any moment, "
              "no matter how many browsing windows you open, "
              "thus reducing resource requirements."
              "<p>Be aware that this also means that, if something goes wrong, "
              "all your browsing windows will be closed simultaneously." ));
    connect( rb_never_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_file_browsing_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    connect( rb_always_reuse, SIGNAL( clicked()), SIGNAL( changed()));
    rb_file_browsing_reuse->setChecked( true );
    
    QString tmp =
        i18n( "If non-zero, this option allows keeping Konqueror instances "
              "in memory after all their windows have been closed, up to the "
              "number specified in this option. "
              "When a new Konqueror instance is needed, one of these preloaded "
              "instances will be reused instead, improving responsiveness at "
              "the expense of the memory required by the preloaded instances." );
    QWhatsThis::add( sb_preload_count, tmp );
    QWhatsThis::add( lb_preload_count, tmp );
    connect( sb_preload_count, SIGNAL( valueChanged( int )), SIGNAL( changed()));
    defaults();
    }
    
void Konqueror::load()
    {
    KConfig cfg( "konquerorrc", true );
    cfg.setGroup( "Reusing" );
    allowed_parts = cfg.readEntry( "SafeParts", "SAFE" );
    if( allowed_parts == "ALL" )
        rb_always_reuse->setChecked( true );
    else if( allowed_parts.isEmpty())
        rb_never_reuse->setChecked( true );
    else
        rb_file_browsing_reuse->setChecked( true );
    sb_preload_count->setValue( cfg.readNumEntry( "MaxPreloadCount", 1 ));
    }
    
void Konqueror::save()
    {
    KConfig cfg( "konquerorrc" );
    cfg.setGroup( "Reusing" );
    if( rb_always_reuse->isChecked())
        allowed_parts = "ALL";
    else if( rb_never_reuse->isChecked())
        allowed_parts = "";
    else
        {
        if( allowed_parts.isEmpty() || allowed_parts == "ALL" )
            allowed_parts = "SAFE";
        // else - keep allowed_parts as read from the file, as the user may have modified the list there
        }
    cfg.writeEntry( "SafeParts", allowed_parts );
    cfg.writeEntry( "MaxPreloadCount", sb_preload_count->value());
    cfg.sync();
    DCOPRef ref1( "konqueror*", "KonquerorIface" );
    ref1.send( "reparseConfiguration()" );
    DCOPRef ref2( "kded", "konqy_preloader" );
    ref2.send( "reconfigure()" );
    }
    
void Konqueror::defaults()
    {
    rb_file_browsing_reuse->setChecked( true );
    allowed_parts = "SAFE";
    sb_preload_count->setValue( 1 );
    }
    
} // namespace

#include "konqueror.moc"
