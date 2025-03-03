// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// types
import {
  BraveWallet,
  SerializableTransactionInfo,
  WalletAccountType,
  WalletRoutes
} from '../../../constants/types'

// options
import { AllNetworksOption } from '../../../options/network-filter-options'
import { AllAccountsOption } from '../../../options/account-filter-options'

// utils
import { getLocale } from 'brave-ui'
import {
  AccountInfoEntity,
  accountInfoEntityAdaptor,
  accountInfoEntityAdaptorInitialState,
  selectAllAccountInfosFromQuery
} from '../../../common/slices/entities/account-info.entity'
import {
  selectAllUserAssetsFromQueryResult,
  selectAllBlockchainTokensFromQueryResult
} from '../../../common/slices/entities/blockchain-token.entity'
import {
  networkEntityAdapter
} from '../../../common/slices/entities/network.entity'
import {
  filterTransactionsBySearchValue,
  makeSearchableTransaction
} from '../../../utils/search-utils'

// hooks
import {
  useGetAccountInfosRegistryQuery,
  useGetNetworksRegistryQuery,
  useLazyGetTransactionsQuery,
  useGetUserTokensRegistryQuery,
  useGetTokensRegistryQuery
} from '../../../common/slices/api.slice'

// components
import { AccountFilterSelector } from '../../../components/desktop/account-filter-selector/account-filter-selector'
import { NetworkFilterSelector } from '../../../components/desktop/network-filter-selector/index'
import { PortfolioTransactionItem } from '../../../components/desktop'
import { SearchBar } from '../../../components/shared'

// styles
import {
  Column,
  LoadingIcon,
  Text,
  VerticalSpacer,
  ScrollableColumn
} from '../../../components/shared/style'
import {
  LoadingSkeletonStyleProps,
  Skeleton
} from '../../../components/shared/loading-skeleton/styles'
import { SearchAndFiltersRow } from './transaction-screen.styles'

interface Params {
  address?: string | null
  chainId?: string | null
  chainCoinType?: BraveWallet.CoinType | null
  transactionId?: string | null
}

const txListItemSkeletonProps: LoadingSkeletonStyleProps = {
  width: '100%',
  height: '60px',
  enableAnimation: true
}

