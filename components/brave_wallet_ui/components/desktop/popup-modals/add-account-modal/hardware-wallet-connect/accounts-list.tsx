// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { create } from 'ethereum-blockies'
import { EntityId } from '@reduxjs/toolkit'
import { Checkbox, Select } from 'brave-ui/components'
import {
  ButtonsContainer,
  DisclaimerWrapper,
  HardwareWalletAccountCircle,
  HardwareWalletAccountListItem,
  HardwareWalletAccountListItemRow,
  HardwareWalletAccountsListContainer,
  SelectRow,
  SelectWrapper,
  LoadingWrapper,
  LoadIcon,
  AddressBalanceWrapper,
  NoSearchResultText
} from './style'
import {
  HardwareWalletDerivationPathLocaleMapping,
  HardwareWalletDerivationPathsMapping,
  SolHardwareWalletDerivationPathLocaleMapping
} from './types'
import { FilecoinNetwork } from '../../../../../common/hardware/types'
import { BraveWallet, WalletAccountType, CreateAccountOptionsType } from '../../../../../constants/types'
import { getLocale } from '../../../../../../common/locale'
import { NavButton } from '../../../../extension'
import { SearchBar } from '../../../../shared'
import { NetworkFilterSelector } from '../../../network-filter-selector'
import { DisclaimerText } from '../style'
import { Skeleton } from '../../../../shared/loading-skeleton/styles'

// Utils
import { reduceAddress } from '../../../../../utils/reduce-address'
import Amount from '../../../../../utils/amount'
import {
  useGetAccountTokenCurrentBalanceQuery,
  useGetNetworksRegistryQuery
} from '../../../../../common/slices/api.slice'
import { makeNetworkAsset } from '../../../../../options/asset-options'
import {
  networkEntityAdapter
} from '../../../../../common/slices/entities/network.entity'

interface Props {
  hardwareWallet: string
  accounts: BraveWallet.HardwareWalletAccount[]
  preAddedHardwareWalletAccounts: WalletAccountType[]
  onLoadMore: () => void
  selectedDerivationPaths: string[]
  setSelectedDerivationPaths: (paths: string[]) => void
  selectedDerivationScheme: string
  setSelectedDerivationScheme: (scheme: string) => void
  onAddAccounts: () => void
  filecoinNetwork: FilecoinNetwork
  onChangeFilecoinNetwork: (network: FilecoinNetwork) => void
  selectedAccountType: CreateAccountOptionsType
}

