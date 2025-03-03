/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_activity/user_activity_features.h"

namespace brave_ads {

BASE_FEATURE(kUserActivityFeature,
             "UserActivity",
             base::FEATURE_ENABLED_BY_DEFAULT);

bool IsUserActivityEnabled() {
  return base::FeatureList::IsEnabled(kUserActivityFeature);
}

}  // namespace brave_ads
