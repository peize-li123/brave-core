/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/batch_dleq_proof.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/dleq_proof.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signed_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/signing_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_preimage.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/verification_signature.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::cbr {

namespace {
constexpr char kMessage[] = "The quick brown fox jumps over the lazy dog";
}  // namespace

TEST(BraveAdsChallengeBypassRistrettoTest, ProveAndVerifyUnblindedToken) {
  // Server prepares a random signing key.
  SigningKey signing_key;
  EXPECT_TRUE(signing_key.has_value());

  // Return the signing keys associated public key.
  const absl::optional<PublicKey> public_key = signing_key.GetPublicKey();
  EXPECT_TRUE(public_key);

  // Client prepares a random token and blinding scalar.
  Token token;
  EXPECT_TRUE(token.has_value());

  // Client blinds the token.
  const absl::optional<BlindedToken> blinded_token = token.Blind();
  EXPECT_TRUE(blinded_token);

  // Server signs the blinded token.
  const absl::optional<SignedToken> signed_token =
      signing_key.Sign(*blinded_token);
  EXPECT_TRUE(signed_token);

  // Server signs a DLEQ proof.
  DLEQProof dleq_proof(*blinded_token, *signed_token, signing_key);
  EXPECT_TRUE(dleq_proof.has_value());

  // Client verifies the DLEQ proof using the public key.
  EXPECT_TRUE(dleq_proof.Verify(*blinded_token, *signed_token, *public_key));

  // Server returns a batch DLEQ proof.
  const std::vector<BlindedToken> blinded_tokens = {*blinded_token};
  const std::vector<SignedToken> signed_tokens = {*signed_token};
  BatchDLEQProof batch_dleq_proof(blinded_tokens, signed_tokens, signing_key);
  EXPECT_TRUE(batch_dleq_proof.has_value());

  // Client verifies the batch DLEQ proof and uses the blinding scalar to
  // unblind the returned signed tokens.
  const std::vector<Token> tokens = {token};
  const absl::optional<std::vector<UnblindedToken>> unblinded_tokens =
      batch_dleq_proof.VerifyAndUnblind(tokens, blinded_tokens, signed_tokens,
                                        *public_key);
  EXPECT_TRUE(unblinded_tokens);

  // Redeem unblinded tokens
  for (const auto& unblinded_token : *unblinded_tokens) {
    // Derive a shared verification key from the unblinded token.
    absl::optional<VerificationKey> verification_key =
        unblinded_token.DeriveVerificationKey();
    EXPECT_TRUE(verification_key);

    // Sign the message using the shared verification key.
    const absl::optional<VerificationSignature> verification_signature =
        verification_key->Sign(kMessage);
    EXPECT_TRUE(verification_signature);

    // Server decodes the token preimage from the unblinded token.
    const absl::optional<TokenPreimage> token_preimage =
        unblinded_token.GetTokenPreimage();
    EXPECT_TRUE(token_preimage);

    // Server derives the unblinded token using the server signing key and the
    // token preimage.
    const absl::optional<UnblindedToken> server_unblinded_token =
        signing_key.RederiveUnblindedToken(*token_preimage);
    EXPECT_TRUE(server_unblinded_token);

    // Server derives the shared verification key from the unblinded token.
    absl::optional<VerificationKey> server_verification_key =
        server_unblinded_token->DeriveVerificationKey();
    EXPECT_TRUE(server_verification_key);

    // Server proves and verifies the validity of the unblinded_token.
    EXPECT_TRUE(
        server_verification_key->Verify(*verification_signature, kMessage));
  }
}

}  // namespace brave_ads::privacy::cbr
