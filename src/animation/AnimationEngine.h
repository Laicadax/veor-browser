// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "core/base/VeorId.h"

namespace veor {

enum class AnimationProperty { kOpacity, kTranslateX, kTranslateY, kScale, kDepth };

struct Keyframe {
  float time;
  float value;
};

using AnimationCallback = std::function<void(float current_value)>;

class Animation {
 public:
  AnimationId id;
  AnimationProperty property;
  std::vector<Keyframe> keyframes;
  int duration_ms;
  AnimationCallback on_update;
  bool completed = false;
};

class AnimationEngine {
 public:
  AnimationEngine();
  ~AnimationEngine();

  AnimationId Animate(AnimationProperty prop,
                      std::vector<Keyframe> keyframes,
                      int duration_ms,
                      AnimationCallback callback);
  void Cancel(AnimationId id);
  void Tick(int64_t timestamp_ms);

 private:
  std::vector<std::unique_ptr<Animation>> animations_;
};

}  // namespace veor
