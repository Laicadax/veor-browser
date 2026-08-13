// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "infrastructure/crypto/CryptoVaultImpl.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "core/logging/VeorLogger.h"

namespace veor {

namespace {

CryptoError MakeCryptoError(int code, const std::string& message) {
  return CryptoError{code, message};
}

}  // namespace

CryptoVaultImpl::CryptoVaultImpl() = default;

Result<std::vector<uint8_t>, CryptoError> CryptoVaultImpl::GenerateRandom(
    size_t length) {
  std::vector<uint8_t> buffer(length);
  if (RAND_bytes(buffer.data(), static_cast<int>(length)) != 1) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "RAND_bytes failed"));
  }
  return Result<std::vector<uint8_t>, CryptoError>::Ok(std::move(buffer));
}

Result<std::vector<uint8_t>, CryptoError> CryptoVaultImpl::HashSha256(
    base::StringPiece data) {
  std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
  SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hash.data());
  return Result<std::vector<uint8_t>, CryptoError>::Ok(std::move(hash));
}

Result<std::vector<uint8_t>, CryptoError> CryptoVaultImpl::Encrypt(
    base::StringPiece plaintext,
    const std::vector<uint8_t>& key) {
  if (key.size() != kAesKeySize) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(0, "Invalid key size: expected " + std::to_string(kAesKeySize)));
  }

  // Generate nonce
  auto nonce_result = GenerateRandom(kAesNonceSize);
  if (nonce_result.IsErr()) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(nonce_result.UnwrapErr());
  }
  std::vector<uint8_t> nonce = std::move(nonce_result).Unwrap();

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_CIPHER_CTX_new failed"));
  }

  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_EncryptInit_ex failed"));
  }

  std::vector<uint8_t> ciphertext(plaintext.size());
  int len = 0;
  if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                        reinterpret_cast<const uint8_t*>(plaintext.data()),
                        static_cast<int>(plaintext.size())) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_EncryptUpdate failed"));
  }

  int final_len = 0;
  if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &final_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_EncryptFinal_ex failed"));
  }

  // Get tag
  std::vector<uint8_t> tag(kAesTagSize);
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, kAesTagSize, tag.data()) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_CTRL_GCM_GET_TAG failed"));
  }

  EVP_CIPHER_CTX_free(ctx);

  // Format: [nonce (12 bytes)] [ciphertext] [tag (16 bytes)]
  std::vector<uint8_t> result;
  result.reserve(kAesNonceSize + ciphertext.size() + kAesTagSize);
  result.insert(result.end(), nonce.begin(), nonce.end());
  result.insert(result.end(), ciphertext.begin(), ciphertext.end());
  result.insert(result.end(), tag.begin(), tag.end());

  return Result<std::vector<uint8_t>, CryptoError>::Ok(std::move(result));
}

Result<std::string, CryptoError> CryptoVaultImpl::Decrypt(
    const std::vector<uint8_t>& ciphertext,
    const std::vector<uint8_t>& key) {
  if (key.size() != kAesKeySize) {
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(0, "Invalid key size: expected " + std::to_string(kAesKeySize)));
  }

  if (ciphertext.size() < kAesNonceSize + kAesTagSize) {
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(0, "Ciphertext too short"));
  }

  // Extract nonce, ciphertext, tag
  const uint8_t* nonce = ciphertext.data();
  const uint8_t* encrypted = ciphertext.data() + kAesNonceSize;
  size_t encrypted_len = ciphertext.size() - kAesNonceSize - kAesTagSize;
  const uint8_t* tag = ciphertext.data() + ciphertext.size() - kAesTagSize;

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) {
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_CIPHER_CTX_new failed"));
  }

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), nonce) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_DecryptInit_ex failed"));
  }

  std::string plaintext(encrypted_len, '\0');
  int len = 0;
  if (EVP_DecryptUpdate(ctx, reinterpret_cast<uint8_t*>(&plaintext[0]), &len,
                        encrypted, static_cast<int>(encrypted_len)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_DecryptUpdate failed"));
  }

  // Set tag before finalizing
  if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, kAesTagSize,
                          const_cast<uint8_t*>(tag)) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "EVP_CTRL_GCM_SET_TAG failed"));
  }

  int final_len = 0;
  if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<uint8_t*>(&plaintext[0]) + len,
                          &final_len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return Result<std::string, CryptoError>::Err(
        MakeCryptoError(ERR_get_error(), "Decryption failed: invalid tag"));
  }

  EVP_CIPHER_CTX_free(ctx);
  plaintext.resize(len + final_len);

  return Result<std::string, CryptoError>::Ok(std::move(plaintext));
}

Result<void, CryptoError> CryptoVaultImpl::StoreSecret(
    const std::string& account,
    const std::vector<uint8_t>& secret) {
  // Derive a key from the account name for deterministic encryption.
  auto key_result = DeriveKey(account);
  if (key_result.IsErr()) {
    return Result<void, CryptoError>::Err(key_result.UnwrapErr());
  }
  auto key = std::move(key_result).Unwrap();

  // Encrypt the secret
  std::string plaintext(secret.begin(), secret.end());
  auto encrypt_result = Encrypt(plaintext, key);
  if (encrypt_result.IsErr()) {
    return Result<void, CryptoError>::Err(encrypt_result.UnwrapErr());
  }
  auto ciphertext = std::move(encrypt_result).Unwrap();

  // Store in-memory keyed by account hash
  std::lock_guard<std::mutex> lock(secrets_mutex_);
  secrets_[account] = std::move(ciphertext);

  VEOR_LOGI(LogCategory::kInfrastructure,
            "Secret stored for account: " + account);
  return Result<void, CryptoError>::Ok();
}

Result<std::vector<uint8_t>, CryptoError> CryptoVaultImpl::RetrieveSecret(
    const std::string& account) {
  std::lock_guard<std::mutex> lock(secrets_mutex_);

  auto it = secrets_.find(account);
  if (it == secrets_.end()) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(
        MakeCryptoError(0, "Secret not found for account: " + account));
  }

  auto key_result = DeriveKey(account);
  if (key_result.IsErr()) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(key_result.UnwrapErr());
  }
  auto key = std::move(key_result).Unwrap();

  auto decrypt_result = Decrypt(it->second, key);
  if (decrypt_result.IsErr()) {
    return Result<std::vector<uint8_t>, CryptoError>::Err(decrypt_result.UnwrapErr());
  }

  std::string plaintext = std::move(decrypt_result).Unwrap();
  std::vector<uint8_t> result(plaintext.begin(), plaintext.end());

  return Result<std::vector<uint8_t>, CryptoError>::Ok(std::move(result));
}

Result<void, CryptoError> CryptoVaultImpl::DeleteSecret(
    const std::string& account) {
  std::lock_guard<std::mutex> lock(secrets_mutex_);
  secrets_.erase(account);

  VEOR_LOGI(LogCategory::kInfrastructure,
            "Secret deleted for account: " + account);
  return Result<void, CryptoError>::Ok();
}

Result<std::vector<uint8_t>, CryptoError> CryptoVaultImpl::DeriveKey(
    const std::string& account) {
  // Simple key derivation: SHA-256(account + fixed_salt)
  // In production, use PBKDF2 or Argon2 via BoringSSL.
  const std::string salt = "veor_vault_salt_v1_2026";
  std::string input = account + salt;

  std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
  SHA256(reinterpret_cast<const uint8_t*>(input.data()), input.size(), hash.data());

  // AES-256-GCM requires 32-byte key
  hash.resize(kAesKeySize);
  return Result<std::vector<uint8_t>, CryptoError>::Ok(std::move(hash));
}

}  // namespace veor
