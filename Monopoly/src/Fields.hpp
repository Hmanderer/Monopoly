#pragma once

#include "Card.hpp"
#include <memory>
#include <string>
#include <vector>

class Player;
class Dice;
class Board;
enum class GameState;

enum class FieldType
{
    Unknown,
    Start,
    CommunityChest,
    Chance,
    Prison,
    Tax,
    GoToPrison,
    FreeParking,
    Street,
    Railroad,
    Utility
};

enum class ColorGroup
{
    Brown,
    LightBlue,
    Pink,
    Orange,
    Red,
    Yellow,
    Green,
    DarkBlue
};

struct StreetRentData
{
    int base = 0;
    int oneHouse = 0;
    int twoHouses = 0;
    int threeHouses = 0;
    int fourHouses = 0;
    int hotel = 0;
};

class Field
{
protected:
    std::string name;
    FieldType type = FieldType::Unknown;

public:
    Field();
    Field(std::string name, FieldType type);
    virtual ~Field() = default;

    const std::string& get_name() const noexcept;
    FieldType get_type() const noexcept;

    virtual void play(Player* current,
                      std::vector<Player>& players,
                      Board& board,
                      Dice& dice,
                      GameState& state) = 0;
};

class StartField final : public Field
{
public:
    StartField();
    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class PrisonField final : public Field
{
public:
    PrisonField();
    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class TaxField final : public Field
{
private:
    int tax = 0;

public:
    TaxField(std::string name, int tax);
    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class GoToPrisonField final : public Field
{
public:
    GoToPrisonField();
    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class FreeParkingField final : public Field
{
public:
    FreeParkingField();
    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class CardField : public Field
{
protected:
    Deck* deck = nullptr;
    Card* current = nullptr;
    CardContext context;
public:
    CardField(std::string name, FieldType type, CardContext& context, Deck* deck);
    ~CardField() override = default;

    void change_current();
    void use_current(int current_player);
    void finish_current();
    const Card* get_current() const noexcept;
    Card* get_current() noexcept;
    void set_context(CardContext value);

    void play(Player* current, std::vector<Player>& players, Board& board,
        Dice& dice, GameState& state) override;
};

class CommunityChestField final : public CardField
{
public:
    explicit CommunityChestField(CardContext context);
};

class ChanceField final : public CardField
{
public:
    explicit ChanceField(CardContext context);
};

class PropertyField : public Field
{
protected:
    int price = 0;
    Player* owner = nullptr;
    bool mortgaged = false;

public:
    PropertyField();
    PropertyField(std::string name, FieldType type, int price);
    ~PropertyField() override = default;

    int get_price() const noexcept;
    void set_owner(Player* player) noexcept;
    Player* get_owner() const noexcept;
    bool is_mortgaged() const noexcept;
    void set_mortgaged(bool value) noexcept;
    int get_mortgage_value() const noexcept;
    bool mortgage(Player* player);
    bool unmortgage(Player* player);

    virtual void reset() noexcept;
    virtual int get_sale_value() const noexcept;
    virtual int get_rent(Dice& dice) = 0;

    virtual void buy(Player* current);
    virtual void buy_menu(Player* current);

    void play(Player* current, std::vector<Player>& players, Board& board,
              Dice& dice, GameState& state) override;
};

class StreetField final : public PropertyField
{
private:
    int houses = 0;
    int house_price = 0;
    bool hotel = false;
    ColorGroup group = ColorGroup::Brown;
    StreetRentData rent{};

    static int get_house_price_for_group(ColorGroup group) noexcept;

public:
    StreetField(std::string name, int price, StreetRentData rent, ColorGroup group);

    void reset() noexcept override;
    int get_house_val() const noexcept;
    int get_house_sale_value() const noexcept;
    int get_sale_value() const noexcept override;
    int get_house_price() const noexcept;
    ColorGroup get_group() const noexcept;
    int get_rent(Dice& dice) override;
    int get_rent(Dice& dice, const Board& board) const;

    void set_hotel(bool value) noexcept;
    void add_house(int value) noexcept;
    void build_house() noexcept;
    void sell_house() noexcept;
    void sell_hotel() noexcept;
    void remove_house(int value) noexcept;
    void buy(Player* current) override;
    void buy_menu(Player* current) override;
    bool has_hotel() const noexcept;
};

class RailroadField final : public PropertyField
{
public:
    explicit RailroadField(std::string name);
    int get_rent(Dice& dice) override;
};

class UtilityField final : public PropertyField
{
public:
    explicit UtilityField(std::string name);
    int get_rent(Dice& dice) override;
};
