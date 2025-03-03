/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"

#include <map>
#include <utility>

#include "base/base64.h"
#include "base/environment.h"
#include "base/strings/strcat.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_topics_builder.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/string_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/url_util.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

constexpr char kEthereum[] = "ethereum";
constexpr char kSolana[] = "solana";
constexpr char kPolygon[] = "polygon";
constexpr char kArbitrum[] = "arbitrum";
constexpr char kOptimism[] = "optimism";
constexpr char kAvalanche[] = "avalanche";
constexpr char kBsc[] = "bsc";
constexpr char kEthereumGoerli[] = "ethereum-goerli";
constexpr char kSolanaTestnet[] = "solana-testnet";
constexpr char kSolanaDevnet[] = "solana-devnet";

absl::optional<std::string> ChainIdToSimpleHashChainId(
    const std::string& chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      chain_id_lookup({
          {brave_wallet::mojom::kMainnetChainId, kEthereum},
          {brave_wallet::mojom::kSolanaMainnet, kSolana},
          {brave_wallet::mojom::kPolygonMainnetChainId, kPolygon},
          {brave_wallet::mojom::kArbitrumMainnetChainId, kArbitrum},
          {brave_wallet::mojom::kOptimismMainnetChainId, kOptimism},
          {brave_wallet::mojom::kAvalancheMainnetChainId, kAvalanche},
          {brave_wallet::mojom::kBinanceSmartChainMainnetChainId, kBsc},
          {brave_wallet::mojom::kGoerliChainId, kEthereumGoerli},
          {brave_wallet::mojom::kSolanaTestnet, kSolanaTestnet},
          {brave_wallet::mojom::kSolanaDevnet, kSolanaDevnet},
      });
  if (!chain_id_lookup->contains(chain_id)) {
    return absl::nullopt;
  }

  return chain_id_lookup->at(chain_id);
}

absl::optional<std::string> SimpleHashChainIdToChainId(
    const std::string& simple_hash_chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      simple_hash_chain_id_lookup({
          {kEthereum, brave_wallet::mojom::kMainnetChainId},
          {kSolana, brave_wallet::mojom::kSolanaMainnet},
          {kPolygon, brave_wallet::mojom::kPolygonMainnetChainId},
          {kArbitrum, brave_wallet::mojom::kArbitrumMainnetChainId},
          {kOptimism, brave_wallet::mojom::kOptimismMainnetChainId},
          {kAvalanche, brave_wallet::mojom::kAvalancheMainnetChainId},
          {kBsc, brave_wallet::mojom::kBinanceSmartChainMainnetChainId},
          {kEthereumGoerli, brave_wallet::mojom::kGoerliChainId},
          {kSolanaTestnet, brave_wallet::mojom::kSolanaTestnet},
          {kSolanaDevnet, brave_wallet::mojom::kSolanaDevnet},
      });
  if (!simple_hash_chain_id_lookup->contains(simple_hash_chain_id)) {
    return absl::nullopt;
  }

  return simple_hash_chain_id_lookup->at(simple_hash_chain_id);
}

base::flat_map<std::string, std::string> MakeBraveServicesKeyHeader() {
  base::flat_map<std::string, std::string> request_headers;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string brave_key(BUILDFLAG(BRAVE_SERVICES_KEY));
  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }
  request_headers["x-brave-key"] = std::move(brave_key);

  return request_headers;
}

}  // namespace

