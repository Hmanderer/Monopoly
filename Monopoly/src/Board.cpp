#include "Board.hpp"
#include "Fields.hpp"
#include <vector>
#include <memory>
#include <string>

Board::Board()
{
}

void Board::setup(CardContext context)
{
	board.clear();
	board.reserve(40);

	// Create the shared decks before creating CardField instances.
	// Card fields keep pointers to these decks, so leaving the unique_ptrs
	// null makes card mechanics silently unusable and can lead to invalid
	// state later in the game.
	community_deck = std::make_unique<CommunityChestDeck>(context);
	chance_deck = std::make_unique<ChanceDeck>(context);

	board.push_back(std::make_unique<StartField>());
	board.push_back(std::make_unique<StreetField>("Mediterranean Avenue", 60, StreetRentData{ 2, 10, 30, 90, 160, 250 }, ColorGroup::Brown));
	board.push_back(std::make_unique<CardField>("Community Chest", FieldType::CommunityChest, context, community_deck.get()));
	board.push_back(std::make_unique<StreetField>("Baltic Avenue", 60, StreetRentData{ 4, 20, 60, 180, 320, 450 }, ColorGroup::Brown));
	board.push_back(std::make_unique<TaxField>("Income Tax", 200));
	board.push_back(std::make_unique<RailroadField>("Reading Railroad"));
	board.push_back(std::make_unique<StreetField>("Oriental Avenue", 100, StreetRentData{ 6, 30, 90, 270, 400, 550 }, ColorGroup::LightBlue));
	board.push_back(std::make_unique<CardField>("Chance", FieldType::Chance, context, chance_deck.get()));
	board.push_back(std::make_unique<StreetField>("Vermont Avenue", 100, StreetRentData{ 6, 30, 90, 270, 400, 550 }, ColorGroup::LightBlue));
	board.push_back(std::make_unique<StreetField>("Connecticut Avenue", 120, StreetRentData{ 8, 40, 100, 300, 450, 600 }, ColorGroup::LightBlue));
	board.push_back(std::make_unique<PrisonField>());
	board.push_back(std::make_unique<StreetField>("St. Charles Place", 140, StreetRentData{ 10, 50, 150, 450, 625, 750 }, ColorGroup::Pink));
	board.push_back(std::make_unique<UtilityField>("Electric Company"));
	board.push_back(std::make_unique<StreetField>("States Avenue", 140, StreetRentData{ 10, 50, 150, 450, 625, 750 }, ColorGroup::Pink));
	board.push_back(std::make_unique<StreetField>("Virginia Avenue", 160, StreetRentData{ 12, 60, 180, 500, 700, 900 }, ColorGroup::Pink));
	board.push_back(std::make_unique<RailroadField>("Pennsylvania Railroad"));
	board.push_back(std::make_unique<StreetField>("St. James Place", 180, StreetRentData{ 14, 70, 200, 550, 750, 950 }, ColorGroup::Orange));
	board.push_back(std::make_unique<CardField>("Community Chest", FieldType::CommunityChest, context, community_deck.get()));
	board.push_back(std::make_unique<StreetField>("Tennessee Avenue", 180, StreetRentData{ 14, 70, 200, 550, 750, 950 }, ColorGroup::Orange));
	board.push_back(std::make_unique<StreetField>("New York Avenue", 200, StreetRentData{ 16, 80, 220, 600, 800, 1000 }, ColorGroup::Orange));
	board.push_back(std::make_unique<FreeParkingField>());
	board.push_back(std::make_unique<StreetField>("Kentucky Avenue", 220, StreetRentData{ 18, 90, 250, 700, 875, 1050 }, ColorGroup::Red));
	board.push_back(std::make_unique<CardField>("Chance", FieldType::Chance, context, chance_deck.get()));
	board.push_back(std::make_unique<StreetField>("Indiana Avenue", 220, StreetRentData{ 18, 90, 250, 700, 875, 1050 }, ColorGroup::Red));
	board.push_back(std::make_unique<StreetField>("Illinois Avenue", 240, StreetRentData{ 20, 100, 300, 750, 925, 1100 }, ColorGroup::Red));
	board.push_back(std::make_unique<RailroadField>("B&O Railroad"));
	board.push_back(std::make_unique<StreetField>("Atlantic Avenue", 260, StreetRentData{ 22, 110, 330, 800, 975, 1150 }, ColorGroup::Yellow));
	board.push_back(std::make_unique<StreetField>("Ventnor Avenue", 260, StreetRentData{ 22, 110, 330, 800, 975, 1150 }, ColorGroup::Yellow));
	board.push_back(std::make_unique<UtilityField>("Water Works"));
	board.push_back(std::make_unique<StreetField>("Marvin Gardens", 280, StreetRentData{ 24, 120, 360, 850, 1025, 1200 }, ColorGroup::Yellow));
	board.push_back(std::make_unique<GoToPrisonField>());
	board.push_back(std::make_unique<StreetField>("Pacific Avenue", 300, StreetRentData{ 26, 130, 390, 900, 1100, 1275 }, ColorGroup::Green));
	board.push_back(std::make_unique<StreetField>("North Carolina Avenue", 300, StreetRentData{ 26, 130, 390, 900, 1100, 1275 }, ColorGroup::Green));
	board.push_back(std::make_unique<CardField>("Community Chest", FieldType::CommunityChest, context, community_deck.get()));
	board.push_back(std::make_unique<StreetField>("Pennsylvania Avenue", 320, StreetRentData{ 28, 150, 450, 1000, 1200, 1400 }, ColorGroup::Green));
	board.push_back(std::make_unique<RailroadField>("Short Line Railroad"));
	board.push_back(std::make_unique<CardField>("Chance", FieldType::Chance, context, chance_deck.get()));
	board.push_back(std::make_unique<StreetField>("Park Place", 350, StreetRentData{ 35, 175, 500, 1100, 1300, 1500 }, ColorGroup::DarkBlue));
	board.push_back(std::make_unique<TaxField>("Luxury Tax", 100));
	board.push_back(std::make_unique<StreetField>("Boardwalk", 400, StreetRentData{ 50, 200, 600, 1400, 1700, 2000 }, ColorGroup::DarkBlue));
}

Field* Board::get_field_on(const int index) const noexcept
{
	if (index >= static_cast<int>(board.size()) || index < 0) {
		return nullptr;
	}
	return board[index].get();
}

int Board::size() const noexcept
{
    return static_cast<int>(board.size());
}

int Board::count_houses() const noexcept
{
    int total = 0;
    for (const auto& field : board) {
        if (auto* street = dynamic_cast<StreetField*>(field.get())) {
            total += street->get_house_val();
        }
    }
    return total;
}

int Board::count_hotels() const noexcept
{
    int total = 0;
    for (const auto& field : board) {
        if (auto* street = dynamic_cast<StreetField*>(field.get())) {
            if (street->has_hotel()) ++total;
        }
    }
    return total;
}

bool Board::return_jail_free_card()
{
    bool returned = false;
    if (chance_deck) returned = chance_deck->return_held_jail_free() || returned;
    if (community_deck) returned = community_deck->return_held_jail_free() || returned;
    return returned;
}

int Board::find(FieldType type, int current_pos, const std::string& name) const
{
	int total = size();
	if (total == 0) return -1;

	// Поиск строго вперед
	for (int offset = 1; offset <= total; ++offset) {
		int idx = (current_pos + offset) % total;

		if (board[idx]->get_type() == type) {
			if (name.empty() || board[idx]->get_name() == name) {
				return idx;
			}
		}
	}

	return -1;
}