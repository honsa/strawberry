/*
 * Strawberry Music Player
 * Copyright 2020-2021, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef QOBUZCOVERPROVIDER_H
#define QOBUZCOVERPROVIDER_H

#include "config.h"

#include <QObject>
#include <QList>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include <QSslError>

#include "includes/shared_ptr.h"
#include "jsoncoverprovider.h"
#include "qobuz/qobuzservice.h"

class QNetworkReply;
class NetworkAccessManager;

class QobuzCoverProvider : public JsonCoverProvider {
  Q_OBJECT

 public:
  explicit QobuzCoverProvider(const QobuzServicePtr service, SharedPtr<NetworkAccessManager> network, QObject *parent = nullptr);
  ~QobuzCoverProvider() override;

  bool StartSearch(const QString &artist, const QString &album, const QString &title, const int id) override;
  void CancelSearch(const int id) override;

  bool IsAuthenticated() const override { return service_ && service_->authenticated(); }
  void Deauthenticate() override { if (service_) service_->Logout(); }

 private Q_SLOTS:
  void HandleSearchReply(QNetworkReply *reply, const int id);

 private:
  QByteArray GetReplyData(QNetworkReply *reply);
  void Error(const QString &error, const QVariant &debug = QVariant()) override;

 private:
  QobuzServicePtr service_;
  QList<QNetworkReply*> replies_;
};

#endif  // QOBUZCOVERPROVIDER_H
