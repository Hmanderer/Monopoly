#include "Fields.hpp"

#include "Board.hpp"
#include "Dice.hpp"
#include "Game.hpp"
#include "Player.hpp"
#include "Card.hpp"

#include <cstdlib>
#include <utility>

Field::Field() = default;

Field::Field(std::string field_name, FieldType field_type)
    : name(std::move(field_name)), type(field_type)
{
}

const std::string& Field::get_name() const noexcept
{
    return name;
}

FieldType Field::get_type() const noexcept
{
    return type;
}

StartField::StartField()
    : Field("START", FieldType::Start)
{
}

void StartField::play(Player*, std::vector<Player>&, Board&, Dice&, GameState& state)
{
    state = GameState::PlayingField;
}

PrisonField::PrisonField()
    : Field("Jail / Just Visiting", FieldType::Prison)
{
}

void PrisonField::play(Player*, std::vector<Player>&, Board&, Dice&, GameState& state)
{
    state = GameState::PlayingField;
}

TaxField::TaxField(std::string field_name, int tax_value)
    : Field(std::move(field_name), FieldType::Tax), tax(tax_value)
{
}

void TaxField::play(Player* current, std::vector<Player>&, Board&, Dice&, GameState& state)
{
    if (current) current->remove_money(tax, "Tax");
    state = GameState::PlayingField;
}

GoToPrisonField::GoToPrisonField()
    : Field("Go To Jail", FieldType::GoToPrison)
{
}

void GoToPrisonField::play(Player* current, std::vector<Player>&, Board& board, Dice&, GameState& state)
{
    if (!current) return;

    const int jail = board.find(FieldType::Prison, current->get_location());
    // Сначала показываем уведомление. Фактическое перемещение в тюрьму
    // выполняется после нажатия OK в Game.
    current->set_prison_score(3);
    state = GameState::JailNotification;
}

FreeParkingField::FreeParkingField()
    : Field("Free Parking", FieldType::FreeParking)
{
}

void FreeParkingField::play(Player*, std::vector<Player>&, Board&, Dice&, GameState& state)
{
    state = GameState::PlayingField;
}

CardField::CardField(std::string field_name, FieldType field_type, CardContext& value, Deck* value_deck)
    : Field(std::move(field_name), field_type), deck(value_deck), context(value)
{
    if (deck) {
        deck->set_context(context);
    }
}

void CardField::finish_current()
{
    if (!current || !deck) {
        current = nullptr;
        return;
    }

    if (current->is_jail_free()) {
        deck->keep_drawn_card();
    } else {
        deck->return_drawn_card();
    }
    current = nullptr;
}

void CardField::change_current()
{
    if (current) finish_current();
    current = deck ? deck->draw_random_card() : nullptr;
}

void CardField::use_current(int current_player)
{
    if (current && context.players) {
        current->use(context, current_player);
    }
}

const Card* CardField::get_current() const noexcept
{
    return current;
}

Card* CardField::get_current() noexcept
{
    return current;
}

void CardField::set_context(CardContext value)
{
    context = value;
    if (deck) deck->set_context(context);
}

void CardField::play(Player*, std::vector<Player>& players, Board& board, Dice&, GameState& state)
{
    context.players = &players;
    context.board = &board;
    change_current();
    state = current ? GameState::UsingCardUI : GameState::PlayingField;
}

CommunityChestField::CommunityChestField(CardContext context)
    : CardField("Community Chest", FieldType::CommunityChest, context, nullptr)
{
}

ChanceField::ChanceField(CardContext context)
    : CardField("Chance", FieldType::Chance, context, nullptr)
{
}

PropertyField::PropertyField()
    : Field("", FieldType::Unknown)
{
}

PropertyField::PropertyField(std::string field_name, FieldType field_type, int field_price)
    : Field(std::move(field_name), field_type), price(field_price)
{
}

int PropertyField::get_price() const noexcept
{
    return price;
}

void PropertyField::set_owner(Player* player) noexcept
{
    owner = player;
}

Player* PropertyField::get_owner() const noexcept
{
    return owner;
}

bool PropertyField::is_mortgaged() const noexcept
{
    return mortgaged;
}

void PropertyField::set_mortgaged(bool value) noexcept
{
    mortgaged = value;
}

int PropertyField::get_mortgage_value() const noexcept
{
    return price / 2;
}

bool PropertyField::mortgage(Player* player)
{
    if (!player || owner != player || mortgaged) return false;
    mortgaged = true;
    player->add_money(get_mortgage_value(), "Mortgage received");
    return true;
}

bool PropertyField::unmortgage(Player* player)
{
    if (!player || owner != player || !mortgaged) return false;
    const int cost = get_mortgage_value() + (get_mortgage_value() * 10 + 99) / 100;
    if (!player->can_pay(cost)) return false;
    if (!player->remove_money(cost, "Mortgage redeemed")) return false;
    mortgaged = false;
    return true;
}

void PropertyField::reset() noexcept
{
    owner = nullptr;
    mortgaged = false;
}

int PropertyField::get_sale_value() const noexcept
{
    return price / 2;
}

