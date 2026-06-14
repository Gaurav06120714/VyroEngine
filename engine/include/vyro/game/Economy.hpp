// VyroEngine — Currency / economy (V9.1)
// Tracks the player's credit balance: kills earn credits, the between-wave shop
// spends them. A tiny guarded wallet — headless and tested.
#pragma once

#include "vyro/core/Types.hpp"

namespace vyro::game {

class Economy
{
public:
    void earn(int amount)
    {
        if (amount > 0) {
            m_credits += amount;
        }
    }

    [[nodiscard]] bool can_afford(int cost) const { return cost >= 0 && m_credits >= cost; }

    // Spend if affordable; returns true on success.
    bool spend(int cost)
    {
        if (!can_afford(cost)) {
            return false;
        }
        m_credits -= cost;
        return true;
    }

    [[nodiscard]] int credits() const { return m_credits; }
    void reset() { m_credits = 0; }

private:
    int m_credits = 0;
};

} // namespace vyro::game