namespace brave_wallet {

AssetDiscoveryManager::AssetDiscoveryManager(
    std::unique_ptr<APIRequestHelper> api_request_helper,
    BraveWalletService* wallet_service,
    JsonRpcService* json_rpc_service,
    KeyringService* keyring_service,
    PrefService* prefs)
    : api_request_helper_(std::move(api_request_helper)),
      wallet_service_(wallet_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
}

AssetDiscoveryManager::~AssetDiscoveryManager() = default;

const std::vector<std::string>&
AssetDiscoveryManager::GetAssetDiscoverySupportedEthChains() {
  if (supported_chains_for_testing_.size() > 0) {
    return supported_chains_for_testing_;
  }

  // Use the hardcoded list of BalanceScanner contract addresses to determine
  // which chains are supported.
  static const base::NoDestructor<std::vector<std::string>>
      asset_discovery_supported_chains([] {
        std::vector<std::string> supported_chains;
        for (const auto& entry : GetEthBalanceScannerContractAddresses()) {
          supported_chains.push_back(entry.first);
        }
        return supported_chains;
      }());
  return *asset_discovery_supported_chains;
}

void AssetDiscoveryManager::DiscoverSolAssets(
    const std::vector<std::string>& account_addresses,
    bool triggered_by_accounts_added) {
  // Convert each account address to SolanaAddress and check validity
  std::vector<SolanaAddress> solana_addresses;
  for (const auto& address : account_addresses) {
    absl::optional<SolanaAddress> solana_address =
        SolanaAddress::FromBase58(address);
    if (!solana_address.has_value()) {
      continue;
    }

    if ((*solana_address).IsValid()) {
      solana_addresses.push_back(*solana_address);
    }
  }

  if (solana_addresses.empty()) {
    CompleteDiscoverAssets(std::vector<mojom::BlockchainTokenPtr>(),
                           triggered_by_accounts_added);
    return;
  }

  // TODO(nvonpentz): When custom networks are supported, we need to check
  // that the active network is our own that supports this RPC call.

  const auto barrier_callback =
      base::BarrierCallback<std::vector<SolanaAddress>>(
          solana_addresses.size(),
          base::BindOnce(&AssetDiscoveryManager::MergeDiscoveredSolanaAssets,
                         weak_ptr_factory_.GetWeakPtr(),
                         triggered_by_accounts_added));
  for (const auto& account_address : solana_addresses) {
    json_rpc_service_->GetSolanaTokenAccountsByOwner(
        account_address,
        base::BindOnce(&AssetDiscoveryManager::OnGetSolanaTokenAccountsByOwner,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(barrier_callback)));
  }
}

void AssetDiscoveryManager::OnGetSolanaTokenAccountsByOwner(
    base::OnceCallback<void(std::vector<SolanaAddress>)> barrier_callback,
    const std::vector<SolanaAccountInfo>& token_accounts,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess || token_accounts.empty()) {
    std::move(barrier_callback).Run(std::vector<SolanaAddress>());
    return;
  }

  // Add each token account to the all_discovered_contract_addresses list
  std::vector<SolanaAddress> discovered_mint_addresses;
  for (const auto& token_account : token_accounts) {
    // Decode Base64
    const absl::optional<std::vector<uint8_t>> data =
        base::Base64Decode(token_account.data);
    if (data.has_value()) {
      // Decode the address
      const absl::optional<SolanaAddress> mint_address =
          DecodeMintAddress(data.value());
      if (mint_address.has_value()) {
        // Add the contract address to the list
        discovered_mint_addresses.push_back(mint_address.value());
      }
    }
  }

  std::move(barrier_callback).Run(discovered_mint_addresses);
}

void AssetDiscoveryManager::MergeDiscoveredSolanaAssets(
    bool triggered_by_accounts_added,
    const std::vector<std::vector<SolanaAddress>>&
        all_discovered_contract_addresses) {
  // Create vector of all discovered contract addresses
  std::vector<std::string> discovered_mint_addresses;
  for (const auto& discovered_contract_address_list :
       all_discovered_contract_addresses) {
    for (const auto& discovered_contract_address :
         discovered_contract_address_list) {
      discovered_mint_addresses.push_back(
          discovered_contract_address.ToBase58());
    }
  }

  // Convert vector to flat_set
  base::flat_set<std::string> discovered_mint_addresses_set(
      std::move(discovered_mint_addresses));

  auto internal_callback = base::BindOnce(
      &AssetDiscoveryManager::OnGetSolanaTokenRegistry,
      weak_ptr_factory_.GetWeakPtr(), triggered_by_accounts_added,
      std::move(discovered_mint_addresses_set));

  // Fetch SOL registry tokens
  BlockchainRegistry::GetInstance()->GetAllTokens(mojom::kSolanaMainnet,
                                                  mojom::CoinType::SOL,
                                                  std::move(internal_callback));
}

void AssetDiscoveryManager::OnGetSolanaTokenRegistry(
    bool triggered_by_accounts_added,
    const base::flat_set<std::string>& discovered_mint_addresses,
    std::vector<mojom::BlockchainTokenPtr> sol_token_registry) {
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;
  for (const auto& token : sol_token_registry) {
    if (discovered_mint_addresses.contains(token->contract_address)) {
      if (!BraveWalletService::AddUserAsset(token.Clone(), prefs_)) {
        continue;
      }
      discovered_tokens.push_back(token.Clone());
    }
  }

  CompleteDiscoverAssets(std::move(discovered_tokens),
                         triggered_by_accounts_added);
}

