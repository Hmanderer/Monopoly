#include "Card.hpp"

#include "Board.hpp"
#include "Fields.hpp"
#include "Player.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <utility>

Card::Card(std::string name) : event_name(std::move(name)) {}

MoveCard::MoveCard(std::string event_name, FieldType type,
                   std::string name, bool go, int steps)
    : Card(std::move(event_name)), type(type), name(std::move(name)), steps(steps), go(go)
{
}

void MoveCard::use(CardContext& context, int current_player)
{
    if (!context.players || !context.board || current_player < 0 ||
        static_cast<std::size_t>(current_player) >= context.players->size()) return;

    Player& player = context.players->at(static_cast<std::size_t>(current_player));

    if (type != FieldType::Unknown) {
        const int index = context.board->find(type, player.get_location(), name);
        if (index < 0) return;

        if (type == FieldType::GoToPrison) {
            player.set_location(index);
            player.set_in_prison(true);
            player.set_prison_score(3);
            return;
        }
        if (go) player.set_location_with_go(index);
        else player.set_location(index);
        return;
    }

    if (go) player.move_steps(steps);
    else player.set_location(player.get_location() + steps);
}

MoneyCard::MoneyCard(std::string event_name, int value)
    : Card(std::move(event_name)), money(value)
{
}

void MoneyCard::use(CardContext& context, int current_player)
{
    if (!context.players || current_player < 0 ||
        static_cast<std::size_t>(current_player) >= context.players->size()) return;

    Player& player = context.players->at(static_cast<std::size_t>(current_player));
    if (money >= 0) player.add_money(money, event_name);
    else player.remove_money(-money, event_name);
}

JailFreeCard::JailFreeCard(std::string event_name, int value)
    : Card(std::move(event_name)), val(value)
{
}

void JailFreeCard::use(CardContext& context, int current_player)
{
    if (!context.players || current_player < 0 ||
        static_cast<std::size_t>(current_player) >= context.players->size()) return;

    context.players->at(static_cast<std::size_t>(current_player))
        .set_get_out_from_jail_card(val);
}

RepairCard::RepairCard(std::string event_name, int house, int hotel)
    : Card(std::move(event_name)), house_price(house), hotel_price(hotel)
{
}

void RepairCard::use(CardContext& context, int current_player)
{
    if (!context.players || current_player < 0 ||
        static_cast<std::size_t>(current_player) >= context.players->size()) return;

    Player& player = context.players->at(static_cast<std::size_t>(current_player));
    int total = 0;

    for (PropertyField* property : player.get_properties()) {
        auto* street = dynamic_cast<StreetField*>(property);
        if (!street) continue;

        total += street->get_house_val() * house_price;
        if (street->has_hotel()) total += hotel_price;
    }

    player.remove_money(total, event_name);
}

PlayersPayCard::PlayersPayCard(std::string event_name, int value)
    : Card(std::move(event_name)), money(value)
{
}

void PlayersPayCard::use(CardContext& context, int current_player)
{
    if (!context.players || current_player < 0 ||
        static_cast<std::size_t>(current_player) >= context.players->size()) return;

    Player& player = context.players->at(static_cast<std::size_t>(current_player));

    for (std::size_t i = 0; i < context.players->size(); ++i) {
        Player& other = context.players->at(i);
        if (&other == &player || other.get_is_bankrupt()) continue;

        if (money < 0) {
            player.pay_rent(&other, -money);
        } else {
            other.pay_rent(&player, money);
        }
    }
}

Deck::Deck(CardContext value) : context(value) {}

void Deck::set_context(CardContext value)
{
    context = value;
    setup();
}

Card* Deck::draw_random_card()
{
    if (drawn_card || cards.empty()) return nullptr;
    const std::size_t index = static_cast<std::size_t>(std::rand()) % cards.size();
    drawn_card = std::move(cards[index]);
    cards.erase(cards.begin() + static_cast<std::ptrdiff_t>(index));
    return drawn_card.get();
}

void Deck::return_drawn_card()
{
    if (!drawn_card) return;
    cards.push_back(std::move(drawn_card));
}

