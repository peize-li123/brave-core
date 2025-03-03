/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_opted_in_confirmation.h"

#include <memory>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate_mock.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::NiceMock;

class BraveAdsRedeemOptedInConfirmationTest : public UnitTestBase {
 protected:
  NiceMock<RedeemConfirmationDelegateMock> redeem_confirmation_delegate_mock_;
  base::WeakPtrFactory<RedeemConfirmationDelegateMock>
      confirmation_delegate_weak_factory_{&redeem_confirmation_delegate_mock_};
};

TEST_F(BraveAdsRedeemOptedInConfirmationTest, Redeem) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(expected_confirmation, _));

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(_, _, _))
      .Times(0);

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest, RetryRedeemingIfNoIssuers) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(*confirmation,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RedeemIfConfirmationWasPreviouslyCreated) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(expected_confirmation, _));

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(_, _, _))
      .Times(0);

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RetryRedeemingForFetchPaymentTokenHttpNotFoundResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, /*response_body*/ {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_NOT_FOUND, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(*confirmation,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingForFetchPaymentTokenHttpBadRequestResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, /*response_body*/ {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_BAD_REQUEST, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RetryRedeemingForFetchPaymentTokenHttpAcceptedResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, /*response_body*/ {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_ACCEPTED, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RetryRedeemingForFetchPaymentTokenHttpInternalServerErrorResponse) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_OK, /*response_body*/ {}}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_INTERNAL_SERVER_ERROR, /*response_body*/ {}}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RetryRedeemingIfInvalidJsonResponseBody) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ "INVALID_JSON"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(_,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyIdIsMissing) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyIdDoesNotMatchConfirmationId) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "393abadc-e9ae-4aac-a321-3307e0d527c6",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyPaymentTokenIsMissing) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyPublicKeyIsMissing) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyPublicKeyIsInvalid) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "INVALID",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       RetryRedeemingIfPublicKeyDoesNotExist) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "paymentToken" : {
                "publicKey" : "Si61i/8huYsx01ED6SZIOvDuD6GQV5LAi2CMu3NAVCQ=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw==",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ true,
                                           /*should_backoff*/ true));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyBatchProofIsMissing) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodyBatchProofIsInvalid) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "INVALID",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodySignedTokensAreMissing) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw=="
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfResponseBodySignedTokenIsInvalid) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "FWTZ5fOYITYlMWMYaxg254QWs+Pmd0dHzoor0mzIlQ8tWHagc7jm7UVJykqIo+ZSM+iK29mPuWJxPHpG4HypBw=="
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

TEST_F(BraveAdsRedeemOptedInConfirmationTest,
       DoNotRetryRedeemingIfFailedToVerifyAndUnblindTokens) {
  // Arrange
  const URLResponseMap url_responses = {
      {// Create confirmation request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/"
       "eyJwYXlsb2FkIjoie1wiYmxpbmRlZFBheW1lbnRUb2tlblwiOlwiRXY1SkU0LzlUWkkvNVR"
       "xeU45SldmSjFUbzBIQndRdzJyV2VBUGNkalgzUT1cIixcImJ1aWxkQ2hhbm5lbFwiOlwidG"
       "VzdFwiLFwiY3JlYXRpdmVJbnN0YW5jZUlkXCI6XCI3MDgyOWQ3MS1jZTJlLTQ0ODMtYTRjM"
       "C1lMWUyYmVlOTY1MjBcIixcInBheWxvYWRcIjp7fSxcInBsYXRmb3JtXCI6XCJ0ZXN0XCIs"
       "XCJ0eXBlXCI6XCJ2aWV3XCJ9Iiwic2lnbmF0dXJlIjoiRkhiczQxY1h5eUF2SnkxUE9HVUR"
       "yR1FoeUtjRkVMSXVJNU5yT3NzT2VLbUV6N1p5azZ5aDhweDQ0WmFpQjZFZkVRc0pWMEpQYm"
       "JmWjVUMGt2QmhEM0E9PSIsInQiOiJWV0tFZEliOG5Nd21UMWVMdE5MR3VmVmU2TlFCRS9TW"
       "GpCcHlsTFlUVk1KVFQrZk5ISTJWQmQyenRZcUlwRVdsZWF6TiswYk5jNGF2S2ZrY3YyRkw3"
       "Zz09In0=",
       {{net::HTTP_CREATED, R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.717Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6"
            }
          )"}}},
      {// Fetch payment token request
       "/v3/confirmation/8b742869-6e4a-490c-ac31-31b49130098a/paymentToken",
       {{net::HTTP_OK, /*response_body*/ R"(
            {
              "id" : "8b742869-6e4a-490c-ac31-31b49130098a",
              "createdAt" : "2020-04-20T10:27:11.717Z",
              "type" : "view",
              "modifiedAt" : "2020-04-20T10:27:11.736Z",
              "creativeInstanceId" : "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
              "paymentToken" : {
                "publicKey" : "bPE1QE65mkIgytffeu7STOfly+x10BXCGuk5pVlOHQU=",
                "batchProof" : "INVALID",
                "signedTokens" : [
                  "DHe4S37Cn1WaTbCC+ytiNTB2s5H0vcLzVcRgzRoO3lU="
                ]
              }
            }
          )"}}}};
  MockUrlResponses(ads_client_mock_, url_responses);

  BuildAndSetIssuers();

  privacy::SetUnblindedTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);

  // Act
  ConfirmationInfo expected_confirmation = *confirmation;
  expected_confirmation.was_created = true;

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedInConfirmation(_, _))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnDidRedeemOptedOutConfirmation(_))
      .Times(0);

  EXPECT_CALL(redeem_confirmation_delegate_mock_,
              OnFailedToRedeemConfirmation(expected_confirmation,
                                           /*should_retry*/ false,
                                           /*should_backoff*/ false));

  RedeemOptedInConfirmation::CreateAndRedeem(
      confirmation_delegate_weak_factory_.GetWeakPtr(), *confirmation);

  // Assert
}

}  // namespace brave_ads