void AssetDiscoveryManager::DiscoverEthAssets(
    const std::vector<std::string>& account_addresses,
    bool triggered_by_accounts_added) {
  if (account_addresses.empty()) {
    CompleteDiscoverAssets({}, triggered_by_accounts_added);
    return;
  }

  std::vector<mojom::BlockchainTokenPtr> user_assets =
      BraveWalletService::GetUserAssets(prefs_);
  TokenListMap token_list_map =
      BlockchainRegistry::GetInstance()->GetEthTokenListMap(
          GetAssetDiscoverySupportedEthChains());

  // Create set of all user assets per chain to use to ensure we don't
  // include assets the user has already added in the call to the BalanceScanner
  base::flat_map<std::string, base::flat_set<base::StringPiece>>
      user_assets_per_chain;
  for (const auto& user_asset : user_assets) {
    user_assets_per_chain[user_asset->chain_id].insert(
        user_asset->contract_address);
  }

  // Create a map of chain_id to a map contract address to BlockchainToken
  // to easily lookup tokens by contract address when the results of the
  // BalanceScanner calls are merged
  base::flat_map<std::string,
                 base::flat_map<std::string, mojom::BlockchainTokenPtr>>
      chain_id_to_contract_address_to_token;

  // Create a map of chain_id to a vector of contract addresses (strings, rather
  // than BlockchainTokens) to pass to GetERC20TokenBalances
  base::flat_map<std::string, std::vector<std::string>>
      chain_id_to_contract_addresses;

  // Populate the chain_id_to_contract_addresses using the token_list_map of
  // BlockchainTokenPtrs
  for (auto& [chain_id, token_list] : token_list_map) {
    for (auto& token : token_list) {
      if (!user_assets_per_chain[chain_id].contains(token->contract_address)) {
        chain_id_to_contract_addresses[chain_id].push_back(
            token->contract_address);
        chain_id_to_contract_address_to_token[chain_id]
                                             [token->contract_address] =
                                                 std::move(token);
      }
    }
  }

  // Use a barrier callback to wait for all GetERC20TokenBalances calls to
  // complete (one for each account address).
  const auto barrier_callback =
      base::BarrierCallback<std::map<std::string, std::vector<std::string>>>(
          account_addresses.size() * chain_id_to_contract_addresses.size(),
          base::BindOnce(&AssetDiscoveryManager::MergeDiscoveredEthAssets,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(chain_id_to_contract_address_to_token),
                         triggered_by_accounts_added));

  // For each account address, call GetERC20TokenBalances for each chain ID
  for (const auto& account_address : account_addresses) {
    for (const auto& [chain_id, contract_addresses] :
         chain_id_to_contract_addresses) {
      auto internal_callback = base::BindOnce(
          &AssetDiscoveryManager::OnGetERC20TokenBalances,
          weak_ptr_factory_.GetWeakPtr(), barrier_callback, chain_id,
          triggered_by_accounts_added, contract_addresses);
      json_rpc_service_->GetERC20TokenBalances(contract_addresses,
                                               account_address, chain_id,
                                               std::move(internal_callback));
    }
  }
}

void AssetDiscoveryManager::OnGetERC20TokenBalances(
    base::OnceCallback<void(std::map<std::string, std::vector<std::string>>)>
        barrier_callback,
    const std::string& chain_id,
    bool triggered_by_accounts_added,
    const std::vector<std::string>&
        contract_addresses,  // Contract addresses queried for
    std::vector<mojom::ERC20BalanceResultPtr> balance_results,
    mojom::ProviderError error,
    const std::string& error_message) {
  // If the request failed, return an empty map
  if (error != mojom::ProviderError::kSuccess || balance_results.empty()) {
    std::move(barrier_callback).Run({});
    return;
  }

  // Create a map of chain_id to a vector of contract addresses that have a
  // balance greater than 0
  std::map<std::string, std::vector<std::string>>
      chain_id_to_contract_addresses_with_balance;

  // Populate the map using the balance_results
  for (size_t i = 0; i < balance_results.size(); i++) {
    if (balance_results[i]->balance.has_value()) {
      uint256_t balance_uint;
      bool success =
          HexValueToUint256(balance_results[i]->balance.value(), &balance_uint);
      if (success && balance_uint > 0) {
        chain_id_to_contract_addresses_with_balance[chain_id].push_back(
            contract_addresses[i]);
      }
    }
  }

  std::move(barrier_callback).Run(chain_id_to_contract_addresses_with_balance);
}

