/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/flags_util.h"

#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switch_values_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switches_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

mojom::EnvironmentType ChooseEnvironmentType() {
  const absl::optional<mojom::EnvironmentType> environment_type =
      ParseEnvironmentCommandLineSwitch();
  return environment_type.value_or(kDefaultEnvironmentType);
}

}  // namespace

mojom::FlagsPtr BuildFlags() {
  mojom::FlagsPtr flags = mojom::Flags::New();

  flags->should_debug = ParseDebugCommandLineSwitch();

  flags->did_override_from_command_line =
      DidOverrideFeaturesFromCommandLine() ||
      DidOverrideCommandLineSwitchValues() || DidOverrideCommandLineSwitches();

  flags->environment_type = ChooseEnvironmentType();

  return flags;
}

}  // namespace brave_ads
