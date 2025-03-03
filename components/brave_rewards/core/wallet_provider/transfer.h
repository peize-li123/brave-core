/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_TRANSFER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_TRANSFER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/database/database_external_transactions.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace wallet_provider {

class Transfer {
 public:
  explicit Transfer(LedgerImpl& ledger);

  virtual ~Transfer();

  void Run(const std::string& contribution_id,
           const std::string& destination,
           double amount,
           ledger::ResultCallback) const;

 protected:
  using MaybeCreateTransactionCallback =
      base::OnceCallback<void(mojom::ExternalTransactionPtr)>;

 private:
  void MaybeCreateTransaction(const std::string& contribution_id,
                              const std::string& destination,
                              const std::string& amount,
                              MaybeCreateTransactionCallback) const;

  void OnGetExternalTransaction(
      MaybeCreateTransactionCallback,
      std::string&& contribution_id,
      std::string&& destination,
      std::string&& amount,
      base::expected<mojom::ExternalTransactionPtr,
                     database::GetExternalTransactionError>) const;

  void SaveExternalTransaction(MaybeCreateTransactionCallback callback,
                               mojom::ExternalTransactionPtr) const;

  void OnSaveExternalTransaction(MaybeCreateTransactionCallback,
                                 mojom::ExternalTransactionPtr,
                                 mojom::Result) const;

 protected:
  virtual void CreateTransaction(MaybeCreateTransactionCallback,
                                 mojom::ExternalTransactionPtr) const;

  virtual void CommitTransaction(ledger::ResultCallback,
                                 mojom::ExternalTransactionPtr) const = 0;

  const raw_ref<LedgerImpl> ledger_;
};

}  // namespace wallet_provider
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_TRANSFER_H_
