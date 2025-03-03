/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_context_helper.h"
#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
#include "chrome/browser/ui/browser.h"

namespace rewards_browsertest {

class RewardsBrowserTestContribution
    : public brave_rewards::RewardsServiceObserver {
 public:
  RewardsBrowserTestContribution();
  ~RewardsBrowserTestContribution() override;

  void Initialize(
      Browser* browser,
      brave_rewards::RewardsServiceImpl* rewards_service);

  void TipViaCode(const std::string& publisher_key,
                  const double amount,
                  const ledger::mojom::PublisherStatus status,
                  const bool recurring = false);

  void TipPublisher(const GURL& url,
                    bool set_monthly,
                    const int32_t number_of_contributions = 0,
                    const int32_t selection = 0,
                    double custom_amount = 0.0);

  void VerifyTip(const double amount,
                 const bool monthly,
                 const bool via_code = false);

  void AddBalance(const double balance);

  double GetBalance();

  std::string GetExternalBalance();

  double GetReconcileTipTotal();

  void WaitForTipReconcileCompleted();

  void UpdateContributionBalance(
      const double amount,
      const bool verified = false,
      const ledger::mojom::ContributionProcessor processor =
          ledger::mojom::ContributionProcessor::BRAVE_TOKENS);

  void WaitForMultipleTipReconcileCompleted(const int32_t needed);

  void WaitForACReconcileCompleted();

  void IsBalanceCorrect();

  void WaitForMultipleACReconcileCompleted(
    const int32_t needed);

  ledger::mojom::Result GetACStatus();

  std::vector<ledger::mojom::Result> GetMultipleACStatus();

  void SetUpUpholdWallet(brave_rewards::RewardsServiceImpl* rewards_service,
                         const double balance,
                         const ledger::mojom::WalletStatus status =
                             ledger::mojom::WalletStatus::kConnected);

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
  void SetUpGeminiWallet(brave_rewards::RewardsServiceImpl* rewards_service,
                         const double balance,
                         const ledger::mojom::WalletStatus status =
                             ledger::mojom::WalletStatus::kConnected);
#endif

  std::vector<ledger::mojom::Result> GetMultipleTipStatus();

  ledger::mojom::Result GetTipStatus();

 private:
  content::WebContents* contents();

  void OnReconcileComplete(
      brave_rewards::RewardsService* rewards_service,
      const ledger::mojom::Result result,
      const std::string& contribution_id,
      const double amount,
      const ledger::mojom::RewardsType type,
      const ledger::mojom::ContributionProcessor processor) override;

  void WaitForRecurringTipToBeSaved();

  void OnRecurringTipSaved(
      brave_rewards::RewardsService* rewards_service,
      const bool success) override;

  std::string GetStringBalance();

  double balance_ = 0;
  double external_balance_ = 0;
  double reconciled_tip_total_ = 0;

  bool tip_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_tip_completed_loop_;
  ledger::mojom::Result tip_reconcile_status_ =
      ledger::mojom::Result::LEDGER_ERROR;
  bool recurring_tip_saved_ = false;
  std::unique_ptr<base::RunLoop> wait_for_recurring_tip_saved_loop_;
  bool multiple_tip_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_multiple_tip_completed_loop_;
  int32_t multiple_tip_reconcile_count_ = 0;
  int32_t multiple_tip_reconcile_needed_ = 0;
  std::vector<ledger::mojom::Result> multiple_tip_reconcile_status_ = {};
  bool multiple_ac_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_multiple_ac_completed_loop_;
  int32_t multiple_ac_reconcile_count_ = 0;
  int32_t multiple_ac_reconcile_needed_ = 0;
  std::vector<ledger::mojom::Result> multiple_ac_reconcile_status_ = {};
  bool ac_reconcile_completed_ = false;
  std::unique_ptr<base::RunLoop> wait_for_ac_completed_loop_;
  ledger::mojom::Result ac_reconcile_status_ =
      ledger::mojom::Result::LEDGER_ERROR;

  raw_ptr<Browser> browser_ = nullptr;  // NOT OWNED
  raw_ptr<brave_rewards::RewardsServiceImpl> rewards_service_ =
      nullptr;  // NOT OWNED
  std::unique_ptr<RewardsBrowserTestContextHelper> context_helper_;
};

}  // namespace rewards_browsertest
#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTRIBUTION_H_