void AssetDiscoveryManager::MergeDiscoveredEthAssets(
    base::flat_map<std::string,
                   base::flat_map<std::string, mojom::BlockchainTokenPtr>>
        chain_id_to_contract_address_to_token,
    bool triggered_by_accounts_added,
    const std::vector<std::map<std::string, std::vector<std::string>>>&
        discovered_assets_results) {
  // Create a vector of BlockchainTokenPtrs to return
  std::vector<mojom::BlockchainTokenPtr> discovered_tokens;

  // Keep track of which contract addresses have been seen per chain
  base::flat_map<std::string, base::flat_set<base::StringPiece>>
      seen_contract_addresses;
  for (const auto& discovered_assets_result : discovered_assets_results) {
    for (const auto& [chain_id, contract_addresses] :
         discovered_assets_result) {
      for (const auto& contract_address : contract_addresses) {
        // Skip if seen
        if (seen_contract_addresses[chain_id].contains(contract_address)) {
          continue;
        }

        // Add to seen and discovered_tokens if not
        seen_contract_addresses[chain_id].insert(contract_address);
        auto token = std::move(
            chain_id_to_contract_address_to_token[chain_id][contract_address]);
        if (token && BraveWalletService::AddUserAsset(token.Clone(), prefs_)) {
          discovered_tokens.push_back(std::move(token));
        }
      }
    }
  }

  CompleteDiscoverAssets(std::move(discovered_tokens),
                         triggered_by_accounts_added);
}

// Calls
// https://simplehash.wallet.brave.com/api/v0/nfts/owners?chains={chains}&wallet_addresses={wallet_addresses}
void AssetDiscoveryManager::FetchNFTsFromSimpleHash(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    FetchNFTsFromSimpleHashCallback callback) {
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL)) {
    std::move(callback).Run({});
    return;
  }

  GURL url = GetSimpleHashNftsByWalletUrl(account_address, chain_ids);
  if (!url.is_valid()) {
    std::move(callback).Run({});
    return;
  }
  std::vector<mojom::BlockchainTokenPtr> nfts_so_far = {};
  auto internal_callback =
      base::BindOnce(&AssetDiscoveryManager::OnFetchNFTsFromSimpleHash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(nfts_so_far),
                     coin, std::move(callback));

  api_request_helper_->Request("GET", url, "", "", true,
                               std::move(internal_callback),
                               MakeBraveServicesKeyHeader());
}

void AssetDiscoveryManager::OnFetchNFTsFromSimpleHash(
    std::vector<mojom::BlockchainTokenPtr> nfts_so_far,
    mojom::CoinType coin,
    FetchNFTsFromSimpleHashCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(std::move(nfts_so_far));
    return;
  }

  // Invalid JSON becomes an empty string after sanitization
  if (api_request_result.body().empty()) {
    std::move(callback).Run(std::move(nfts_so_far));
    return;
  }

  absl::optional<std::pair<GURL, std::vector<mojom::BlockchainTokenPtr>>>
      result = ParseNFTsFromSimpleHash(api_request_result.value_body(), coin);
  if (!result) {
    std::move(callback).Run(std::move(nfts_so_far));
    return;
  }

  for (auto& token : result.value().second) {
    nfts_so_far.push_back(std::move(token));
  }

  // If there is a next page, fetch it
  if (result.value().first.is_valid()) {
    auto internal_callback =
        base::BindOnce(&AssetDiscoveryManager::OnFetchNFTsFromSimpleHash,
                       weak_ptr_factory_.GetWeakPtr(), std::move(nfts_so_far),
                       coin, std::move(callback));
    api_request_helper_->Request("GET", result.value().first, "", "", true,
                                 std::move(internal_callback),
                                 MakeBraveServicesKeyHeader());
    return;
  }

  // Otherwise, return the nfts_so_far
  std::move(callback).Run(std::move(nfts_so_far));
}

