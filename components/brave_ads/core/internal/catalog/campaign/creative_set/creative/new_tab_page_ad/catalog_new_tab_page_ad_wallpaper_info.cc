/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"

namespace brave_ads {

bool CatalogNewTabPageAdWallpaperInfo::operator==(
    const CatalogNewTabPageAdWallpaperInfo& other) const {
  return image_url == other.image_url && focal_point == other.focal_point;
}

bool CatalogNewTabPageAdWallpaperInfo::operator!=(
    const CatalogNewTabPageAdWallpaperInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
