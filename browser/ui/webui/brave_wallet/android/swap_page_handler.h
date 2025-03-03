// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_HANDLER_H_

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "ui/webui/mojo_web_ui_controller.h"

class SwapPageHandler : WalletPageHandler {
 public:
  SwapPageHandler(
      mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
      Profile* profile,
      ui::MojoWebUIController* webui_controller);

  SwapPageHandler(const SwapPageHandler&) = delete;
  SwapPageHandler& operator=(const SwapPageHandler&) = delete;
  ~SwapPageHandler() override;

  void ShowApprovePanelUI() override;

 private:
  ui::MojoWebUIController* const webui_controller_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WALLET_ANDROID_SWAP_PAGE_HANDLER_H_
