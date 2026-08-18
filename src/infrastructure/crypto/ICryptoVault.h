// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>
#include <string_view>

#include "core/base/VeorResult.h"

namespace veor {

class ICryptoVault {
 public:
  virtual ~ICryptoVault() = default;

  virtual Result<std::vector<uint8_t>, CryptoError> GenerateRandom(size_t length) = 0;
  virtual Result<std::vector<uint8_t>, CryptoError> HashSha256(std::string_view data) = 0;
  virtual Result<std::vector<uint8_t>, CryptoError> Encrypt(std::string_view plaintext, const std::vector<uint8_t>& key) = 0;
  virtual Result<std::string, CryptoError> Decrypt(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key) = 0;
  virtual Result<void, CryptoError> StoreSecret(const std::string& account, const std::vector<uint8_t>& secret) = 0;
  virtual Result<std::vector<uint8_t>, CryptoError> RetrieveSecret(const std::string& account) = 0;
  virtual Result<void, CryptoError> DeleteSecret(const std::string& account) = 0;
};

}  // namespace veor
