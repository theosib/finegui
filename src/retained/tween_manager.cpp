#include <finegui/tween_manager.hpp>
#include <finegui/gui_renderer.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace finegui {

TweenManager::TweenManager(GuiRenderer& renderer)
    : renderer_(renderer) {}

WidgetNode* TweenManager::resolve(int guiId, const std::vector<int>& childPath) {
    WidgetNode* node = renderer_.get(guiId);
    if (!node) return nullptr;
    for (int idx : childPath) {
        if (idx < 0 || idx >= static_cast<int>(node->children.size()))
            return nullptr;
        node = &node->children[idx];
    }
    return node;
}

float TweenManager::readProperty(const WidgetNode& node, TweenProperty prop) {
    switch (prop) {
        case TweenProperty::Alpha:      return node.alpha;
        case TweenProperty::PosX:       return node.windowPosX;
        case TweenProperty::PosY:       return node.windowPosY;
        case TweenProperty::FloatValue: return node.floatValue;
        case TweenProperty::IntValue:   return static_cast<float>(node.intValue);
        case TweenProperty::ColorR:     return node.colorR;
        case TweenProperty::ColorG:     return node.colorG;
        case TweenProperty::ColorB:     return node.colorB;
        case TweenProperty::ColorA:     return node.colorA;
        case TweenProperty::Width:      return node.width;
        case TweenProperty::Height:     return node.height;
        case TweenProperty::ScaleX:     return node.scaleX;
        case TweenProperty::ScaleY:     return node.scaleY;
        case TweenProperty::RotationY:  return node.rotationY;
    }
    return 0.0f;
}

void TweenManager::writeProperty(WidgetNode& node, TweenProperty prop, float value) {
    switch (prop) {
        case TweenProperty::Alpha:      node.alpha = value; break;
        case TweenProperty::PosX:       node.windowPosX = value; break;
        case TweenProperty::PosY:       node.windowPosY = value; break;
        case TweenProperty::FloatValue: node.floatValue = value; break;
        case TweenProperty::IntValue:   node.intValue = static_cast<int>(value); break;
        case TweenProperty::ColorR:     node.colorR = value; break;
        case TweenProperty::ColorG:     node.colorG = value; break;
        case TweenProperty::ColorB:     node.colorB = value; break;
        case TweenProperty::ColorA:     node.colorA = value; break;
        case TweenProperty::Width:      node.width = value; break;
        case TweenProperty::Height:     node.height = value; break;
        case TweenProperty::ScaleX:     node.scaleX = value; break;
        case TweenProperty::ScaleY:     node.scaleY = value; break;
        case TweenProperty::RotationY:  node.rotationY = value; break;
    }
}

float TweenManager::applyEasing(float t, Easing easing) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    switch (easing) {
        case Easing::Linear:
            return t;
        case Easing::EaseIn:
            return t * t;
        case Easing::EaseOut:
            return t * (2.0f - t);
        case Easing::EaseInOut:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case Easing::CubicOut: {
            float u = 1.0f - t;
            return 1.0f - u * u * u;
        }
        case Easing::ElasticOut: {
            float p = 0.3f;
            return std::pow(2.0f, -10.0f * t) *
                   std::sin((t - p / 4.0f) * (2.0f * static_cast<float>(M_PI)) / p) + 1.0f;
        }
        case Easing::BounceOut: {
            if (t < 1.0f / 2.75f) {
                return 7.5625f * t * t;
            } else if (t < 2.0f / 2.75f) {
                t -= 1.5f / 2.75f;
                return 7.5625f * t * t + 0.75f;
            } else if (t < 2.5f / 2.75f) {
                t -= 2.25f / 2.75f;
                return 7.5625f * t * t + 0.9375f;
            } else {
                t -= 2.625f / 2.75f;
                return 7.5625f * t * t + 0.984375f;
            }
        }
    }
    return t;
}

void TweenManager::update(float dt) {
    // Process property tweens
    std::vector<std::function<void()>> completedCallbacks;

    for (auto it = tweens_.begin(); it != tweens_.end(); ) {
        auto& tw = *it;
        WidgetNode* node = resolve(tw.guiId, tw.childPath);
        if (!node) {
            // Target gone — silently remove
            it = tweens_.erase(it);
            continue;
        }

        // On first frame, read current value if auto-from
        if (!tw.started) {
            if (std::isnan(tw.fromValue)) {
                tw.fromValue = readProperty(*node, tw.property);
            }
            tw.started = true;
        }

        tw.elapsed += dt;
        float t = std::min(tw.elapsed / tw.duration, 1.0f);
        float eased = applyEasing(t, tw.easing);
        float value = tw.fromValue + (tw.toValue - tw.fromValue) * eased;
        writeProperty(*node, tw.property, value);

        if (t >= 1.0f) {
            if (tw.onComplete) {
                completedCallbacks.push_back(
                    [cb = tw.onComplete, id = tw.id]() { cb(id); });
            }
            it = tweens_.erase(it);
        } else {
            ++it;
        }
    }

    // Process shake tweens
    for (auto it = shakes_.begin(); it != shakes_.end(); ) {
        auto& sk = *it;
        WidgetNode* node = resolve(sk.guiId, {});
        if (!node) {
            it = shakes_.erase(it);
            continue;
        }

        if (!sk.started) {
            // Capture base position
            sk.basePosX = (node->windowPosX != FLT_MAX) ? node->windowPosX : 0.0f;
            sk.basePosY = (node->windowPosY != FLT_MAX) ? node->windowPosY : 0.0f;
            sk.started = true;
        }

        sk.elapsed += dt;
        float t = std::min(sk.elapsed / sk.duration, 1.0f);

        if (t >= 1.0f) {
            // Restore base position
            node->windowPosX = sk.basePosX;
            node->windowPosY = sk.basePosY;
            if (sk.onComplete) {
                completedCallbacks.push_back(
                    [cb = sk.onComplete, id = sk.id]() { cb(id); });
            }
            it = shakes_.erase(it);
        } else {
            // Damped sinusoidal offset
            float decay = std::exp(-3.0f * t);
            float offset = sk.amplitude * decay *
                           std::sin(2.0f * static_cast<float>(M_PI) * sk.frequency * sk.elapsed);
            node->windowPosX = sk.basePosX + offset;
            node->windowPosY = sk.basePosY + offset * 0.7f; // slightly different Y for natural feel
            ++it;
        }
    }

    // Fire callbacks after mutation is done (safe to start new tweens in callbacks)
    for (auto& cb : completedCallbacks) {
        cb();
    }
}

