/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/observer_list_types.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"

namespace brave_rewards {

class RewardsService;

class RewardsServiceObserver : public base::CheckedObserver {
 public:
  ~RewardsServiceObserver() override {}

  virtual void OnRewardsInitialized(
      RewardsService* rewards_service) {}

  // Called when the user's Rewards wallet information has been created or
  // updated.
  virtual void OnRewardsWalletUpdated() {}

  virtual void OnFetchPromotions(
      RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::vector<ledger::mojom::PromotionPtr>& list) {}

  virtual void OnPromotionFinished(RewardsService* rewards_service,
                                   const ledger::mojom::Result result,
                                   ledger::mojom::PromotionPtr promotion) {}

  virtual void OnExcludedSitesChanged(
      RewardsService* rewards_service,
      std::string publisher_id,
      bool excluded) {}

  virtual void OnReconcileComplete(
      RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::mojom::RewardsType type,
      const ledger::mojom::ContributionProcessor processor) {}

  virtual void OnAdsEnabled(
      brave_rewards::RewardsService* rewards_service,
      bool ads_enabled) {}

  virtual void OnPendingContributionSaved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result) {}

  virtual void OnPublisherListNormalized(
      RewardsService* rewards_service,
      std::vector<ledger::mojom::PublisherInfoPtr> list) {}

  virtual void OnPublisherRegistryUpdated() {}

  virtual void OnPublisherUpdated(const std::string& publisher_id) {}

  virtual void OnStatementChanged(
      brave_rewards::RewardsService* rewards_service) {}

  virtual void OnRecurringTipSaved(
      brave_rewards::RewardsService* rewards_service,
      bool success) {}

  virtual void OnRecurringTipRemoved(
      brave_rewards::RewardsService* rewards_service,
      bool success) {}

  virtual void OnPendingContributionRemoved(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result) {}

  virtual void OnExternalWalletConnected() {}

  virtual void OnExternalWalletLoggedOut() {}

  virtual void OnExternalWalletReconnected() {}

  virtual void OnUnblindedTokensReady(
      brave_rewards::RewardsService* rewards_service) {}

  virtual void ReconcileStampReset() {}

  virtual void OnCompleteReset(const bool success) {}

  virtual void OnPanelPublisherInfo(RewardsService* rewards_service,
                                    const ledger::mojom::Result result,
                                    const ledger::mojom::PublisherInfo* info,
                                    uint64_t windowId) {}

  // DO NOT ADD ANY MORE METHODS HERE UNLESS IT IS A BROADCAST NOTIFICATION
  // RewardsServiceObserver should not be used to return responses to the
  // caller. Method calls on RewardsService should use callbacks to return
  // responses. The observer is primarily for broadcast notifications of events
  // from the rewards service. OnPublisherInfoUpdated,
  // etc... are examples of events that all observers will be interested in.
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_SERVICE_OBSERVER_H_
