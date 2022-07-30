// Copyright 2015 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDialog>
#include <QListWidget>
#include <vector>

#include "Plugins/PluginHost.h"

class PluginsWindow final : public QDialog
{
    Q_OBJECT
public:
    explicit PluginsWindow(QWidget* parent = nullptr);
    void RefreshPluginsList() 
    { 
        m_plugins_list->clear();
        LoadPluginsList(); 
    }

private:
    void LoadPluginsList();
    void PluginItemChanged(QListWidgetItem* item);

    QListWidget* m_plugins_list;
    std::vector<Plugins::PluginFiles> plugins;
};