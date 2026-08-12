#include "TurnManager.hpp"

#include "Game.hpp"
#include "Player.hpp"

void TurnManager::setup(CardContext context)
{
    board.setup(context);
    dice = Dice{};
    current_player = 0;
    consecutive_doubles = 0;
    extra_roll_available = false;
}

void TurnManager::set_players(std::vector<Player>& players_ref) noexcept
{
    players = &players_ref;
    current_player = 0;
    consecutive_doubles = 0;
    extra_roll_available = false;
    dice.reset();
}

Player* TurnManager::get_current_player() noexcept
{
    if (!players || players->empty() || current_player >= players->size()) return nullptr;
    return &(*players)[current_player];
}

const Player* TurnManager::get_current_player() const noexcept
{
    if (!players || players->empty() || current_player >= players->size()) return nullptr;
    return &(*players)[current_player];
}

std::size_t TurnManager::get_current_player_index() const noexcept
{
    return current_player;
}

Board* TurnManager::get_board() noexcept
{
    return &board;
}

const Board* TurnManager::get_board() const noexcept
{
    return &board;
}

Dice& TurnManager::get_dice() noexcept
{
    return dice;
}

const Dice& TurnManager::get_dice() const noexcept
{
    return dice;
}

int TurnManager::get_consecutive_doubles() const noexcept
{
    return consecutive_doubles;
}

bool TurnManager::can_roll_again() const noexcept
{
    return extra_roll_available;
}

std::size_t TurnManager::get_active_players_count() const
{
    if (!players) return 0;

    std::size_t count = 0;
    for (const Player& player : *players) {
        if (!player.get_is_bankrupt()) ++count;
    }
    return count;
}

void TurnManager::turn(Player* player, Dice& dice_ref, GameState& state)
{
    if (!player || !players || player->get_is_bankrupt()) return;

    Field* field = board.get_field_on(player->get_location());
    if (field) {
        field->play(player, *players, board, dice_ref, state);
    }
}

void TurnManager::set_current_player_index(std::size_t index) noexcept
{
    if (!players || players->empty()) return;
    current_player = index % players->size();
    consecutive_doubles = 0;
    extra_roll_available = false;
    dice.reset();
}

void TurnManager::change_current_player()
{
    if (!players || players->empty()) return;
    if (get_active_players_count() <= 1) return;

    const std::size_t count = players->size();
    for (std::size_t i = 0; i < count; ++i) {
        current_player = (current_player + 1) % count;
        if (!(*players)[current_player].get_is_bankrupt()) {
            consecutive_doubles = 0;
            extra_roll_available = false;
            dice.reset();
            return;
        }
    }
}

void TurnManager::update(float, GameState& state)
{
    if (!players || players->empty()) return;

    if (get_active_players_count() <= 1) {
        state = GameState::GameOver;
        extra_roll_available = false;
        return;
    }

    Player* player = get_current_player();
    if (!player || player->get_is_bankrupt()) {
        change_current_player();
        return;
    }

    switch (state) {
    case GameState::RollingDice:
        dice.roll();

        if (player->is_in_prison()) {
            // Бросок из тюрьмы — отдельное правило.
            // Даже если выпал дубль, он не считается дублем для
            // правила "три дубля подряд" и не даёт дополнительного хода.
            extra_roll_available = false;
            state = GameState::MovingPlayer;
            break;
        }

        if (dice.is_double()) {
            ++consecutive_doubles;

            // Третий дубль подряд немедленно отправляет игрока в тюрьму.
            // Поле, на котором оказался бы игрок после третьего броска,
            // не разыгрывается.
            if (consecutive_doubles >= 3) {
                // Третий дубль запускает уведомление; перенос на клетку
                // тюрьмы выполняется после подтверждения пользователем.
                player->set_prison_score(3);
                consecutive_doubles = 0;
                extra_roll_available = false;
                state = GameState::JailNotification;
                break;
            }

            extra_roll_available = true;
        }
        else {
            consecutive_doubles = 0;
            extra_roll_available = false;
        }

        state = GameState::MovingPlayer;
        break;

    case GameState::MovingPlayer:
        if (player->is_in_prison()) {
            // Дубль при выходе из тюрьмы не считается обычным дублем
            // и не даёт дополнительного броска.
            if (dice.is_double()) {
                player->set_in_prison(false);
                player->move_steps(dice.get_sum());
                turn(player, dice, state);
            }
            else {
                const int remaining = player->get_prison_score() - 1;
                player->set_prison_score(remaining);

                if (remaining <= 0) {
                    // After three failed attempts the player must pay $50.
                    // If they cannot, open the normal liquidation flow first;
                    // movement continues only after the debt is settled.
                    if (player->remove_money(50, "Jail fee")) {
                        player->set_in_prison(false);
                        player->move_steps(dice.get_sum());
                        turn(player, dice, state);
                    } else {
                        player->set_pending_jail_release(true, dice.get_sum());
                    }
                }
            }

            extra_roll_available = false;
            consecutive_doubles = 0;

            if (state == GameState::MovingPlayer || state == GameState::JailMenu) {
                state = GameState::PlayingField;
            }
            break;
        }

        player->move_steps(dice.get_sum());
        turn(player, dice, state);

        // "Go To Jail", а также эффекты карт могут отправить игрока
        // в тюрьму. В таком случае дополнительный бросок теряется.
        if (player->is_in_prison()) {
            extra_roll_available = false;
            consecutive_doubles = 0;
        }

        if (state == GameState::MovingPlayer) {
            state = GameState::PlayingField;
        }
        break;

    case GameState::UsingCard: {
        const int old_location = player->get_location();
        Field* source_field = board.get_field_on(old_location);
        auto* card_field = dynamic_cast<CardField*>(source_field);

        if (!card_field || !card_field->get_current()) {
            state = GameState::PlayingField;
            break;
        }

        card_field->use_current(static_cast<int>(current_player));
        card_field->finish_current();

        if (player->get_is_bankrupt()) {
            extra_roll_available = false;
            state = GameState::GameOver;
            break;
        }

        const int new_location = player->get_location();
        if (new_location != old_location) {
            Field* destination = board.get_field_on(new_location);
            if (destination) {
                destination->play(player, *players, board, dice, state);
                if (state == GameState::UsingCardUI) {
                    break;
                }
            }
        }

        if (player->is_in_prison()) {
            extra_roll_available = false;
            consecutive_doubles = 0;
        }

        if (state == GameState::UsingCard) {
            state = GameState::PlayingField;
        }
        break;
    }
    case GameState::PlayingField:
    case GameState::WaitingRoll:
    case GameState::JailNotification:
    case GameState::JailMenu:
    case GameState::BuyingProperty:
    case GameState::ManagingHouses:
    case GameState::Setup:
    case GameState::FirstRoll:
    case GameState::GameOver:
        break;
    }
}
