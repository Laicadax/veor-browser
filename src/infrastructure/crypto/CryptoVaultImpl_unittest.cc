// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "infrastructure/crypto/CryptoVaultImpl.h"
#include "core/test/VeorTestBase.h"

namespace veor {

class CryptoVaultTest : public VeorTestBase {
 protected:
  void SetUp() override {
    VeorTestBase::SetUp();
    vault_ = std::make_unique<CryptoVaultImpl>();
  }

  std::unique_ptr<CryptoVaultImpl> vault_;
};

TEST_F(CryptoVaultTest, GenerateRandom) {
  auto result = vault_->GenerateRandom(32);
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.Unwrap().size(), 32u);

  // Two calls should produce different results (with overwhelming probability)
  auto result2 = vault_->GenerateRandom(32);
  ASSERT_TRUE(result2.IsOk());
  EXPECT_NE(result.Unwrap(), result2.Unwrap());
}

TEST_F(CryptoVaultTest, HashSha256) {
  auto result = vault_->HashSha256("hello");
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.Unwrap().size(), 32u);  // SHA-256 = 32 bytes

  // Deterministic
  auto result2 = vault_->HashSha256("hello");
  ASSERT_TRUE(result2.IsOk());
  EXPECT_EQ(result.Unwrap(), result2.Unwrap());

  // Different input -> different hash
  auto result3 = vault_->HashSha256("world");
  ASSERT_TRUE(result3.IsOk());
  EXPECT_NE(result.Unwrap(), result3.Unwrap());
}

TEST_F(CryptoVaultTest, EncryptDecryptRoundtrip) {
  auto key_result = vault_->GenerateRandom(32);
  ASSERT_TRUE(key_result.IsOk());
  std::vector<uint8_t> key = std::move(key_result).Unwrap();

  std::string plaintext = "Hello, VEOR! This is a secret message.";

  auto enc_result = vault_->Encrypt(plaintext, key);
  ASSERT_TRUE(enc_result.IsOk());
  std::vector<uint8_t> ciphertext = std::move(enc_result).Unwrap();

  // Ciphertext should be longer than plaintext (nonce + tag overhead)
  EXPECT_GT(ciphertext.size(), plaintext.size());

  auto dec_result = vault_->Decrypt(ciphertext, key);
  ASSERT_TRUE(dec_result.IsOk());
  EXPECT_EQ(dec_result.Unwrap(), plaintext);
}

TEST_F(CryptoVaultTest, DecryptWithWrongKeyFails) {
  auto key1 = vault_->GenerateRandom(32).Unwrap();
  auto key2 = vault_->GenerateRandom(32).Unwrap();

  auto ciphertext = vault_->Encrypt("secret", key1).Unwrap();

  auto dec_result = vault_->Decrypt(ciphertext, key2);
  EXPECT_TRUE(dec_result.IsErr());
}

TEST_F(CryptoVaultTest, EncryptWithInvalidKeySize) {
  auto short_key = vault_->GenerateRandom(16).Unwrap();
  auto result = vault_->Encrypt("test", short_key);
  EXPECT_TRUE(result.IsErr());
}

TEST_F(CryptoVaultTest, EmptyPlaintext) {
  auto key = vault_->GenerateRandom(32).Unwrap();
  auto enc = vault_->Encrypt("", key);
  ASSERT_TRUE(enc.IsOk());

  auto dec = vault_->Decrypt(enc.Unwrap(), key);
  ASSERT_TRUE(dec.IsOk());
  EXPECT_EQ(dec.Unwrap(), "");
}

TEST_F(CryptoVaultTest, LargePlaintext) {
  auto key = vault_->GenerateRandom(32).Unwrap();
  std::string plaintext(10000, 'A');

  auto enc = vault_->Encrypt(plaintext, key);
  ASSERT_TRUE(enc.IsOk());

  auto dec = vault_->Decrypt(enc.Unwrap(), key);
  ASSERT_TRUE(dec.IsOk());
  EXPECT_EQ(dec.Unwrap(), plaintext);
}

}  // namespace veor
