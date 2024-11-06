/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2018-2021, Jonas Kvinge <jonas@jkvinge.net>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <QWidget>
#include <QList>
#include <QStringList>
#include <QAction>
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <QTreeWidget>
#include <QKeySequence>
#include <QShortcut>
#include <QSettings>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QLabel>

#include "core/iconloader.h"
#include "core/logging.h"
#include "core/settings.h"
#include "utilities/envutils.h"
#include "constants/globalshortcutssettings.h"
#include "globalshortcuts/globalshortcutgrabber.h"
#include "globalshortcuts/globalshortcutsmanager.h"
#include "settingspage.h"
#include "settingsdialog.h"
#include "globalshortcutssettingspage.h"
#include "ui_globalshortcutssettingspage.h"

using namespace Qt::Literals::StringLiterals;

using namespace GlobalShortcutsSettings;

GlobalShortcutsSettingsPage::GlobalShortcutsSettingsPage(SettingsDialog *dialog, GlobalShortcutsManager *global_shortcuts_manager, QWidget *parent)
    : SettingsPage(dialog, parent),
      ui_(new Ui_GlobalShortcutsSettingsPage),
      global_shortcuts_manager_(global_shortcuts_manager),
      initialized_(false),
      grabber_(new GlobalShortcutGrabber()) {

  ui_->setupUi(this);
  ui_->shortcut_options->setEnabled(false);
  ui_->list->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
  setWindowIcon(IconLoader::Load(u"keyboard"_s, true, 0, 32));

  QObject::connect(ui_->list, &QTreeWidget::currentItemChanged, this, &GlobalShortcutsSettingsPage::ItemClicked);
  QObject::connect(ui_->radio_none, &QRadioButton::clicked, this, &GlobalShortcutsSettingsPage::NoneClicked);
  QObject::connect(ui_->radio_default, &QRadioButton::clicked, this, &GlobalShortcutsSettingsPage::DefaultClicked);
  QObject::connect(ui_->radio_custom, &QRadioButton::clicked, this, &GlobalShortcutsSettingsPage::ChangeClicked);
  QObject::connect(ui_->button_change, &QPushButton::clicked, this, &GlobalShortcutsSettingsPage::ChangeClicked);

#ifdef HAVE_KDE_GLOBALSHORTCUTS
  QObject::connect(ui_->checkbox_kde, &QCheckBox::toggled, this, &GlobalShortcutsSettingsPage::ShortcutOptionsChanged);
#else
  ui_->widget_kde->hide();
#endif
#ifdef HAVE_GNOME_GLOBALSHORTCUTS
  QObject::connect(ui_->checkbox_gnome, &QCheckBox::toggled, this, &GlobalShortcutsSettingsPage::ShortcutOptionsChanged);
  QObject::connect(ui_->button_gnome_open, &QPushButton::clicked, this, &GlobalShortcutsSettingsPage::OpenGnomeKeybindingProperties);
#else
  ui_->widget_gnome->hide();
 #endif
#ifdef HAVE_MATE_GLOBALSHORTCUTS
  QObject::connect(ui_->checkbox_mate, &QCheckBox::toggled, this, &GlobalShortcutsSettingsPage::ShortcutOptionsChanged);
  QObject::connect(ui_->button_mate_open, &QPushButton::clicked, this, &GlobalShortcutsSettingsPage::OpenMateKeybindingProperties);
#else
  ui_->widget_mate->hide();
#endif

#ifdef HAVE_X11_GLOBALSHORTCUTS
  QObject::connect(ui_->checkbox_x11, &QCheckBox::toggled, this, &GlobalShortcutsSettingsPage::ShortcutOptionsChanged);
#else
  ui_->widget_x11->hide();
#endif

#ifndef Q_OS_MACOS
  ui_->widget_macos_access->hide();
#endif

}

GlobalShortcutsSettingsPage::~GlobalShortcutsSettingsPage() { delete ui_; }