absl::optional<std::pair<GURL, std::vector<mojom::BlockchainTokenPtr>>>
AssetDiscoveryManager::ParseNFTsFromSimpleHash(const base::Value& json_value,
                                               mojom::CoinType coin) {
  // Parses responses like this
  // {
  //   "next_cursor": null,
  //   "next": null,
  //   "previous": null,
  //   "nfts": [
  //     {
  //       "nft_id":
  //       "ethereum.0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85.53762001732575849527995595036249427730510390651723189221519398504820492711584",
  //       "chain": "ethereum",
  //       "contract_address": "0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85",
  //       "token_id":
  //       "53762001732575849527995595036249427730510390651723189221519398504820492711584",
  //       "name": "stochasticparrot.eth",
  //       "description": "stochasticparrot.eth, an ENS name.",
  //       "previews": {
  //         "image_small_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=s250",
  //         "image_medium_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8",
  //         "image_large_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=s1000",
  //         "image_opengraph_url":
  //         "https://lh3.googleusercontent.com/KV7QzwejzheyvEsTvYogBPJnKUdNlrid6PwMnrA4WzOU0eWfOF6w6RRdnVM7n7DHWBFVAy7ocS-mAdM_GmwTvw4DV7kLdogg_e8=k-w1200-s2400-rj",
  //         "blurhash": "UCBiFG+PX7aPz5tht3az%HowWWa#j0WVagj?",
  //         "predominant_color": "#5b99f3"
  //       },
  //       "image_url":
  //       "https://cdn.simplehash.com/assets/6e174a2e0091ffd5c0c63904366a62da8890508b01e7e85b13d5475b038e6544.svg",
  //       "image_properties": {
  //         "width": 1000,
  //         "height": 1000,
  //         "size": 101101,
  //         "mime_type": "image/svg+xml"
  //       },
  //       "video_url": null,
  //       "video_properties": null,
  //       "audio_url": null,
  //       "audio_properties": null,
  //       "model_url": null,
  //       "model_properties": null,
  //       "background_color": null,
  //       "external_url": "https://app.ens.domains/name/stochasticparrot.eth",
  //       "created_date": "2022-12-08T00:52:23",
  //       "status": "minted",
  //       "token_count": 1,
  //       "owner_count": 1,
  //       "owners": [
  //         {
  //           "owner_address": "0xB4B2802129071b2B9eBb8cBB01EA1E4D14B34961",
  //           "quantity": 1,
  //           "first_acquired_date": "2022-12-08T00:52:23",
  //           "last_acquired_date": "2022-12-08T00:52:23"
  //         }
  //       ],
  //       "last_sale": null,
  //       "first_created": {
  //         "minted_to": "0x283af0b28c62c092c9727f1ee09c02ca627eb7f5",
  //         "quantity": 1,
  //         "timestamp": "2022-12-08T00:52:23",
  //         "block_number": 16136530,
  //         "transaction":
  //         "0xe06e9d1f6a3bfcc9f1fc6b9731524c69d09569cab746f2e24571a010d4ce99eb",
  //         "transaction_initiator":
  //         "0xb4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
  //       },
  //       "contract": {
  //         "type": "ERC721",
  //         "name": null,
  //         "symbol": null,
  //         "deployed_by": "0x4fe4e666be5752f1fdd210f4ab5de2cc26e3e0e8",
  //         "deployed_via_contract": null
  //       },
  //       "collection": {
  //         "collection_id": "e34baafc65deb66d52d11be5d44f523e",
  //         "name": "ENS: Ethereum Name Service",
  //         "description": "Ethereum Name Service (ENS) domains are secure
  //         domain names for the decentralized world. ENS domains provide a way
  //         for users to map human readable names to blockchain and
  //         non-blockchain resources, like Ethereum addresses, IPFS hashes, or
  //         website URLs. ENS domains can be bought and sold on secondary
  //         markets.", "image_url":
  //         "https://lh3.googleusercontent.com/yXNjPUCCTHyvYNarrb81ln31I6hUIaoPzlGU8kki-OohiWuqxfrIkMaOdLzcO4iGuXcvE5mgCZ-ds9tZotEJi3hdkNusheEK_w2V",
  //         "banner_image_url": null,
  //         "external_url": "https://ens.domains",
  //         "twitter_username": "ensdomains",
  //         "discord_url": null,
  //         "marketplace_pages": [
  //           {
  //             "marketplace_id": "opensea",
  //             "marketplace_name": "OpenSea",
  //             "marketplace_collection_id": "ens",
  //             "nft_url":
  //             "https://opensea.io/assets/ethereum/0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85/53762001732575849527995595036249427730510390651723189221519398504820492711584",
  //             "collection_url": "https://opensea.io/collection/ens",
  //             "verified": true
  //           }
  //         ],
  //         "metaplex_mint": null,
  //         "metaplex_first_verified_creator": null,
  //         "spam_score": 0,
  //         "floor_prices": [
  //           {
  //             "marketplace_id": "opensea",
  //             "marketplace_name": "OpenSea",
  //             "value": 1,
  //             "payment_token": {
  //               "payment_token_id": "ethereum.native",
  //               "name": "Ether",
  //               "symbol": "ETH",
  //               "address": null,
  //               "decimals": 18
  //             }
  //           }
  //         ],
  //         "distinct_owner_count": 667362,
  //         "distinct_nft_count": 2962658,
  //         "total_quantity": 2962620,
  //         "top_contracts": [
  //           "ethereum.0x57f1887a8bf19b14fc0df6fd9b2acc9af147ea85"
  //         ]
  //       },
  //       "rarity": {
  //         "rank": null,
  //         "score": null,
  //         "unique_attributes": null
  //       },
  //       "extra_metadata": {
  //         "attributes": [
  //           {
  //             "trait_type": "Created Date",
  //             "value": "1670460743000",
  //             "display_type": "date"
  //           },
  //           {
  //             "trait_type": "Length",
  //             "value": "16",
  //             "display_type": "number"
  //           },
  //           {
  //             "trait_type": "Segment Length",
  //             "value": "16",
  //             "display_type": "number"
  //           },
  //           {
  //             "trait_type": "Character Set",
  //             "value": "letter",
  //             "display_type": "string"
  //           },
  //           {
  //             "trait_type": "Registration Date",
  //             "value": "1670460743000",
  //             "display_type": "date"
  //           },
  //           {
  //             "trait_type": "Expiration Date",
  //             "value": "1733574647000",
  //             "display_type": "date"
  //           }
  //         ],
  //         "is_normalized": true,
  //         "name_length": 16,
  //         "segment_length": 16,
  //         "version": 0,
  //         "background_image":
  //         "https://metadata.ens.domains/mainnet/avatar/stochasticparrot.eth",
  //         "image_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/0x76dc36f2ff546436694c7eee18598a1309f0d382934ac4fd977ed24f3b9bb6a0/image",
  //         "image_original_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/0x76dc36f2ff546436694c7eee18598a1309f0d382934ac4fd977ed24f3b9bb6a0/image",
  //         "animation_original_url": null,
  //         "metadata_original_url":
  //         "https://metadata.ens.domains/mainnet/0x57f1887a8BF19b14fC0dF6Fd9B2acc9Af147eA85/53762001732575849527995595036249427730510390651723189221519398504820492711584/"
  //       }
  //     },
  //     ...
  // }

  // Only ETH and SOL NFTs are supported.
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL)) {
    return absl::nullopt;
  }

  const base::Value::Dict* dict = json_value.GetIfDict();
  if (!dict) {
    return absl::nullopt;
  }

  GURL nextURL;
  auto* next = dict->FindString("next");
  if (next) {
    nextURL = GURL(*next);
  }

  // Validate the URL of the next page using HTTPS and SimpleHash host
  // and replace the host with the proxy host
  if (!nextURL.is_empty() && (nextURL.host() != GURL(kSimpleHashUrl).host() ||
                              !(nextURL.scheme() == url::kHttpsScheme))) {
    nextURL = GURL();
  } else {
    GURL::Replacements replacements;
    std::string replacement_host = GURL(kSimpleHashBraveProxyUrl).host();
    replacements.SetHostStr(replacement_host);
    nextURL = nextURL.ReplaceComponents(replacements);
  }

  const base::Value::List* nfts = dict->FindList("nfts");
  if (!nfts) {
    return absl::nullopt;
  }

  std::vector<mojom::BlockchainTokenPtr> nft_tokens;
  for (const auto& nft_value : *nfts) {
    auto token = mojom::BlockchainToken::New();

    // Skip all tokens with a collection.spam_score > 0.
    const base::Value::Dict* nft = nft_value.GetIfDict();
    if (!nft) {
      continue;
    }
    auto* collection = nft->FindDict("collection");
    if (!collection) {
      continue;
    }
    absl::optional<int> spam_score = collection->FindInt("spam_score");
    if (!spam_score || *spam_score > 0) {
      continue;
    }

    // contract_address (required)
    auto* contract_address = nft->FindString("contract_address");
    if (!contract_address) {
      continue;
    }
    token->contract_address = *contract_address;

    // chain_id (required)
    auto* chain = nft->FindString("chain");
    if (!chain) {
      continue;
    }
    absl::optional<std::string> chain_id = SimpleHashChainIdToChainId(*chain);
    if (!chain_id) {
      continue;
    }
    token->chain_id = *chain_id;

    // name
    auto* name = nft->FindString("name");
    if (name) {
      token->name = *name;
    }

    // logo
    auto* logo = nft->FindString("image_url");
    if (logo) {
      token->logo = *logo;
    }

    // is_erc20
    token->is_erc20 = false;

    // The contract dict has the standard information
    // so we skip if it's not there.
    auto* contract = nft->FindDict("contract");
    if (!contract) {
      continue;
    }
    auto* type = contract->FindString("type");
    if (!type) {
      continue;
    }

    // is_erc721
    if (coin == mojom::CoinType::ETH) {
      bool is_erc721 = base::EqualsCaseInsensitiveASCII(*type, "ERC721");
      if (!is_erc721) {
        continue;
      }
      token->is_erc721 = true;
    } else {  // mojom::CoinType::SOL
      // Solana NFTs must be "NonFungible"
      if (!base::EqualsCaseInsensitiveASCII(*type, "NonFungible")) {
        continue;
      }
      token->is_erc721 = false;
    }

    // is_erc1155 TODO(nvonpentz) Support ERC1155 tokens by parsing type above
    // https://github.com/brave/brave-browser/issues/29304
    token->is_erc1155 = false;

    // is_nft
    token->is_nft = true;

    // symbol
    auto* symbol = contract->FindString("symbol");
    if (!symbol) {
      // If symbol is null, assign an empty string to avoid display issues
      // on the frontend
      token->symbol = "";
    } else {
      token->symbol = *symbol;
    }

    // decimals
    token->decimals = 0;

    // visible
    token->visible = true;

    // token_id (required for ETH only)
    if (coin == mojom::CoinType::ETH) {
      auto* token_id = nft->FindString("token_id");
      if (!token_id) {
        continue;
      }
      uint256_t token_id_uint256;
      if (!Base10ValueToUint256(*token_id, &token_id_uint256)) {
        continue;
      }
      token->token_id = Uint256ValueToHex(token_id_uint256);
    }

    // coin
    token->coin = coin;

    nft_tokens.push_back(std::move(token));
  }

  return std::make_pair(nextURL, std::move(nft_tokens));
}

