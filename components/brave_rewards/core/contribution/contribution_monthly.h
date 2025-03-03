/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_

#include <vector>

#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
class LedgerImpl;

namespace contribution {

class ContributionMonthly {
 public:
  explicit ContributionMonthly(LedgerImpl& ledger);

  ~ContributionMonthly();

  void Process(absl::optional<base::Time> cutoff_time,
               ledger::LegacyResultCallback callback);

 private:
  void AdvanceContributionDates(
      absl::optional<base::Time> cutoff_time,
      ledger::LegacyResultCallback callback,
      std::vector<mojom::PublisherInfoPtr> publishers);

  void OnNextContributionDateAdvanced(
      std::vector<mojom::PublisherInfoPtr> publishers,
      ledger::LegacyResultCallback callback,
      bool success);

  const raw_ref<LedgerImpl> ledger_;
};

}  // namespace contribution
}  // namespace ledger
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CONTRIBUTION_CONTRIBUTION_MONTHLY_H_
