/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/url_request_builders/fetch_payment_token_url_request_builder.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_util.h"
#include "url/gurl.h"

namespace brave_ads {

FetchPaymentTokenUrlRequestBuilder::FetchPaymentTokenUrlRequestBuilder(
    ConfirmationInfo confirmation)
    : confirmation_(std::move(confirmation)) {
  DCHECK(IsValid(confirmation_));
}

// GET /v3/confirmation/{transactionId}/paymentToken

mojom::UrlRequestInfoPtr FetchPaymentTokenUrlRequestBuilder::Build() {
  mojom::UrlRequestInfoPtr url_request = mojom::UrlRequestInfo::New();
  url_request->url = BuildUrl();
  url_request->method = mojom::UrlRequestMethodType::kGet;

  return url_request;
}

///////////////////////////////////////////////////////////////////////////////

GURL FetchPaymentTokenUrlRequestBuilder::BuildUrl() const {
  const std::string url_host = confirmation_.ad_type == AdType::kSearchResultAd
                                   ? GetAnonymousSearchUrlHost()
                                   : GetAnonymousUrlHost();

  const std::string spec = base::ReplaceStringPlaceholders(
      "$1/v3/confirmation/$2/paymentToken",
      {url_host, confirmation_.transaction_id}, nullptr);

  return GURL(spec);
}

}  // namespace brave_ads
