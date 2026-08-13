// Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "animation/AnimationEngine.h"

#include <algorithm>

#include "core/base/VeorId.h"

namespace veor {

AnimationEngine::AnimationEngine() = default;
AnimationEngine::~AnimationEngine() = default;

AnimationId AnimationEngine::Animate(AnimationProperty prop,
                                     std::vector<Keyframe> keyframes,
                                     int duration_ms,
                                     AnimationCallback callback) {
  auto id = IdGenerator::NextId<AnimationTag>();
  auto anim = std::make_unique<Animation>();
  anim->id = id;
  anim->property = prop;
  anim->keyframes = std::move(keyframes);
  anim->duration_ms = duration_ms;
  anim->on_update = std::move(callback);
  animations_.push_back(std::move(anim));
  return id;
}

void AnimationEngine::Cancel(AnimationId id) {
  animations_.erase(
      std::remove_if(animations_.begin(), animations_.end(),
                     [&](const auto& a) { return a->id == id; }),
      animations_.end());
}

void AnimationEngine::Tick(int64_t timestamp_ms) {
  for (auto& anim : animations_) {
    if (anim->completed) continue;
    float t = static_cast<float>(timestamp_ms % anim->duration_ms) / anim->duration_ms;
    float value = 0.0f;
    for (size_t i = 1; i < anim->keyframes.size(); ++i) {
      if (t <= anim->keyframes[i].time) {
        float seg = (t - anim->keyframes[i - 1].time) /
                    (anim->keyframes[i].time - anim->keyframes[i - 1].time);
        value = anim->keyframes[i - 1].value +
                seg * (anim->keyframes[i].value - anim->keyframes[i - 1].value);
        break;
      }
    }
    if (anim->on_update) anim->on_update(value);
  }
}

}  // namespace veor
