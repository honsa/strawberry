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

#include "config.h"

#include <utility>

#include <QtGlobal>
#include <QString>

#include "coversearchstatistics.h"

using namespace Qt::Literals::StringLiterals;

CoverSearchStatistics::CoverSearchStatistics()
    : network_requests_made_(0),
      bytes_transferred_(0),
      chosen_images_(0),
      missing_images_(0),
      chosen_width_(0),
      chosen_height_(0) {}

CoverSearchStatistics &CoverSearchStatistics::operator+=(const CoverSearchStatistics &other) {

  network_requests_made_ += other.network_requests_made_;
  bytes_transferred_ += other.bytes_transferred_;

  QStringList keys = other.chosen_images_by_provider_.keys();
  for (const QString &key : std::as_const(keys)) {
    chosen_images_by_provider_[key] += other.chosen_images_by_provider_[key];
  }
  keys = other.total_images_by_provider_.keys();
  for (const QString &key : std::as_const(keys)) {
    total_images_by_provider_[key] += other.total_images_by_provider_[key];
  }

  chosen_images_ += other.chosen_images_;
  missing_images_ += other.missing_images_;

  chosen_width_ += other.chosen_width_;
  chosen_height_ += other.chosen_height_;

  return *this;

}

QString CoverSearchStatistics::AverageDimensions() const {

  if (chosen_images_ == 0) {
    return u"0x0"_s;
  }

  return QString::number(chosen_width_ / chosen_images_) + QLatin1Char('x') + QString::number(chosen_height_ / chosen_images_);

}
