// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css'
import Icon from '@brave/leo/react/icon'
import { WalletButton, Text } from '../../shared/style'

export const StyledWrapper = styled.div<{ yPosition?: number }>`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-conent: center;
  padding: 8px 8px 0px 8px;
  background-color: ${leo.color.container.background};
  border-radius: 8px;
  box-shadow: 0px 1px 4px rgba(0, 0, 0, 0.25);
  position: absolute;
  top: ${(p) => p.yPosition !== undefined ? p.yPosition : 35}px;
  right: 15px;
  z-index: 20;
 `

export const PopupButton = styled(WalletButton)`
  display: flex;
  align-items: center;
  justify-content: flex-start;
  text-align: left;
  cursor: pointer;
  width: 220px;
  border-radius: 8px;
  outline: none;
  border: none;
  background: none;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
  &:hover {
    background-color: ${leo.color.divider.subtle};
  }
`

export const PopupButtonText = styled.span`
  flex: 1;
  font-family: Poppins;
  font-style: normal;
  font-size: 14px;
  font-weight: 400;
  line-height: 24px;
  color: ${leo.color.text.primary};
`

export const ButtonIcon = styled(Icon)`
  --leo-icon-size: 18px;
  color: ${leo.color.icon.default};
  margin-right: 16px;
`

export const CheckBoxRow = styled.label`
  display: flex;
  align-items: center;
  justify-content: space-between;
  cursor: pointer;
  width: 220px;
  border-radius: 8px;
  padding: 12px 8px;
  margin: 0px 0px 8px 0px;
  background-color: transparent;
`

export const SectionTitle = styled(Text)`
  text-transform: uppercase;
`