void GlobalShortcutsSettingsPage::Load() {

  Settings s;
  s.beginGroup(kSettingsGroup);

  if (!initialized_) {
    initialized_ = true;

    de_ = Utilities::DesktopEnvironment();
    ui_->widget_warning->hide();

#ifdef Q_OS_MACOS
    QObject::connect(ui_->button_macos_preferences, &QPushButton::clicked, global_shortcuts_manager_, &GlobalShortcutsManager::ShowMacAccessibilityDialog);
#endif

#ifdef HAVE_KDE_GLOBALSHORTCUTS
    if (GlobalShortcutsManager::IsKdeAvailable()) {
      qLog(Debug) << "KDE (KGlobalAccel) backend is available.";
      ui_->widget_kde->show();
    }
    else {
      qLog(Debug) << "KDE (KGlobalAccel) backend is unavailable.";
      ui_->widget_kde->hide();
    }
#endif

#ifdef HAVE_GNOME_GLOBALSHORTCUTS
    if (GlobalShortcutsManager::IsGnomeAvailable()) {
      qLog(Debug) << "Gnome (GSD) backend is available.";
      ui_->widget_gnome->show();
    }
    else {
      qLog(Debug) << "Gnome (GSD) backend is unavailable.";
      ui_->widget_gnome->hide();
    }
#endif

#ifdef HAVE_MATE_GLOBALSHORTCUTS
    if (GlobalShortcutsManager::IsMateAvailable()) {
      qLog(Debug) << "MATE backend is available.";
      ui_->widget_mate->show();
    }
    else {
      qLog(Debug) << "MATE backend is unavailable.";
      ui_->widget_mate->hide();
    }
#endif

#ifdef HAVE_X11_GLOBALSHORTCUTS
    if (GlobalShortcutsManager::IsX11Available()) {
      qLog(Debug) << "X11 backend is available.";
      ui_->widget_x11->show();
    }
    else {
      qLog(Debug) << "X11 backend is unavailable.";
      ui_->widget_x11->hide();
    }
#endif

    const QList<GlobalShortcutsManager::Shortcut> shortcuts = global_shortcuts_manager_->shortcuts().values();
    for (const GlobalShortcutsManager::Shortcut &i : shortcuts) {
      Shortcut shortcut;
      shortcut.s = i;
      shortcut.key = i.action->shortcut();
      shortcut.item = new QTreeWidgetItem(ui_->list, QStringList() << i.action->text() << i.action->shortcut().toString(QKeySequence::NativeText));
      shortcut.item->setData(0, Qt::UserRole, i.id);
      shortcuts_[i.id] = shortcut;
    }

    ui_->list->sortItems(0, Qt::AscendingOrder);
    ItemClicked(ui_->list->topLevelItem(0));
  }

  const QList<Shortcut> shortcuts = shortcuts_.values();
  for (const Shortcut &shortcut : shortcuts) {
    SetShortcut(shortcut.s.id, shortcut.s.action->shortcut());
  }

#ifdef HAVE_KDE_GLOBALSHORTCUTS
  if (ui_->widget_kde->isVisibleTo(this)) {
    ui_->checkbox_kde->setChecked(s.value(kUseKDE, true).toBool());
  }
#endif

#ifdef HAVE_GNOME_GLOBALSHORTCUTS
  if (ui_->widget_gnome->isVisibleTo(this)) {
    ui_->checkbox_gnome->setChecked(s.value(kUseGnome, true).toBool());
  }
#endif

#ifdef HAVE_MATE_GLOBALSHORTCUTS
  if (ui_->widget_mate->isVisibleTo(this)) {
    ui_->checkbox_mate->setChecked(s.value(kUseMate, true).toBool());
  }
#endif

#ifdef HAVE_X11_GLOBALSHORTCUTS
  if (ui_->widget_x11->isVisibleTo(this)) {
    ui_->checkbox_x11->setChecked(s.value(kUseX11, false).toBool());
  }
#endif

#if defined(HAVE_KDE_GLOBALSHORTCUTS) || defined(HAVE_GNOME_GLOBALSHORTCUTS) || defined(HAVE_MATE_GLOBALSHORTCUTS) || defined(HAVE_X11_GLOBALSHORTCUTS)
  ShortcutOptionsChanged();
#endif

#ifdef Q_OS_MACOS
  ui_->widget_macos_access->setVisible(!GlobalShortcutsManager::IsMacAccessibilityEnabled());
#endif  // Q_OS_MACOS

  s.endGroup();

  Init(ui_->layout_globalshortcutssettingspage->parentWidget());

  if (!Settings().childGroups().contains(QLatin1String(kSettingsGroup))) set_changed();

}

void GlobalShortcutsSettingsPage::Save() {

  Settings s;
  s.beginGroup(kSettingsGroup);

  const QList<Shortcut> shortcuts = shortcuts_.values();
  for (const Shortcut &shortcut : shortcuts) {
    shortcut.s.action->setShortcut(shortcut.key);
    shortcut.s.shortcut->setKey(shortcut.key);
    s.setValue(shortcut.s.id, shortcut.key.toString());
  }

#ifdef HAVE_KDE_GLOBALSHORTCUTS
  s.setValue(kUseKDE, ui_->checkbox_kde->isChecked());
#endif

#ifdef HAVE_GNOME_GLOBALSHORTCUTS
  s.setValue(kUseGnome, ui_->checkbox_gnome->isChecked());
#endif

#ifdef HAVE_MATE_GLOBALSHORTCUTS
  s.setValue(kUseMate, ui_->checkbox_mate->isChecked());
#endif

#ifdef HAVE_X11_GLOBALSHORTCUTS
  s.setValue(kUseX11, ui_->checkbox_x11->isChecked());
#endif

  s.endGroup();

  global_shortcuts_manager_->ReloadSettings();

}

