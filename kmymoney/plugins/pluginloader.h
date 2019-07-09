/***************************************************************************
                          pluginloader.h
                             -------------------
    begin                : Thu Feb 12 2009
    copyright            : (C) 2009 Cristian Onet
    email                : onet.cristian@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

// ----------------------------------------------------------------------------
// QT Includes

#include <QObject>
#include <QByteArray>

// ----------------------------------------------------------------------------
// KDE Includes

// ----------------------------------------------------------------------------
// Project Includes

#include <kmm_plugin_export.h>

class KPluginSelector;
class KPluginInfo;

namespace KMyMoneyPlugin
{
class Plugin;

class KMM_PLUGIN_EXPORT PluginLoader : public QObject
{
  Q_OBJECT
public:
  typedef QMap<QString, Plugin*> PluginsMap;

  PluginLoader(QObject* parent);
  virtual ~PluginLoader();
  static PluginLoader* instance();

  void loadPlugins();
  Plugin* getPluginFromInfo(KPluginInfo*);
  KPluginSelector* pluginSelectorWidget();
  const PluginsMap &loadedPlugins();

private:
  void loadPlugin(KPluginInfo*);

signals:
  void plug(KPluginInfo*);
  void unplug(KPluginInfo*);
  void configChanged(Plugin*);  // consfiguration of the plugin has changed not the enabled/disabled state

private slots:
  void changed();
  void changedConfigOfPlugin(const QByteArray &);

private:
  struct Private;
  Private* const d;
};
}

#endif /* PLUGINLOADER_H */
