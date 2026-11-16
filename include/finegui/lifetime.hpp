#pragma once

/**
 * @file lifetime.hpp
 * @brief What a GUI shows and loads lasts as long as a lifetime the program names
 *
 * A program with distinct stretches (a title screen, then a session; an editor, then a project)
 * gives each a Lifetime and makes it current while it builds what belongs there. Every window,
 * script GUI, named texture, loaded image, tween, hotkey and carried drag-and-drop item made
 * meanwhile belongs to that lifetime, and endLifetime() takes all of them away at once, with
 * what ImGui remembers about the windows they drew: nothing of one stretch is left to point
 * into the next.
 *
 * Whatever is made while FOREVER is current (the default) lasts as long as what holds it.
 */

#include <cstdint>

namespace finegui {

using Lifetime = uint32_t;

/// The lifetime of anything made before a program chooses one
inline constexpr Lifetime FOREVER = 0;

/// The lifetime what's shown or loaded now goes in
[[nodiscard]] Lifetime currentLifetime();

/// Make `lifetime` current: what's shown or loaded from now on belongs to it
void setCurrentLifetime(Lifetime lifetime);

/// Make a lifetime current for a scope, and put the one before back after it
class LifetimeScope {
public:
    explicit LifetimeScope(Lifetime lifetime) : previous_(currentLifetime()) {
        setCurrentLifetime(lifetime);
    }
    ~LifetimeScope() { setCurrentLifetime(previous_); }
    LifetimeScope(const LifetimeScope&) = delete;
    LifetimeScope& operator=(const LifetimeScope&) = delete;

private:
    Lifetime previous_;
};

/// End a lifetime: everything any holder keeps under it goes. Call between frames, once the GPU
/// has finished the frames that drew it (images and their descriptor sets are freed). A tree
/// still being drawn goes when the frame ends; anything its callbacks point into must last
/// that long.
void endLifetime(Lifetime lifetime);

/// Something that keeps things by lifetime. It is on the list endLifetime() goes through for as
/// long as it exists. GUI thread only, like the rest of finegui.
class LifetimeHolder {
public:
    LifetimeHolder();
    virtual ~LifetimeHolder();
    LifetimeHolder(const LifetimeHolder&) = delete;
    LifetimeHolder& operator=(const LifetimeHolder&) = delete;

    /// Let go of everything kept under `lifetime`
    virtual void endLifetime(Lifetime lifetime) = 0;
};

}  // namespace finegui
