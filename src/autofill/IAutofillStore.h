// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
#pragma once

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "core/base/VeorResult.h"

namespace veor {

// ─────────────────────────────────────────────────────────────────────────────
// AutofillProfile
// ─────────────────────────────────────────────────────────────────────────────

struct AutofillProfile {
  int64_t id = 0;
  std::string name_full;
  std::string name_first;
  std::string name_last;
  std::string email;
  std::string phone;
  std::string address_line1;
  std::string address_line2;
  std::string city;
  std::string state;
  std::string postal_code;
  std::string country;
  base::Time created_at;
  base::Time last_used;
};

struct CreditCard {
  int64_t id = 0;
  std::string cardholder_name;
  std::string number_encrypted;
  std::string expiry_month;
  std::string expiry_year;
  std::string cvv_encrypted;
  base::Time created_at;
  base::Time last_used;
};

struct PasswordEntry {
  int64_t id = 0;
  std::string origin;
  std::string username;
  std::string password_encrypted;
  base::Time created_at;
  base::Time last_used;
};

// ─────────────────────────────────────────────────────────────────────────────
// IAutofillStore
// ─────────────────────────────────────────────────────────────────────────────

class IAutofillStore {
 public:
  virtual ~IAutofillStore() = default;

  // Profiles
  virtual Result<std::vector<AutofillProfile>, std::string> GetProfiles() = 0;
  virtual Result<void, std::string> SaveProfile(const AutofillProfile& profile) = 0;
  virtual Result<void, std::string> DeleteProfile(int64_t id) = 0;

  // Credit cards
  virtual Result<std::vector<CreditCard>, std::string> GetCreditCards() = 0;
  virtual Result<void, std::string> SaveCreditCard(const CreditCard& card) = 0;
  virtual Result<void, std::string> DeleteCreditCard(int64_t id) = 0;

  // Passwords
  virtual Result<std::vector<PasswordEntry>, std::string> GetPasswordsForOrigin(
      const std::string& origin) = 0;
  virtual Result<void, std::string> SavePassword(const PasswordEntry& entry) = 0;
  virtual Result<void, std::string> DeletePassword(int64_t id) = 0;
};

}  // namespace veor