void Deck::keep_drawn_card()
{
    if (!drawn_card) return;
    held_cards.push_back(std::move(drawn_card));
}

bool Deck::return_held_jail_free()
{
    for (auto it = held_cards.begin(); it != held_cards.end(); ++it) {
        if ((*it)->is_jail_free()) {
            cards.push_back(std::move(*it));
            held_cards.erase(it);
            return true;
        }
    }
    return false;
}

void Deck::use_random_card(int current_player)
{
    if (Card* card = draw_random_card()) {
        card->use(context, current_player);
        if (card->is_jail_free()) keep_drawn_card();
        else return_drawn_card();
    }
}

void CommunityChestDeck::setup()
{
    cards.clear();
    cards.push_back(std::make_unique<MoveCard>("Advance to Go", FieldType::Start));
    cards.push_back(std::make_unique<MoneyCard>("Bank error in your favor", 200));
    cards.push_back(std::make_unique<MoneyCard>("Doctor's fees", -50));
    cards.push_back(std::make_unique<MoneyCard>("From sale of stock you get $50", 50));
    cards.push_back(std::make_unique<JailFreeCard>("Get Out of Jail Free"));
    cards.push_back(std::make_unique<MoveCard>("Go to Jail", FieldType::GoToPrison, "", false));
    cards.push_back(std::make_unique<PlayersPayCard>("Grand Opera Night", -50));
    cards.push_back(std::make_unique<MoneyCard>("Holiday Fund matures", 100));
    cards.push_back(std::make_unique<MoneyCard>("Income tax refund", 20));
    cards.push_back(std::make_unique<PlayersPayCard>("It is your birthday", -10));
    cards.push_back(std::make_unique<MoneyCard>("Life insurance matures", 100));
    cards.push_back(std::make_unique<MoneyCard>("Pay hospital fees", -100));
    cards.push_back(std::make_unique<MoneyCard>("Pay school fees", -50));
    cards.push_back(std::make_unique<MoneyCard>("Receive $25 consultancy fee", 25));
    cards.push_back(std::make_unique<RepairCard>("You are assessed for street repairs", 40, 115));
    cards.push_back(std::make_unique<MoneyCard>("You have won second prize in a beauty contest", 10));
}

void ChanceDeck::setup()
{
    cards.clear();
    cards.push_back(std::make_unique<MoveCard>("Advance to Boardwalk", FieldType::Street, "Boardwalk"));
    cards.push_back(std::make_unique<MoveCard>("Advance to Go", FieldType::Start));
    cards.push_back(std::make_unique<MoveCard>("Advance to Illinois Avenue", FieldType::Street, "Illinois Avenue"));
    cards.push_back(std::make_unique<MoveCard>("Advance to St. Charles Place", FieldType::Street, "St. Charles Place"));
    cards.push_back(std::make_unique<MoveCard>("Advance to the nearest Railroad", FieldType::Railroad));
    cards.push_back(std::make_unique<MoveCard>("Advance to the nearest Railroad", FieldType::Railroad));
    cards.push_back(std::make_unique<MoveCard>("Advance to the nearest Utility", FieldType::Utility));
    cards.push_back(std::make_unique<MoneyCard>("Bank pays you dividend of $50", 50));
    cards.push_back(std::make_unique<JailFreeCard>("Get Out of Jail Free"));
    cards.push_back(std::make_unique<MoveCard>("Go Back 3 Spaces", FieldType::Unknown, "", false, -3));
    cards.push_back(std::make_unique<MoveCard>("Go to Jail", FieldType::GoToPrison, "", false));
    cards.push_back(std::make_unique<RepairCard>("Make general repairs on all your property", 25, 100));
    cards.push_back(std::make_unique<MoneyCard>("Speeding fine $15", -15));
    cards.push_back(std::make_unique<MoveCard>("Take a trip to Reading Railroad", FieldType::Railroad, "Reading Railroad"));
    cards.push_back(std::make_unique<PlayersPayCard>("You have been elected Chairman of the Board", 50));
    cards.push_back(std::make_unique<MoneyCard>("Your building loan matures", 150));
}
