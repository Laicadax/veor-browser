// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "base/strings/string_piece.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// CryptoError
// ─────────────────────────────────────────────────────────────────────────────

struct CryptoError {
  int code = 0;
  std::string message;

  std::string ToString() const {
    return "CRYPTO[" + std::to_string(code) + "]: " + message;
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// ICryptoVault
// ─────────────────────────────────────────────────────────────────────────────
// Cryptographic operations and OS secure storage.
//
// Thread safety:
//   - In-memory ops (GenerateRandom, Hash, Encrypt, Decrypt): [Any Thread]
//   - OS keychain ops (StoreSecret, RetrieveSecret, DeleteSecret): [IO Thread]

class ICryptoVault {
 public:
  virtual ~ICryptoVault() = default;

  // Generates a cryptographically secure random buffer.
  // [Any Thread]
  virtual Result<std::vector<uint8_t>, CryptoError> GenerateRandom(size_t length) = 0;

  // Hashes data using SHA-256.
  // [Any Thread]
  virtual Result<std::vector<uint8_t>, CryptoError> HashSha256(
      base::StringPiece data) = 0;

  // Encrypts data using AES-256-GCM.
  // Key must be 32 bytes (256 bits).
  // Returns ciphertext with 12-byte nonce prepended.
  // [Any Thread]
  virtual Result<std::vector<uint8_t>, CryptoError> Encrypt(
      base::StringPiece plaintext,
      const std::vector<uint8_t>& key) = 0;

  // Decrypts AES-256-GCM encrypted data.
  // Expects 12-byte nonce prepended to ciphertext.
  // Key must be 32 bytes (256 bits).
  // [Any Thread]
  virtual Result<std::string, CryptoError> Decrypt(
      const std::vector<uint8_t>& ciphertext,
      const std::vector<uint8_t>& key) = 0;

  // ── OS Secure Storage ──

  // Stores a secret in the OS keychain/credential manager.
  // [IO Thread]
  virtual Result<void, CryptoError> StoreSecret(
      const std::string& account,
      const std::vector<uint8_t>& secret) = 0;

  // Retrieves a secret from OS secure storage.
  // [IO Thread]
  virtual Result<std::vector<uint8_t>, CryptoError> RetrieveSecret(
      const std::string& account) = 0;

  // Deletes a secret from OS secure storage.
  // [IO Thread]
  virtual Result<void, CryptoError> DeleteSecret(const std::string& account) = 0;
};

}  // namespace veor
