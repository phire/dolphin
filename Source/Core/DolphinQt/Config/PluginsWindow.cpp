// Copyright 2015 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Config/PluginsWindow.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QPushButton>
#include <string>
#include <vector>


#include "Common/FileUtil.h"
#include "Plugins/PluginHost.h"


PluginsWindow::PluginsWindow(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Plugins"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout* layout = new QVBoxLayout();
    QHBoxLayout* h_layout = new QHBoxLayout();

    m_plugins_list = new QListWidget();

    LoadPluginsList();

    QDialogButtonBox* close = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* refresh = new QPushButton(this);
    refresh->setText(tr("Refresh"));
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(refresh, &QPushButton::pressed, this, &PluginsWindow::RefreshPluginsList);

    layout->addWidget(m_plugins_list);
    h_layout->addWidget(refresh);
    h_layout->addWidget(close);

    layout->addLayout(h_layout);

    connect(m_plugins_list, &QListWidget::itemChanged, this, &PluginsWindow::PluginItemChanged);

    setLayout(layout);
}

void PluginsWindow::LoadPluginsList()
{
    plugins = Plugins::GetAllPlugins();
    for(const auto& plugin : plugins)
    {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(plugin.virtualName));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setData(Qt::UserRole, QString::fromStdString(plugin.physicalName));
        item->setCheckState(plugin.Active ? Qt::Checked : Qt::Unchecked);
        m_plugins_list->addItem(item);
    }
}

void PluginsWindow::PluginItemChanged(QListWidgetItem* item)
{
    const auto plugin_path = item->data(Qt::UserRole).toString();

    for(u32 i = 0; i < plugins.size(); i++)
    {
        if(plugins.at(i).physicalName == plugin_path.toStdString())
        {
            if(!plugins.at(i).Active)
                Plugins::LoadPlugin(i);
            if(plugins.at(i).Active)
                Plugins::ShutdownPlugin(i);
        }
    }
    RefreshPluginsList();
}