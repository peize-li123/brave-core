/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/contribution/contribution_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"

namespace ledger {
namespace contribution {

mojom::ReportType GetReportTypeFromRewardsType(const mojom::RewardsType type) {
  switch (static_cast<int>(type)) {
    case static_cast<int>(mojom::RewardsType::AUTO_CONTRIBUTE): {
      return mojom::ReportType::AUTO_CONTRIBUTION;
    }
    case static_cast<int>(mojom::RewardsType::ONE_TIME_TIP): {
      return mojom::ReportType::TIP;
    }
    case static_cast<int>(mojom::RewardsType::RECURRING_TIP): {
      return mojom::ReportType::TIP_RECURRING;
    }
    default: {
      // missing conversion, returning dummy value.
      NOTREACHED();
      return mojom::ReportType::TIP;
    }
  }
}

mojom::ContributionProcessor GetProcessor(const std::string& wallet_type) {
  if (wallet_type == constant::kWalletUnBlinded) {
    return mojom::ContributionProcessor::BRAVE_TOKENS;
  }

  if (wallet_type == constant::kWalletUphold) {
    return mojom::ContributionProcessor::UPHOLD;
  }

  if (wallet_type == constant::kWalletBitflyer) {
    return mojom::ContributionProcessor::BITFLYER;
  }

  if (wallet_type == constant::kWalletGemini) {
    return mojom::ContributionProcessor::GEMINI;
  }

  return mojom::ContributionProcessor::NONE;
}

std::string GetNextProcessor(const std::string& current_processor) {
  if (current_processor == constant::kWalletUnBlinded) {
    return constant::kWalletUphold;
  }

  if (current_processor == constant::kWalletUphold) {
    return constant::kWalletBitflyer;
  }

  if (current_processor == constant::kWalletBitflyer) {
    return constant::kWalletGemini;
  }

  if (current_processor == constant::kWalletGemini) {
    return "";
  }

  return constant::kWalletUnBlinded;
}

bool HaveEnoughFundsToContribute(double* amount,
                                 const bool partial,
                                 const double balance) {
  DCHECK(amount);

  if (partial) {
    if (balance == 0) {
      return false;
    }

    if (*amount > balance) {
      *amount = balance;
    }

    return true;
  }

  if (*amount > balance) {
    return false;
  }

  return true;
}

int32_t GetVotesFromAmount(const double amount) {
  DCHECK_GT(constant::kVotePrice, 0);
  return std::floor(amount / constant::kVotePrice);
}

}  // namespace contribution
}  // namespace ledger
