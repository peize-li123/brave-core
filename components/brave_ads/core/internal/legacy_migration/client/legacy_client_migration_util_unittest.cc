/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/client/legacy_client_migration_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::client {

class BraveAdsLegacyClientMigrationUtilTest : public UnitTestBase {};

TEST_F(BraveAdsLegacyClientMigrationUtilTest, HasMigrated) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedClientState, true);

  // Act

  // Assert
  EXPECT_TRUE(HasMigrated());
}

TEST_F(BraveAdsLegacyClientMigrationUtilTest, HasNotMigrated) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedClientState, false);

  // Act

  // Assert
  EXPECT_FALSE(HasMigrated());
}

}  // namespace brave_ads::client
