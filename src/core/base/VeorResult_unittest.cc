// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "core/base/VeorResult.h"

namespace veor {

TEST(VeorResult, OkUnwrap) {
  auto r = Result<int, std::string>::Ok(42);
  EXPECT_TRUE(r.IsOk());
  EXPECT_FALSE(r.IsErr());
  EXPECT_EQ(r.Unwrap(), 42);
}

TEST(VeorResult, ErrPropagation) {
  auto r = Result<int, std::string>::Err("fail");
  EXPECT_TRUE(r.IsErr());
  EXPECT_FALSE(r.IsOk());
  EXPECT_EQ(r.UnwrapErr(), "fail");
}

TEST(VeorResult, BoolConversion) {
  auto ok = Result<int, std::string>::Ok(1);
  auto err = Result<int, std::string>::Err("fail");
  EXPECT_TRUE(static_cast<bool>(ok));
  EXPECT_FALSE(static_cast<bool>(err));
}

TEST(VeorResult, MapTransformsValue) {
  auto r = Result<int, std::string>::Ok(21).Map([](int x) { return x * 2; });
  EXPECT_TRUE(r.IsOk());
  EXPECT_EQ(r.Unwrap(), 42);
}

TEST(VeorResult, MapSkipsOnErr) {
  bool called = false;
  auto r = Result<int, std::string>::Err("fail").Map([&](int x) {
    called = true;
    return x * 2;
  });
  EXPECT_TRUE(r.IsErr());
  EXPECT_FALSE(called);
}

TEST(VeorResult, MapErrTransformsError) {
  auto r = Result<int, std::string>::Err("fail").MapErr([](const std::string& e) {
    return e + "!";
  });
  EXPECT_TRUE(r.IsErr());
  EXPECT_EQ(r.UnwrapErr(), "fail!");
}

TEST(VeorResult, MapErrSkipsOnOk) {
  bool called = false;
  auto r = Result<int, std::string>::Ok(42).MapErr([&](const std::string& e) {
    called = true;
    return e + "!";
  });
  EXPECT_TRUE(r.IsOk());
  EXPECT_FALSE(called);
  EXPECT_EQ(r.Unwrap(), 42);
}

TEST(VeorResult, AndThenChains) {
  auto r = Result<int, std::string>::Ok(21).AndThen([](int x) {
    return Result<float, std::string>::Ok(static_cast<float>(x) * 2.0f);
  });
  EXPECT_TRUE(r.IsOk());
  EXPECT_FLOAT_EQ(r.Unwrap(), 42.0f);
}

TEST(VeorResult, AndThenShortCircuitsOnErr) {
  bool called = false;
  auto r = Result<int, std::string>::Err("fail").AndThen([&](int x) {
    called = true;
    return Result<float, std::string>::Ok(0.0f);
  });
  EXPECT_TRUE(r.IsErr());
  EXPECT_FALSE(called);
}

TEST(VeorResult, OrElseProvidesAlternative) {
  auto r = Result<int, std::string>::Err("fail").OrElse([](const std::string&) {
    return Result<int, std::string>::Ok(99);
  });
  EXPECT_TRUE(r.IsOk());
  EXPECT_EQ(r.Unwrap(), 99);
}

TEST(VeorResult, OrElseSkipsOnOk) {
  bool called = false;
  auto r = Result<int, std::string>::Ok(42).OrElse([&](const std::string&) {
    called = true;
    return Result<int, std::string>::Ok(99);
  });
  EXPECT_TRUE(r.IsOk());
  EXPECT_FALSE(called);
  EXPECT_EQ(r.Unwrap(), 42);
}

TEST(VeorResult, UnwrapOr) {
  auto ok = Result<int, std::string>::Ok(42);
  auto err = Result<int, std::string>::Err("fail");
  EXPECT_EQ(ok.UnwrapOr(0), 42);
  EXPECT_EQ(err.UnwrapOr(0), 0);
}

TEST(VeorResult, UnwrapOrElse) {
  auto ok = Result<int, std::string>::Ok(42);
  auto err = Result<int, std::string>::Err("fail");
  EXPECT_EQ(ok.UnwrapOrElse([]() { return 0; }), 42);
  EXPECT_EQ(err.UnwrapOrElse([]() { return 99; }), 99);
}

TEST(VeorResult, OkOrNull) {
  auto ok = Result<int, std::string>::Ok(42);
  auto err = Result<int, std::string>::Err("fail");
  EXPECT_NE(ok.OkOrNull(), nullptr);
  EXPECT_EQ(*ok.OkOrNull(), 42);
  EXPECT_EQ(err.OkOrNull(), nullptr);
}

TEST(VeorResult, ErrOrNull) {
  auto ok = Result<int, std::string>::Ok(42);
  auto err = Result<int, std::string>::Err("fail");
  EXPECT_EQ(ok.ErrOrNull(), nullptr);
  EXPECT_NE(err.ErrOrNull(), nullptr);
  EXPECT_EQ(*err.ErrOrNull(), "fail");
}

TEST(VeorResult, VoidResultOk) {
  auto r = Result<void, std::string>::Ok();
  EXPECT_TRUE(r.IsOk());
  EXPECT_FALSE(r.IsErr());
}

TEST(VeorResult, VoidResultErr) {
  auto r = Result<void, std::string>::Err("fail");
  EXPECT_TRUE(r.IsErr());
  EXPECT_FALSE(r.IsOk());
  EXPECT_EQ(r.UnwrapErr(), "fail");
}

TEST(VeorResult, ChainedOperations) {
  auto result = Result<int, std::string>::Ok(10)
      .Map([](int x) { return x * 3; })
      .AndThen([](int x) {
        if (x > 20) return Result<int, std::string>::Ok(x + 2);
        return Result<int, std::string>::Err("too small");
      })
      .Map([](int x) { return x * 2; });

  EXPECT_TRUE(result.IsOk());
  EXPECT_EQ(result.Unwrap(), 64);  // (10*3+2)*2 = 64
}

}  // namespace veor
