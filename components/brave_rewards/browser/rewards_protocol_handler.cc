/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_protocol_handler.h"

#include <string>
#include <utility>

#include "base/strings/escape.h"
#include "base/strings/strcat.h"
#include "base/strings/string_piece_forward.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_rewards/common/url_constants.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace {

GURL TranslateUrl(const GURL& url) {
  if (!url.is_valid()) {
    return GURL();
  }

  std::string path = url.path();
  std::string query;

  if (url.has_query()) {
    query = base::StrCat({"?", base::EscapeExternalHandlerValue(url.query())});
  }

  base::ReplaceFirstSubstringAfterOffset(&path, 0, "/", "");

  return GURL(
      base::StrCat({
        "chrome://rewards",
        path,
        query
      }));
}

void LoadRewardsURL(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture) {
  content::WebContents* web_contents = std::move(web_contents_getter).Run();
  if (!web_contents) {
    return;
  }

  const auto ref_url = web_contents->GetURL();
  if (!ref_url.is_valid()) {
    return;
  }

  // Only accept rewards scheme from allowed domains
  const GURL bitflyer_staging_url(BUILDFLAG(BITFLYER_STAGING_URL));
  const GURL gemini_oauth_staging_url(BUILDFLAG(GEMINI_OAUTH_STAGING_URL));

  DCHECK(bitflyer_staging_url.is_valid() && bitflyer_staging_url.has_host());
  DCHECK(gemini_oauth_staging_url.is_valid() &&
         gemini_oauth_staging_url.has_host());

  const auto bitflyer_staging_host = bitflyer_staging_url.host_piece();
  const auto gemini_staging_host = gemini_oauth_staging_url.host_piece();
  const base::StringPiece kAllowedDomains[] = {
      "bitflyer.com",         // bitFlyer production
      bitflyer_staging_host,  // bitFlyer staging
      "gemini.com",           // Gemini production
      gemini_staging_host,    // Gemini staging
      "uphold.com",           // Uphold staging/production
  };
  bool allowed_domain = false;
  for (const auto domain : kAllowedDomains) {
    if (ref_url.DomainIs(domain)) {
      allowed_domain = true;
      break;
    }
  }

  if (!allowed_domain) {
    LOG(ERROR) << "Blocked invalid rewards url domain: " << ref_url.spec();
    return;
  }

  // we need to allow url to be processed even if rewards are off
  const auto new_url = TranslateUrl(url);
  web_contents->GetController().LoadURL(new_url, content::Referrer(),
      page_transition, std::string());
}

}  // namespace

namespace brave_rewards {

void HandleRewardsProtocol(const GURL& url,
                           content::WebContents::OnceGetter web_contents_getter,
                           ui::PageTransition page_transition,
                           bool has_user_gesture) {
  DCHECK(url.SchemeIs(kRewardsScheme));
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&LoadRewardsURL, url, std::move(web_contents_getter),
                     page_transition, has_user_gesture));
}

bool IsRewardsProtocol(const GURL& url) {
  return url.SchemeIs(kRewardsScheme);
}

}  // namespace brave_rewards