export const HardwareWalletAccountsList = ({
  accounts,
  preAddedHardwareWalletAccounts,
  hardwareWallet,
  selectedDerivationScheme,
  setSelectedDerivationScheme,
  setSelectedDerivationPaths,
  selectedDerivationPaths,
  onLoadMore,
  onAddAccounts,
  filecoinNetwork,
  onChangeFilecoinNetwork,
  selectedAccountType
}: Props) => {
  // queries
  const { data: networksRegistry } = useGetNetworksRegistryQuery()

  // state
  const [filteredAccountList, setFilteredAccountList] = React.useState<
    BraveWallet.HardwareWalletAccount[]
  >([])
  const [isLoadingMore, setIsLoadingMore] = React.useState<boolean>(false)
  const [selectedNetworkId, setSelectedNetworkId] = React.useState<EntityId>(
    selectedAccountType.coin === BraveWallet.CoinType.ETH
      ? BraveWallet.MAINNET_CHAIN_ID
      : selectedAccountType.coin === BraveWallet.CoinType.SOL
      ? BraveWallet.SOLANA_MAINNET
      : BraveWallet.FILECOIN_MAINNET
  )

  // memos
  const accountNativeAsset = React.useMemo(() => {
    if (!networksRegistry) {
      return undefined
    }
    return makeNetworkAsset(networksRegistry.entities[selectedNetworkId])
  }, [networksRegistry, selectedNetworkId])

  const networksSubset = React.useMemo(() => {
    if (!networksRegistry) {
      return []
    }
    return networksRegistry.idsByCoinType[selectedAccountType.coin].map(
      (id) => networksRegistry.entities[id] as BraveWallet.NetworkInfo
    )
  }, [networksRegistry, selectedAccountType])

  // computed
  const ethDerivationPathsEnum =
    HardwareWalletDerivationPathsMapping[hardwareWallet]
  const solDerivationPathsEnum = SolHardwareWalletDerivationPathLocaleMapping

  // methods
  const onSelectAccountCheckbox = (account: BraveWallet.HardwareWalletAccount) => () => {
    const { derivationPath } = account
    const isSelected = selectedDerivationPaths.includes(derivationPath)
    const updatedPaths = isSelected
      ? selectedDerivationPaths.filter((path) => path !== derivationPath)
      : [...selectedDerivationPaths, derivationPath]
    setSelectedDerivationPaths(updatedPaths)
  }

  const filterAccountList = (event: React.ChangeEvent<HTMLInputElement>) => {
    const search = event?.target?.value || ''
    if (search === '') {
      setFilteredAccountList(accounts)
    } else {
      const filteredList = accounts.filter((account) => {
        return (
          account.address.toLowerCase() === search.toLowerCase() ||
          account.address.toLowerCase().startsWith(search.toLowerCase())
        )
      })
      setFilteredAccountList(filteredList)
    }
  }

  const onClickLoadMore = () => {
    setIsLoadingMore(true)
    onLoadMore()
  }

  const isPreAddedAccount = React.useCallback((account: BraveWallet.HardwareWalletAccount) => {
    return preAddedHardwareWalletAccounts.some(e => e.address === account.address)
  }, [preAddedHardwareWalletAccounts])

  const onSelectNetwork = React.useCallback(
    (n: BraveWallet.NetworkInfo): void => {
      setSelectedNetworkId(networkEntityAdapter.selectId(n))
      if (selectedAccountType.coin === BraveWallet.CoinType.FIL) {
        onChangeFilecoinNetwork(n.chainId as FilecoinNetwork)
      }
    },
    [selectedAccountType.coin, onChangeFilecoinNetwork]
  )

  // effects
  React.useEffect(() => {
    setFilteredAccountList(accounts)
    setIsLoadingMore(false)
  }, [accounts])

  React.useEffect(() => {
    if (selectedNetworkId) {
      return
    }
    if (!networksRegistry) {
      return
    }

    // set network dropdown default value
    setSelectedNetworkId(
      networksRegistry.idsByCoinType[selectedAccountType.coin][0]
    )
  }, [networksRegistry, selectedAccountType])

  // render
  return (
    <>
      <SelectRow>
        <SelectWrapper>
          <NetworkFilterSelector
            networkListSubset={networksSubset}
            selectedNetwork={networksRegistry?.entities[selectedNetworkId]}
            onSelectNetwork={onSelectNetwork}
          />
          {selectedAccountType.coin === BraveWallet.CoinType.ETH ? (
            <Select value={selectedDerivationScheme} onChange={setSelectedDerivationScheme}>
              {Object.keys(ethDerivationPathsEnum).map((path, index) => {
                const pathValue = ethDerivationPathsEnum[path]
                const pathLocale = HardwareWalletDerivationPathLocaleMapping[pathValue]
                return (
                  <div data-value={pathValue} key={index}>
                    {pathLocale}
                  </div>
                )
              })}
            </Select>
          ) : null}
          {selectedAccountType.coin === BraveWallet.CoinType.SOL ? (
            <Select value={selectedDerivationScheme} onChange={setSelectedDerivationScheme}>
              {Object.keys(solDerivationPathsEnum).map((path, index) => {
                const pathLocale = solDerivationPathsEnum[path]
                return (
                  <div data-value={path} key={index}>
                    {pathLocale}
                  </div>
                )
              })}
            </Select>
          ) : null}
        </SelectWrapper>
      </SelectRow>
      <DisclaimerWrapper>
        <DisclaimerText>{getLocale('braveWalletSwitchHDPathTextHardwareWallet')}</DisclaimerText>
      </DisclaimerWrapper>
      <SearchBar
        placeholder={getLocale('braveWalletSearchScannedAccounts')}
        action={filterAccountList}
      />
      <HardwareWalletAccountsListContainer>
        {accounts.length === 0 && (
          <LoadingWrapper>
            <LoadIcon size={'big'} />
          </LoadingWrapper>
        )}

        {accounts.length > 0 && filteredAccountList?.length === 0 && (
          <NoSearchResultText>
            {getLocale('braveWalletConnectHardwareSearchNothingFound')}
          </NoSearchResultText>
        )}

        {accounts.length > 0 && filteredAccountList.length > 0 && (
          <>
            {filteredAccountList?.map((account) => {
              return (
                <AccountListItem
                  key={account.derivationPath}
                  balanceAsset={accountNativeAsset}
                  account={account}
                  selected={
                    selectedDerivationPaths.includes(account.derivationPath) ||
                    isPreAddedAccount(account)
                  }
                  disabled={isPreAddedAccount(account)}
                  onSelect={onSelectAccountCheckbox(account)}
                />
              )
            })}
          </>
        )}
      </HardwareWalletAccountsListContainer>
      <ButtonsContainer>
        <NavButton
          onSubmit={onClickLoadMore}
          text={isLoadingMore ? getLocale('braveWalletLoadingMoreAccountsHardwareWallet')
            : getLocale('braveWalletLoadMoreAccountsHardwareWallet')}
          buttonType='primary'
          disabled={isLoadingMore || accounts.length === 0}
        />
        <NavButton
          onSubmit={onAddAccounts}
          text={getLocale('braveWalletAddCheckedAccountsHardwareWallet')}
          buttonType='primary'
          disabled={accounts.length === 0 || selectedDerivationPaths.length === 0}
        />
      </ButtonsContainer>
    </>
  )
}