void AssetDiscoveryManager::DiscoverNFTsOnAllSupportedChains(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses,
    bool triggered_by_accounts_added) {
  // Users must opt-in for NFT discovery
  if (!prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled)) {
    CompleteDiscoverAssets({}, triggered_by_accounts_added);
    return;
  }

  auto it_eth = account_addresses.find(mojom::CoinType::ETH);
  const auto& eth_account_addresses = it_eth != account_addresses.end()
                                          ? it_eth->second
                                          : std::vector<std::string>();
  auto it_sol = account_addresses.find(mojom::CoinType::SOL);
  const auto& sol_account_addresses = it_sol != account_addresses.end()
                                          ? it_sol->second
                                          : std::vector<std::string>();
  const auto barrier_callback =
      base::BarrierCallback<std::vector<mojom::BlockchainTokenPtr>>(
          eth_account_addresses.size() + sol_account_addresses.size(),
          base::BindOnce(&AssetDiscoveryManager::MergeDiscoveredNFTs,
                         weak_ptr_factory_.GetWeakPtr(),
                         triggered_by_accounts_added));
  for (const auto& account_address : eth_account_addresses) {
    FetchNFTsFromSimpleHash(account_address,
                            GetAssetDiscoverySupportedEthChains(),
                            mojom::CoinType::ETH, std::move(barrier_callback));
  }

  for (const auto& account_address : sol_account_addresses) {
    FetchNFTsFromSimpleHash(account_address, {mojom::kSolanaMainnet},
                            mojom::CoinType::SOL, std::move(barrier_callback));
  }
}