void GlobalShortcutsSettingsPage::ShortcutOptionsChanged() {

  bool configure_shortcuts = (ui_->widget_kde->isVisibleTo(this) && ui_->checkbox_kde->isChecked()) ||
                             (ui_->widget_x11->isVisibleTo(this) && ui_->checkbox_x11->isChecked());

  ui_->list->setEnabled(configure_shortcuts);
  ui_->shortcut_options->setEnabled(configure_shortcuts);

  if (ui_->widget_x11->isVisibleTo(this) && ui_->checkbox_x11->isChecked()) {
    ui_->widget_warning->show();
    X11Warning();
  }
  else {
    ui_->widget_warning->hide();
  }

}

void GlobalShortcutsSettingsPage::OpenGnomeKeybindingProperties() {

  if (!QProcess::startDetached(u"gnome-keybinding-properties"_s, QStringList())) {
    if (!QProcess::startDetached(u"gnome-control-center"_s, QStringList() << u"keyboard"_s)) {
      QMessageBox::warning(this, u"Error"_s, tr("The \"%1\" command could not be started.").arg("gnome-keybinding-properties"_L1));
    }
  }

}

void GlobalShortcutsSettingsPage::OpenMateKeybindingProperties() {

  if (!QProcess::startDetached(u"mate-keybinding-properties"_s, QStringList())) {
    if (!QProcess::startDetached(u"mate-control-center"_s, QStringList() << u"keyboard"_s)) {
      QMessageBox::warning(this, u"Error"_s, tr("The \"%1\" command could not be started.").arg("mate-keybinding-properties"_L1));
    }
  }

}

void GlobalShortcutsSettingsPage::SetShortcut(const QString &id, const QKeySequence &key) {

  Shortcut shortcut = shortcuts_.value(id);

  shortcut.key = key;
  shortcut.item->setText(1, key.toString(QKeySequence::NativeText));

  shortcuts_[id] = shortcut;

}

void GlobalShortcutsSettingsPage::ItemClicked(QTreeWidgetItem *item) {

  current_id_ = item->data(0, Qt::UserRole).toString();
  const Shortcut shortcut = shortcuts_.value(current_id_);

  // Enable options
  ui_->shortcut_options->setEnabled(true);
  ui_->shortcut_options->setTitle(tr("Shortcut for %1").arg(shortcut.s.action->text()));

  if (shortcut.key == shortcut.s.default_key) {
    ui_->radio_default->setChecked(true);
  }
  else if (shortcut.key.isEmpty()) {
    ui_->radio_none->setChecked(true);
  }
  else {
    ui_->radio_custom->setChecked(true);
  }

}

void GlobalShortcutsSettingsPage::NoneClicked() {

  SetShortcut(current_id_, QKeySequence());
  set_changed();

}

void GlobalShortcutsSettingsPage::DefaultClicked() {

  SetShortcut(current_id_, shortcuts_[current_id_].s.default_key);
  set_changed();

}

void GlobalShortcutsSettingsPage::ChangeClicked() {

  global_shortcuts_manager_->Unregister();
  QKeySequence key = grabber_->GetKey(shortcuts_.value(current_id_).s.action->text());
  global_shortcuts_manager_->Register();

  if (key.isEmpty()) return;

  // Check if this key sequence is used by any other actions
  const QStringList ids = shortcuts_.keys();
  for (const QString &id : ids) {
    if (shortcuts_[id].key == key) SetShortcut(id, QKeySequence());
  }

  ui_->radio_custom->setChecked(true);
  SetShortcut(current_id_, key);

  set_changed();

}

void GlobalShortcutsSettingsPage::X11Warning() {

  QString de = de_.toLower();
  if (de == "kde"_L1 || de == "gnome"_L1 || de == "x-cinnamon"_L1 || de == "mate"_L1) {
    QString text(tr("Using X11 shortcuts on %1 is not recommended and can cause keyboard to become unresponsive!").arg(de_));
    if (de == "kde"_L1) {
      text += tr(" Shortcuts on %1 are usually used through MPRIS and KGlobalAccel.").arg(de_);
    }
    else if (de == "gnome"_L1) {
      text += tr(" Shortcuts on %1 are usually used through Gnome Settings Daemon and should be configured in gnome-settings-daemon instead.").arg(de_);
    }
    else if (de == "x-cinnamon"_L1) {
      text += tr(" Shortcuts on %1 are usually used through Gnome Settings Daemon and should be configured in cinnamon-settings-daemon instead.").arg(de_);
    }
    else if (de == "mate"_L1) {
      text += tr(" Shortcuts on %1 are usually used through MATE Settings Daemon and should be configured there instead.").arg(de_);
    }
    ui_->label_warn_text->setText(text);
    ui_->widget_warning->show();
  }
  else {
    ui_->widget_warning->hide();
  }

}