void PropertyField::buy(Player* current)
{
    if (!current || owner || mortgaged || !current->can_pay(price)) return;
    if (!current->remove_money(price, "Property purchased")) return;
    current->add_property(this);
}

void PropertyField::buy_menu(Player*)
{
    // UI управляется Game. Метод оставлен как расширяемая точка для меню покупки.
}

void PropertyField::play(Player* current, std::vector<Player>&, Board& board, Dice& dice, GameState& state)
{
    if (!current || current->get_is_bankrupt()) return;

    if (!owner) {
        state = GameState::BuyingProperty;
        return;
    }

    if (owner == current || mortgaged) {
        state = GameState::PlayingField;
        return;
    }

    int rent_value = get_rent(dice);
    if (auto* street = dynamic_cast<StreetField*>(this)) {
        rent_value = street->get_rent(dice, board);
    }

    current->pay_rent(owner, rent_value);
    state = GameState::PlayingField;
}

int StreetField::get_house_price_for_group(ColorGroup value) noexcept
{
    switch (value) {
    case ColorGroup::Brown:
    case ColorGroup::LightBlue:
        return 50;
    case ColorGroup::Pink:
    case ColorGroup::Orange:
        return 100;
    case ColorGroup::Red:
    case ColorGroup::Yellow:
        return 150;
    case ColorGroup::Green:
    case ColorGroup::DarkBlue:
        return 200;
    }
    return 0;
}

StreetField::StreetField(std::string field_name, int field_price, StreetRentData rent_data, ColorGroup color_group)
    : PropertyField(std::move(field_name), FieldType::Street, field_price),
      group(color_group), rent(rent_data), house_price(get_house_price_for_group(color_group))
{
}

void StreetField::reset() noexcept
{
    PropertyField::reset();
    houses = 0;
    hotel = false;
}

int StreetField::get_house_val() const noexcept
{
    return houses;
}

int StreetField::get_house_sale_value() const noexcept
{
    return house_price / 2;
}

int StreetField::get_sale_value() const noexcept
{
    const int building_value = hotel ? 5 * house_price : houses * house_price;
    return price / 2 + building_value / 2;
}

int StreetField::get_house_price() const noexcept
{
    return house_price;
}

ColorGroup StreetField::get_group() const noexcept
{
    return group;
}

int StreetField::get_rent(Dice&)
{
    if (hotel) return rent.hotel;

    switch (houses) {
    case 0: return rent.base;
    case 1: return rent.oneHouse;
    case 2: return rent.twoHouses;
    case 3: return rent.threeHouses;
    case 4: return rent.fourHouses;
    default: return rent.base;
    }
}

int StreetField::get_rent(Dice& dice, const Board& board) const
{
    int value = const_cast<StreetField*>(this)->get_rent(dice);

    // A complete color group doubles the base rent, but only when there
    // are no houses or hotel on the property.
    if (houses == 0 && !hotel && owner && owner->owns_full_group(group, board)) {
        value *= 2;
    }

    return value;
}

void StreetField::set_hotel(bool value) noexcept
{
    hotel = value;
    if (hotel) houses = 0;
}

void StreetField::add_house(int value) noexcept
{
    if (value <= 0 || hotel) return;
    houses += value;
    if (houses > 4) houses = 4;
}

void StreetField::build_house() noexcept
{
    if (hotel) return;

    if (houses < 4) {
        ++houses;
    }
    else {
        houses = 0;
        hotel = true;
    }
}

void StreetField::sell_house() noexcept
{
    if (houses > 0) --houses;
}

void StreetField::sell_hotel() noexcept
{
    if (hotel) {
        hotel = false;
        houses = 0;
    }
}

void StreetField::remove_house(int value) noexcept
{
    if (value <= 0 || hotel) return;
    houses -= value;
    if (houses < 0) houses = 0;
}

void StreetField::buy(Player* current)
{
    PropertyField::buy(current);
}

void StreetField::buy_menu(Player* current)
{
    PropertyField::buy_menu(current);
}

bool StreetField::has_hotel() const noexcept
{
    return hotel;
}

RailroadField::RailroadField(std::string field_name)
    : PropertyField(std::move(field_name), FieldType::Railroad, 200)
{
}

int RailroadField::get_rent(Dice&)
{
    if (!owner) return 0;

    // В классической Monopoly аренда: 25 / 50 / 100 / 200
    // в зависимости от количества железных дорог у владельца.
    // count_property использует тип поля и не требует доступа к Board.
    const int count = owner->count_property(FieldType::Railroad);
    switch (count) {
    case 1: return 25;
    case 2: return 50;
    case 3: return 100;
    case 4: return 200;
    default: return 0;
    }
}

UtilityField::UtilityField(std::string field_name)
    : PropertyField(std::move(field_name), FieldType::Utility, 150)
{
}

int UtilityField::get_rent(Dice& dice)
{
    if (!owner) return 0;

    const int count = owner->count_property(FieldType::Utility);
    const int multiplier = (count >= 2) ? 10 : 4;
    return dice.get_sum() * multiplier;
}