void AssetDiscoveryManager::MergeDiscoveredNFTs(
    bool triggered_by_accounts_added,
    const std::vector<std::vector<mojom::BlockchainTokenPtr>>& nfts) {
  // De-dupe the NFTs
  base::flat_set<mojom::BlockchainTokenPtr> seen_nft;
  std::vector<mojom::BlockchainTokenPtr> discovered_nfts;
  for (const auto& nft_list : nfts) {
    for (const auto& nft : nft_list) {
      if (seen_nft.contains(nft)) {
        continue;
      }
      seen_nft.insert(nft.Clone());

      // Add the NFT to the user's assets
      if (BraveWalletService::AddUserAsset(nft.Clone(), prefs_)) {
        discovered_nfts.push_back(nft.Clone());
      }
    }
  }

  CompleteDiscoverAssets(std::move(discovered_nfts),
                         triggered_by_accounts_added);
}

// Called when asset discovery has completed for
void AssetDiscoveryManager::CompleteDiscoverAssets(
    std::vector<mojom::BlockchainTokenPtr> discovered_assets_for_bucket,
    bool triggered_by_accounts_added) {
  if (discover_assets_completed_callback_for_testing_) {
    std::vector<mojom::BlockchainTokenPtr> discovered_assets_for_bucket_clone;
    for (const auto& asset : discovered_assets_for_bucket) {
      discovered_assets_for_bucket_clone.push_back(asset.Clone());
    }
    discover_assets_completed_callback_for_testing_.Run(
        std::move(discovered_assets_for_bucket_clone));
  }

  // Do not emit event or modify remaining_buckets_ count if
  // call was triggered by an AccountsAdded event
  if (triggered_by_accounts_added) {
    return;
  }

  // Complete the call by decrementing remaining_buckets_, storing the
  // discovered assets for later, and emitting the event if this was the final
  // chain to finish
  remaining_buckets_--;
  for (auto& asset : discovered_assets_for_bucket) {
    discovered_assets_.push_back(std::move(asset));
  }

  if (remaining_buckets_ == 0) {
    wallet_service_->OnDiscoverAssetsCompleted(std::move(discovered_assets_));
    discovered_assets_.clear();
  }
}

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChainsAccountsAdded(
    mojom::CoinType coin,
    const std::vector<std::string>& account_addresses) {
  std::map<mojom::CoinType, std::vector<std::string>> account_addresses_by_coin;
  if (coin == mojom::CoinType::ETH) {
    account_addresses_by_coin[mojom::CoinType::ETH] = account_addresses;
    DiscoverEthAssets(account_addresses, true);
  } else if (coin == mojom::CoinType::SOL) {
    account_addresses_by_coin[mojom::CoinType::SOL] = account_addresses;
    DiscoverSolAssets(account_addresses, true);
  }
  DiscoverNFTsOnAllSupportedChains(account_addresses_by_coin, true);
}

