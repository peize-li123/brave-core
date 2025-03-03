/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"

#include <utility>

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_value_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/device_id_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/enabled_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/locale_diagnostic_entry.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

DiagnosticManager::DiagnosticManager() {
  SetEntry(std::make_unique<EnabledDiagnosticEntry>());
  SetEntry(std::make_unique<DeviceIdDiagnosticEntry>());
  SetEntry(std::make_unique<LocaleDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogIdDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogLastUpdatedDiagnosticEntry>());
  SetEntry(std::make_unique<LastUnIdleTimeDiagnosticEntry>());
}

DiagnosticManager::~DiagnosticManager() {}

// static
DiagnosticManager* DiagnosticManager::GetInstance() {
  auto* diagnostic_manager = GlobalState::GetInstance()->GetDiagnosticManager();
  DCHECK(diagnostic_manager);
  return diagnostic_manager;
}

void DiagnosticManager::SetEntry(
    std::unique_ptr<DiagnosticEntryInterface> entry) {
  DCHECK(entry);

  const DiagnosticEntryType type = entry->GetType();
  diagnostics_[type] = std::move(entry);
}

void DiagnosticManager::GetDiagnostics(GetDiagnosticsCallback callback) const {
  std::move(callback).Run(DiagnosticsToValue(diagnostics_));
}

}  // namespace brave_ads
