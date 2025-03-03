/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/gemini/gemini_server.h"

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {
namespace endpoint {

GeminiServer::GeminiServer(LedgerImpl& ledger)
    : post_account_(ledger),
      post_balance_(ledger),
      post_oauth_(ledger),
      post_recipient_id_(ledger) {}

GeminiServer::~GeminiServer() = default;

}  // namespace endpoint
}  // namespace ledger
