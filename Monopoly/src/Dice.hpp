#pragma once

#include <utility>

class Dice
{
private:
    int first = 1;
    int second = 1;
    bool has_rolled = false;

public:
    Dice() = default;
    ~Dice() = default;

    void roll();
    void reset() noexcept;

    int get_sum() const noexcept;
    bool is_double() const noexcept;
    bool has_rolled_once() const noexcept;
    std::pair<int, int> get_values() const noexcept;
};
