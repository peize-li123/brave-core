/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/hosts/static_url_host.h"

#include <ostream>

#include "base/notreached.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

namespace {

constexpr char kProductionHost[] = "https://static.ads.brave.com";
constexpr char kStagingHost[] = "https://static.ads.bravesoftware.com";

}  // namespace

std::string StaticUrlHost::Get() const {
  const mojom::EnvironmentType environment_type =
      GlobalState::GetInstance()->Flags().environment_type;

  switch (environment_type) {
    case mojom::EnvironmentType::kProduction: {
      return kProductionHost;
    }

    case mojom::EnvironmentType::kStaging: {
      return kStagingHost;
    }
  }

  NOTREACHED() << "Unexpected value for EnvironmentType: "
               << static_cast<int>(environment_type);
  return kStagingHost;
}

}  // namespace brave_ads