int TweenManager::animate(int guiId, std::vector<int> childPath,
                           TweenProperty prop, float toValue,
                           float duration, Easing easing,
                           TweenCallback onComplete) {
    int id = nextId_++;
    tweens_.push_back(Tween{
        id, guiId, std::move(childPath), prop,
        std::numeric_limits<float>::quiet_NaN(), // auto-from: read on first frame
        toValue, duration, 0.0f, easing,
        std::move(onComplete), false
    });
    return id;
}

int TweenManager::animate(int guiId, std::vector<int> childPath,
                           TweenProperty prop, float fromValue, float toValue,
                           float duration, Easing easing,
                           TweenCallback onComplete) {
    int id = nextId_++;
    tweens_.push_back(Tween{
        id, guiId, std::move(childPath), prop,
        fromValue, toValue, duration, 0.0f, easing,
        std::move(onComplete), false
    });
    return id;
}

int TweenManager::fadeIn(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    return animate(guiId, {}, TweenProperty::Alpha, 0.0f, 1.0f, duration, easing, std::move(onComplete));
}

int TweenManager::fadeOut(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    return animate(guiId, {}, TweenProperty::Alpha, 1.0f, 0.0f, duration, easing, std::move(onComplete));
}

int TweenManager::slideTo(int guiId, float x, float y, float duration, Easing easing,
                           TweenCallback onComplete) {
    // Two tweens for X and Y; callback on the last one
    animate(guiId, {}, TweenProperty::PosX, x, duration, easing);
    return animate(guiId, {}, TweenProperty::PosY, y, duration, easing, std::move(onComplete));
}

int TweenManager::colorTo(int guiId, std::vector<int> childPath,
                           float r, float g, float b, float a,
                           float duration, Easing easing, TweenCallback onComplete) {
    animate(guiId, childPath, TweenProperty::ColorR, r, duration, easing);
    animate(guiId, childPath, TweenProperty::ColorG, g, duration, easing);
    animate(guiId, childPath, TweenProperty::ColorB, b, duration, easing);
    return animate(guiId, std::move(childPath), TweenProperty::ColorA, a, duration, easing, std::move(onComplete));
}

int TweenManager::zoomIn(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    animate(guiId, {}, TweenProperty::ScaleX, 0.0f, 1.0f, duration, easing);
    return animate(guiId, {}, TweenProperty::ScaleY, 0.0f, 1.0f, duration, easing, std::move(onComplete));
}

int TweenManager::zoomOut(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    animate(guiId, {}, TweenProperty::ScaleX, 1.0f, 0.0f, duration, easing);
    return animate(guiId, {}, TweenProperty::ScaleY, 1.0f, 0.0f, duration, easing, std::move(onComplete));
}

int TweenManager::flipY(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    return animate(guiId, {}, TweenProperty::RotationY, 0.0f,
                   static_cast<float>(M_PI), duration, easing, std::move(onComplete));
}

int TweenManager::flipYBack(int guiId, float duration, Easing easing, TweenCallback onComplete) {
    return animate(guiId, {}, TweenProperty::RotationY, static_cast<float>(M_PI),
                   0.0f, duration, easing, std::move(onComplete));
}

int TweenManager::shake(int guiId, float duration, float amplitude, float frequency,
                         TweenCallback onComplete) {
    int id = nextId_++;
    shakes_.push_back(ShakeTween{
        id, guiId, duration, 0.0f, amplitude, frequency,
        0.0f, 0.0f, false, std::move(onComplete)
    });
    return id;
}

void TweenManager::cancel(int tweenId) {
    tweens_.erase(
        std::remove_if(tweens_.begin(), tweens_.end(),
                        [tweenId](const Tween& t) { return t.id == tweenId; }),
        tweens_.end());
    shakes_.erase(
        std::remove_if(shakes_.begin(), shakes_.end(),
                        [tweenId](const ShakeTween& s) { return s.id == tweenId; }),
        shakes_.end());
}

void TweenManager::cancelAll(int guiId) {
    tweens_.erase(
        std::remove_if(tweens_.begin(), tweens_.end(),
                        [guiId](const Tween& t) { return t.guiId == guiId; }),
        tweens_.end());
    shakes_.erase(
        std::remove_if(shakes_.begin(), shakes_.end(),
                        [guiId](const ShakeTween& s) { return s.guiId == guiId; }),
        shakes_.end());
}

void TweenManager::cancelAll() {
    tweens_.clear();
    shakes_.clear();
}

bool TweenManager::isActive(int tweenId) const {
    for (auto& t : tweens_) {
        if (t.id == tweenId) return true;
    }
    for (auto& s : shakes_) {
        if (s.id == tweenId) return true;
    }
    return false;
}

int TweenManager::activeCount() const {
    return static_cast<int>(tweens_.size() + shakes_.size());
}

} // namespace finegui
