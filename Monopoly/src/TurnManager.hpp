#pragma once

#include "Board.hpp"
#include "Dice.hpp"

#include <cstddef>
#include <vector>

class Player;
class Game;
struct CardContext;
enum class GameState;

class TurnManager
{
private:
    Dice dice;
    Board board;
    std::vector<Player>* players = nullptr;
    std::size_t current_player = 0;

    // Количество дублей подряд в рамках текущего хода.
    // Дубль при попытке выйти из тюрьмы сюда НЕ засчитывается.
    int consecutive_doubles = 0;
    bool extra_roll_available = false;

    std::size_t get_active_players_count() const;

public:
    TurnManager() = default;
    ~TurnManager() = default;

    TurnManager(const TurnManager&) = delete;
    TurnManager& operator=(const TurnManager&) = delete;

    void setup(CardContext context);
    void set_players(std::vector<Player>& players_ref) noexcept;

    Player* get_current_player() noexcept;
    const Player* get_current_player() const noexcept;
    std::size_t get_current_player_index() const noexcept;
    Board* get_board() noexcept;
    const Board* get_board() const noexcept;
    Dice& get_dice() noexcept;
    const Dice& get_dice() const noexcept;
    int get_consecutive_doubles() const noexcept;
    bool can_roll_again() const noexcept;

    void turn(Player* player, Dice& dice, GameState& state);
    void change_current_player();
    void set_current_player_index(std::size_t index) noexcept;
    void update(float delta_time, GameState& state);
};