export const TransactionsScreen: React.FC = () => {
  // routing
  const history = useHistory()

  // state
  const [searchValue, setSearchValue] = React.useState<string>('')
  const [txInfos, setTxInfos] = React.useState<SerializableTransactionInfo[]>(
    []
  )
  const [isLoadingTxsList, setIsLoadingTxsList] = React.useState<boolean>(false)

  // route params
  const {
    address,
    chainId,
    transactionId,
    chainCoinType
  } = React.useMemo(() => {
    const searchParams = new URLSearchParams(history.location.search)
    return {
      address: searchParams.get('address'),
      chainId: searchParams.get('chainId'),
      transactionId: searchParams.get('transactionId'),
      chainCoinType:
        Number(searchParams.get('chainCoinType')) || BraveWallet.CoinType.ETH
    }
  }, [history.location.search])

  // queries
  const {
    data: accountInfosRegistry = accountInfoEntityAdaptorInitialState,
    isLoading: isLoadingAccounts,
    accounts
  } = useGetAccountInfosRegistryQuery(undefined, {
    selectFromResult: res => ({
      ...res,
      accounts: selectAllAccountInfosFromQuery(res)
    })
  })

  const { data: knownTokensList } = useGetTokensRegistryQuery(undefined, {
    selectFromResult: (res) => ({
      isLoading: res.isLoading,
      data: selectAllBlockchainTokensFromQueryResult(res)
    })
  })

  const { data: userTokensList } = useGetUserTokensRegistryQuery(
    undefined,
    {
      selectFromResult: (res) => ({
        isLoading: res.isLoading,
        data: selectAllUserAssetsFromQueryResult(res)
      })
    }
  )

  const [fetchTransactions] = useLazyGetTransactionsQuery()

  const { data: networksRegistry } = useGetNetworksRegistryQuery()

  // computed / memos
  const specificNetworkFromParam =
    chainId &&
    chainId !== AllNetworksOption.chainId &&
    chainCoinType !== undefined &&
    networksRegistry
      ? networksRegistry.entities[
          networkEntityAdapter.selectId({
            chainId,
            coin: chainCoinType
          })
        ]
      : undefined

  const foundNetworkFromParam = chainId
    ? chainId === AllNetworksOption.chainId
      ? AllNetworksOption
      : specificNetworkFromParam
    : undefined

  const isLoadingDeps = isLoadingAccounts

  const foundAccountFromParam = address ? accountInfosRegistry.entities[
    accountInfoEntityAdaptor.selectId({ address })
  ] : undefined

  const fetchTxsForAccounts = React.useCallback((accounts: Array<Pick<AccountInfoEntity, 'address' | 'coin'>>) => {
    setIsLoadingTxsList(true)

    Promise.all(
      accounts.map(({ coin, address }) => {
        return fetchTransactions({
          address: address,
          coinType: coin,
          chainId:
            foundNetworkFromParam?.chainId === AllNetworksOption.chainId
              ? null
              : foundNetworkFromParam?.chainId || null
        }).unwrap()
      }
    ))
    .then(txInfos => {
      setTxInfos(txInfos.flat())
      setIsLoadingTxsList(false)
    })
    .catch(error => {
      console.error(error)
      // stop loading if a error other than empty tokens list was thrown
      if (!error?.toString().includes('Unable to fetch Tokens Registry')) {
        setIsLoadingTxsList(false)
        return
      }
      // retry when browser is idle
      requestIdleCallback(() => {
        fetchTxsForAccounts(accounts)
      })
    })
  }, [
    fetchTransactions,
    foundNetworkFromParam?.chainId
  ])

  React.useEffect(() => {
    if (isLoadingDeps) {
      return
    }

    if (
      foundAccountFromParam?.address &&
      foundAccountFromParam?.coin !== undefined
    ) {
      fetchTxsForAccounts([{
        address: foundAccountFromParam.address,
        coin: foundAccountFromParam.coin
      }])
      return
    }

    // get txs for all accounts
    fetchTxsForAccounts(accounts)
  }, [
    isLoadingDeps,
    fetchTxsForAccounts,
    foundAccountFromParam?.address,
    foundAccountFromParam?.coin
  ])

  const combinedTokensList = React.useMemo(() => {
    return userTokensList.concat(knownTokensList)
  }, [userTokensList, knownTokensList])

  const txsForSelectedChain = React.useMemo(() => {
    return chainId && chainId !== AllNetworksOption.chainId
      ? txInfos.filter(tx => tx.chainId === chainId)
      : txInfos
  }, [chainId, txInfos])

  const combinedTokensListForSelectedChain = React.useMemo(() => {
    return chainId && chainId !== AllNetworksOption.chainId
      ? combinedTokensList.filter(token => token.chainId === chainId)
      : combinedTokensList
  }, [chainId, combinedTokensList])

  const searchableTransactions = React.useMemo(() => {
    return txsForSelectedChain.map((tx) => {
      return makeSearchableTransaction(
        tx,
        combinedTokensListForSelectedChain,
        networksRegistry,
        accountInfosRegistry
      )
    })
  }, [
    txsForSelectedChain,
    combinedTokensListForSelectedChain,
    networksRegistry,
    accountInfosRegistry
  ])

  const filteredTransactions = React.useMemo(() => {
    if (searchValue.trim() === '') {
      return searchableTransactions
    }

    return filterTransactionsBySearchValue(
      searchableTransactions,
      searchValue.toLowerCase()
    )
  }, [
    searchValue,
    searchableTransactions
  ])

  // methods
  const onSelectAccount = React.useCallback(
    ({ address, coin }: WalletAccountType): void => {
      history.push(
        updatePageParams({
          address: address || undefined,
          // reset chains filter on account select
          chainId: AllNetworksOption.chainId,
          chainCoinType: coin,
          transactionId
        })
      )
    },
    [history, transactionId]
  )

  const onSelectNetwork = React.useCallback(
    ({ chainId, coin }: BraveWallet.NetworkInfo) => {
      history.push(
        updatePageParams({
          address: foundAccountFromParam?.address || AllAccountsOption.id,
          chainId,
          chainCoinType: coin,
          transactionId
        })
      )
    },
    [history, foundAccountFromParam?.address, transactionId]
  )

  // render
  if (isLoadingDeps) {
    return <Column fullHeight>
      <LoadingIcon opacity={100} size='50px' color='interactive05' />
    </Column>
  }

  return (
    <>
      <SearchAndFiltersRow>
        <Column flex={1} style={{ minWidth: '25%' }} alignItems='flex-start'>
          <SearchBar
            placeholder={getLocale('braveWalletSearchText')}
            action={(e) => setSearchValue(e.target.value)}
            value={searchValue}
          />
        </Column>
        <AccountFilterSelector
          selectedAccount={foundAccountFromParam || AllAccountsOption}
          onSelectAccount={onSelectAccount}
          selectedNetwork={foundNetworkFromParam || AllNetworksOption}
        />
        <NetworkFilterSelector
          selectedAccount={foundAccountFromParam || AllAccountsOption}
          selectedNetwork={foundNetworkFromParam || AllNetworksOption}
          onSelectNetwork={onSelectNetwork}
        />
      </SearchAndFiltersRow>

      {isLoadingTxsList
        ? <Column fullHeight fullWidth>
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
            <Skeleton {...txListItemSkeletonProps} />
            <VerticalSpacer space={8} />
          </Column>
        : <>
            {txsForSelectedChain?.length === 0 &&
              <Column fullHeight gap={'24px'}>
                <VerticalSpacer space={14} />
                <Text textSize='18px' isBold>
                  {getLocale('braveWalletNoTransactionsYet')}
                </Text>
                <Text textSize='14px'>
                  {getLocale('braveWalletNoTransactionsYetDescription')}
                </Text>
              </Column>
            }

          <ScrollableColumn>
            {filteredTransactions.map(tx =>
              <PortfolioTransactionItem
                key={tx.id}
                displayAccountName
                transaction={tx}
              />
            )}
          </ScrollableColumn>

            {txsForSelectedChain &&
              txsForSelectedChain.length !== 0 &&
              filteredTransactions.length === 0 &&
              <Column fullHeight>
                <Text textSize='14px'>
                  {getLocale('braveWalletConnectHardwareSearchNothingFound')}
                </Text>
              </Column>
            }
          </>
      }
    </>
  )
}

export default TransactionsScreen

const updatePageParams = ({
  address,
  chainId,
  transactionId,
  chainCoinType
}: Params) => {
  const params = new URLSearchParams()
  if (address) {
    params.append('address', address)
  }
  if (chainId) {
    params.append('chainId', chainId)
  }
  if (transactionId) {
    params.append('transactionId', transactionId)
  }
  if (chainCoinType) {
    params.append('chainCoinType', chainCoinType.toString())
  }
  const paramsString = params.toString()

  return `${WalletRoutes.Activity}${paramsString ? `?${paramsString}` : ''}`
}