interface AccountListItemProps {
  account: BraveWallet.HardwareWalletAccount
  onSelect: () => void
  selected: boolean
  disabled: boolean
  balanceAsset?: Pick<
    BraveWallet.BlockchainToken,
    | 'chainId'
    | 'contractAddress'
    | 'isErc721'
    | 'isNft'
    | 'symbol'
    | 'tokenId'
    | 'decimals'
  >
}

function AccountListItem({
  account,
  onSelect,
  selected,
  disabled,
  balanceAsset
}: AccountListItemProps) {
  // queries
  const { data: balanceResult, isFetching: isLoadingBalance } =
    useGetAccountTokenCurrentBalanceQuery(
      {
        account,
        token: {
          chainId: balanceAsset?.chainId || '',
          contractAddress: balanceAsset?.contractAddress || '',
          isErc721: balanceAsset?.isErc721 || false,
          isNft: balanceAsset?.isNft || false,
          symbol: balanceAsset?.symbol || '',
          tokenId: balanceAsset?.tokenId || ''
        }
      },
      { skip: !balanceAsset }
    )

  // memos
  const orb = React.useMemo(() => {
    return create({
      seed: account.address.toLowerCase(),
      size: 8,
      scale: 16
    }).toDataURL()
  }, [account.address])

  const balance = React.useMemo(() => {
    if (
      isLoadingBalance ||
      !balanceResult?.balance ||
      balanceAsset?.decimals === undefined
    ) {
      return undefined
    }

    return new Amount(balanceResult.balance)
      .divideByDecimals(balanceAsset.decimals)
      .formatAsAsset(undefined, balanceAsset.symbol)
  }, [
    isLoadingBalance,
    balanceResult?.balance,
    balanceAsset?.decimals,
    balanceAsset?.symbol
  ])

  // render
  return (
    <HardwareWalletAccountListItem>
      <HardwareWalletAccountCircle orb={orb} />
      <HardwareWalletAccountListItemRow>
        <AddressBalanceWrapper>
          <div>{reduceAddress(account.address)}</div>
        </AddressBalanceWrapper>
        {isLoadingBalance ? (
          <Skeleton width={'140px'} height={'100%'} />
        ) : (
          <AddressBalanceWrapper>{balance}</AddressBalanceWrapper>
        )}
        <Checkbox value={{ selected }} onChange={onSelect} disabled={disabled}>
          <div data-key={'selected'} />
        </Checkbox>
      </HardwareWalletAccountListItemRow>
    </HardwareWalletAccountListItem>
  )
}

export default HardwareWalletAccountsList