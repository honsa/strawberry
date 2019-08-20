/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
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

#ifndef REDIRECTFOLLOWER_H
#define REDIRECTFOLLOWER_H

#include "config.h"

#include <stdbool.h>

#include <QtGlobal>
#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QString>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

class RedirectFollower : public QObject {
  Q_OBJECT

 public:
  explicit RedirectFollower(QNetworkReply *first_reply, int max_redirects = 5);

  bool hit_redirect_limit() const { return redirects_remaining_ < 0; }
  QNetworkReply *reply() const { return current_reply_; }

  // These are all forwarded to the current reply.
  QNetworkReply::NetworkError error() const { return current_reply_->error(); }
  QString errorString() const { return current_reply_->errorString(); }
  QVariant attribute(QNetworkRequest::Attribute code) const { return current_reply_->attribute(code); }
  QVariant header(QNetworkRequest::KnownHeaders header) const { return current_reply_->header(header); }
  qint64 bytesAvailable() const { return current_reply_->bytesAvailable(); }
  QUrl url() const { return current_reply_->url(); }
  QByteArray readAll() { return current_reply_->readAll(); }
  void abort() { current_reply_->abort(); }

signals:
  // These are all forwarded from the current reply.
  void readyRead();
  void error(QNetworkReply::NetworkError);
  void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
  void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

  // This is NOT emitted when a request that has a redirect finishes.
  void finished();

 private slots:
  void ReadyRead();
  void ReplyFinished();

 private:
  void ConnectReply(QNetworkReply *reply);

 private:
  QNetworkReply *current_reply_;
  int redirects_remaining_;
};

#endif  // REDIRECTFOLLOWER_H
