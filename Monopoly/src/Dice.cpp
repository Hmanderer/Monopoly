#include "Dice.hpp"

#include <cstdlib>

void Dice::roll()
{
    first = 1 + std::rand() % 6;
    second = 1 + std::rand() % 6;
    has_rolled = true;
}

void Dice::reset() noexcept
{
    first = 1;
    second = 1;
    has_rolled = false;
}

int Dice::get_sum() const noexcept
{
    return first + second;
}

bool Dice::is_double() const noexcept
{
    return has_rolled && first == second;
}

bool Dice::has_rolled_once() const noexcept
{
    return has_rolled;
}

std::pair<int, int> Dice::get_values() const noexcept
{
    return { first, second };
}
