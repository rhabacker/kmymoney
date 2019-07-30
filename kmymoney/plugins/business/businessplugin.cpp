/***************************************************************************
 *   Copyright 2018  Ralf Habacker <ralf.habacker@freenet.de>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>  *
 ***************************************************************************/

#include "businessplugin.h"

// ----------------------------------------------------------------------------
// KDE Includes

#include <KPluginFactory>

K_PLUGIN_FACTORY(BusinessFactory, registerPlugin<BusinessPlugin>();)
K_EXPORT_PLUGIN(BusinessFactory("kmm_business"))

BusinessPlugin::BusinessPlugin(QObject *parent, const QVariantList&)
  : KMyMoneyPlugin::Plugin(parent, "business"/*must be the same as X-KDE-PluginInfo-Name*/)
{
  setComponentData(BusinessFactory::componentData());
  // For information, announce that we have been loaded.
  qDebug("KMyMoney csvexport plugin loaded");
}

BusinessPlugin::~BusinessPlugin()
{

}
