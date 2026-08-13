// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/base/VeorResult.h"

// Explicit instantiations for common Result types used across the codebase.
// This reduces compile times by avoiding repeated template instantiation in
// every translation unit.

namespace veor {

// Result<void, StringError> — for operations that return nothing or an error message.
template class Result<void, StringError>;

// Result<int, StringError>
template class Result<int, StringError>;

// Result<std::string, StringError>
template class Result<std::string, StringError>;

// Result<void, IOError>
template class Result<void, IOError>;

// Result<void, StorageError>
template class Result<void, StorageError>;

// Result<void, NetworkError>
template class Result<void, NetworkError>;

// Result<void, CryptoError>
template class Result<void, CryptoError>;

}  // namespace veor