void AssetDiscoveryManager::DiscoverAssetsOnAllSupportedChainsRefresh(
    const std::map<mojom::CoinType, std::vector<std::string>>&
        account_addresses) {
  // Simple client side rate limiting (only applies to refreshes)
  const base::Time assets_last_discovered_at =
      prefs_->GetTime(kBraveWalletLastDiscoveredAssetsAt);
  if (!assets_last_discovered_at.is_null() &&
      ((base::Time::Now() - base::Minutes(kAssetDiscoveryMinutesPerRequest)) <
       assets_last_discovered_at)) {
    wallet_service_->OnDiscoverAssetsCompleted({});
    return;
  }
  prefs_->SetTime(kBraveWalletLastDiscoveredAssetsAt, base::Time::Now());

  // Return early and do not send a notification
  // if a discover assets process is flight already
  if (remaining_buckets_ != 0) {
    return;
  }

  remaining_buckets_ = 3;  // 1 for ETH + 1 for SOL + 1 for NFTs

  auto sol_it = account_addresses.find(mojom::CoinType::SOL);
  const auto& sol_account_addresses = sol_it != account_addresses.end()
                                          ? sol_it->second
                                          : std::vector<std::string>();
  if (sol_it != account_addresses.end()) {
    DiscoverSolAssets(sol_account_addresses, false);
  }

  auto eth_it = account_addresses.find(mojom::CoinType::ETH);
  const auto& eth_account_addresses = eth_it != account_addresses.end()
                                          ? eth_it->second
                                          : std::vector<std::string>();
  if (eth_it != account_addresses.end()) {
    DiscoverEthAssets(eth_account_addresses, false);
  }

  DiscoverNFTsOnAllSupportedChains(account_addresses, false);
}

void AssetDiscoveryManager::AccountsAdded(
    mojom::CoinType coin,
    const std::vector<std::string>& addresses) {
  if (!(coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL) ||
      addresses.size() == 0u) {
    return;
  }
  DiscoverAssetsOnAllSupportedChainsAccountsAdded(coin, addresses);
}

// static
// Parses the Account object for the `mint` field which is a 32 byte public key.
// See
// https://github.com/solana-labs/solana-program-library/blob/f97a3dc7cf0e6b8e346d473a8c9d02de7b213cfd/token/program/src/state.rs#L86-L105
absl::optional<SolanaAddress> AssetDiscoveryManager::DecodeMintAddress(
    const std::vector<uint8_t>& data) {
  if (data.size() < 32) {
    return absl::nullopt;
  }

  std::vector<uint8_t> pub_key_bytes(data.begin(), data.begin() + 32);
  return SolanaAddress::FromBytes(pub_key_bytes);
}

// static
// Creates a URL like
// https://simplehash.wallet.brave.com/api/v0/nfts/owners?chains={chains}&wallet_addresses={wallet_addresses}
GURL AssetDiscoveryManager::GetSimpleHashNftsByWalletUrl(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids) {
  if (chain_ids.empty() || account_address.empty()) {
    return GURL();
  }

  std::string urlStr =
      base::StrCat({kSimpleHashBraveProxyUrl, "/api/v0/nfts/owners"});

  std::string chain_ids_param;
  for (const auto& chain_id : chain_ids) {
    absl::optional<std::string> simple_hash_chain_id =
        ChainIdToSimpleHashChainId(chain_id);
    if (simple_hash_chain_id) {
      if (!chain_ids_param.empty()) {
        chain_ids_param += ",";
      }
      chain_ids_param += *simple_hash_chain_id;
    }
  }

  if (chain_ids_param.empty()) {
    return GURL();
  }

  GURL url = GURL(urlStr);
  url = net::AppendQueryParameter(url, "chains", chain_ids_param);
  url = net::AppendQueryParameter(url, "wallet_addresses", account_address);
  return url;
}

}  // namespace brave_wallet
