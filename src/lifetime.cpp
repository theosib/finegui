#include "finegui/lifetime.hpp"

#include <algorithm>
#include <vector>

namespace finegui {

namespace {

Lifetime current = FOREVER;

std::vector<LifetimeHolder*>& holders() {
    // Never destroyed: holders in statics may leave the list as the program exits
    static auto* list = new std::vector<LifetimeHolder*>();
    return *list;
}

}  // namespace

Lifetime currentLifetime() {
    return current;
}

void setCurrentLifetime(Lifetime lifetime) {
    current = lifetime;
}

void endLifetime(Lifetime lifetime) {
    // A copy: ending one holder's lifetime can destroy others (a GUI that owned a manager)
    std::vector<LifetimeHolder*> now = holders();
    for (LifetimeHolder* holder : now) {
        auto& live = holders();
        if (std::find(live.begin(), live.end(), holder) == live.end()) continue;
        holder->endLifetime(lifetime);
    }
}

LifetimeHolder::LifetimeHolder() {
    holders().push_back(this);
}

LifetimeHolder::~LifetimeHolder() {
    auto& list = holders();
    list.erase(std::remove(list.begin(), list.end(), this), list.end());
}

}  // namespace finegui
