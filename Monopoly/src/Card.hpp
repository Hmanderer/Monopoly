#pragma once

#include <memory>
#include <string>
#include <vector>

class Player;
class Board;
class Game;
enum class FieldType;

struct CardContext
{
    std::vector<Player>* players = nullptr;
    Game* game = nullptr;
    Board* board = nullptr;
};

class Card
{
protected:
    std::string event_name;

public:
    explicit Card(std::string event_name = "Unknown");
    virtual ~Card() = default;

    const std::string& get_event_name() const noexcept { return event_name; }
    virtual void use(CardContext& context, int current_player) = 0;
    virtual bool is_jail_free() const noexcept { return false; }
};

class MoveCard final : public Card
{
private:
    FieldType type;
    std::string name;
    int steps;
    bool go;

public:
    MoveCard(std::string event_name, FieldType type,
             std::string name = {}, bool go = true, int steps = 0);

    void use(CardContext& context, int current_player) override;
};

class MoneyCard final : public Card
{
private:
    int money;

public:
    MoneyCard(std::string event_name, int money);
    void use(CardContext& context, int current_player) override;
};

class JailFreeCard final : public Card
{
private:
    int val;

public:
    JailFreeCard(std::string event_name, int val = 1);
    void use(CardContext& context, int current_player) override;
    bool is_jail_free() const noexcept override { return true; }
};

class RepairCard final : public Card
{
private:
    int house_price;
    int hotel_price;

public:
    RepairCard(std::string event_name, int house_price, int hotel_price);
    void use(CardContext& context, int current_player) override;
};

class PlayersPayCard final : public Card
{
private:
    int money;

public:
    PlayersPayCard(std::string event_name, int money);
    void use(CardContext& context, int current_player) override;
};

class Deck
{
protected:
    std::vector<std::unique_ptr<Card>> cards;
    CardContext context;
    std::unique_ptr<Card> drawn_card;
    std::vector<std::unique_ptr<Card>> held_cards;

public:
    Deck() = default;
    explicit Deck(CardContext context);
    virtual ~Deck() = default;

    Deck(const Deck&) = delete;
    Deck& operator=(const Deck&) = delete;

    void set_context(CardContext context);
    virtual void setup() = 0;

    Card* draw_random_card();
    void return_drawn_card();
    void keep_drawn_card();
    bool return_held_jail_free();
    void use_random_card(int current_player);
};

class CommunityChestDeck final : public Deck
{
public:
    using Deck::Deck;
    void setup() override;
};

class ChanceDeck final : public Deck
{
public:
    using Deck::Deck;
    void setup() override;
};
