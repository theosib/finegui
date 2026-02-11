#pragma once

#include "widget_node.hpp"
#include <vector>
#include <functional>

namespace finegui {

class GuiRenderer;

enum class Easing {
    Linear,
    EaseIn,       // quadratic
    EaseOut,      // quadratic
    EaseInOut,    // quadratic
    CubicOut,
    ElasticOut,
    BounceOut
};

enum class TweenProperty {
    Alpha, PosX, PosY,
    FloatValue, IntValue,
    ColorR, ColorG, ColorB, ColorA,
    Width, Height,
    ScaleX, ScaleY, RotationY
};

using TweenCallback = std::function<void(int tweenId)>;

class TweenManager {
public:
    explicit TweenManager(GuiRenderer& renderer);

    /// Advance all active tweens by dt seconds.
    void update(float dt);

    /// Animate property to target value (reads current value as "from" on first frame).
    int animate(int guiId, std::vector<int> childPath,
                TweenProperty prop, float toValue,
                float duration, Easing easing = Easing::EaseOut,
                TweenCallback onComplete = {});

    /// Animate property with explicit from and to values.
    int animate(int guiId, std::vector<int> childPath,
                TweenProperty prop, float fromValue, float toValue,
                float duration, Easing easing = Easing::EaseOut,
                TweenCallback onComplete = {});

    /// Fade window from alpha 0 to 1.
    int fadeIn(int guiId, float duration = 0.3f, Easing easing = Easing::EaseOut,
              TweenCallback onComplete = {});

    /// Fade window from alpha 1 to 0.
    int fadeOut(int guiId, float duration = 0.3f, Easing easing = Easing::EaseIn,
               TweenCallback onComplete = {});

    /// Slide window to position (x, y).
    int slideTo(int guiId, float x, float y, float duration = 0.4f,
                Easing easing = Easing::EaseOut, TweenCallback onComplete = {});

    /// Animate color of a child widget.
    int colorTo(int guiId, std::vector<int> childPath,
                float r, float g, float b, float a,
                float duration = 0.3f, Easing easing = Easing::EaseOut,
                TweenCallback onComplete = {});

    /// Zoom in from scale 0 to 1 (window appears from center point).
    int zoomIn(int guiId, float duration = 0.3f, Easing easing = Easing::EaseOut,
               TweenCallback onComplete = {});

    /// Zoom out from scale 1 to 0 (window collapses to center point).
    int zoomOut(int guiId, float duration = 0.3f, Easing easing = Easing::EaseIn,
                TweenCallback onComplete = {});

    /// Flip around the Y-axis from 0 to PI (shows back side).
    int flipY(int guiId, float duration = 0.5f, Easing easing = Easing::EaseInOut,
              TweenCallback onComplete = {});

    /// Flip around the Y-axis from PI back to 0 (shows front side).
    int flipYBack(int guiId, float duration = 0.5f, Easing easing = Easing::EaseInOut,
                  TweenCallback onComplete = {});

    /// Screen shake effect on a window.
    int shake(int guiId, float duration = 0.4f,
              float amplitude = 8.0f, float frequency = 15.0f,
              TweenCallback onComplete = {});

    /// Cancel a specific tween by ID.
    void cancel(int tweenId);

    /// Cancel all tweens targeting a specific guiId.
    void cancelAll(int guiId);

    /// Cancel all active tweens.
    void cancelAll();

    /// Check if a tween is still active.
    bool isActive(int tweenId) const;

    /// Number of active tweens.
    int activeCount() const;

private:
    struct Tween {
        int id;
        int guiId;
        std::vector<int> childPath;
        TweenProperty property;
        float fromValue;
        float toValue;
        float duration;
        float elapsed;
        Easing easing;
        TweenCallback onComplete;
        bool started;  // false until first frame (for auto-from)
    };

    struct ShakeTween {
        int id;
        int guiId;
        float duration;
        float elapsed;
        float amplitude;
        float frequency;
        float basePosX;
        float basePosY;
        bool started;
        TweenCallback onComplete;
    };

    GuiRenderer& renderer_;
    int nextId_ = 1;
    std::vector<Tween> tweens_;
    std::vector<ShakeTween> shakes_;

    WidgetNode* resolve(int guiId, const std::vector<int>& childPath);
    static float readProperty(const WidgetNode& node, TweenProperty prop);
    static void writeProperty(WidgetNode& node, TweenProperty prop, float value);
    static float applyEasing(float t, Easing easing);
};

} // namespace finegui
