#include "Player.hpp"
#include "Fields.hpp"
#include "Board.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

Player::Player(std::string name)
	: name(std::move(name)), balance(0), prison_score(3), location(0),
	get_out_from_jail_card(0), in_prison(false), is_bankrupt(false)
{
}

const std::string& Player::get_name() const noexcept
{
	return name;
}

int Player::get_balance() const noexcept
{
	return balance;
}

int Player::get_prison_score() const noexcept
{
	return prison_score;
}

int Player::get_location() const noexcept
{
	return location;
}

int Player::get_index(std::vector<Player>* players) 
{
	for (size_t i = 0; i < players->size(); ++i) {
		Player& current = (*players)[i];

		if (this == &current) {
			return i;
		}
	}

	return -1;
}

int Player::get_get_out_from_jail_card() const noexcept
{
	return get_out_from_jail_card;
}

bool Player::is_in_prison() const noexcept
{
	return in_prison;
}

bool Player::get_is_bankrupt() const noexcept
{
	return is_bankrupt;
}

bool Player::can_pay(int val) const noexcept
{
	return val >= 0 && balance >= val;
}

int Player::get_pending_debt() const noexcept
{
	return pending_debt;
}

Player* Player::get_pending_creditor() const noexcept
{
	return pending_creditor;
}

bool Player::has_pending_debt() const noexcept
{
	return pending_debt > 0;
}

void Player::set_pending_jail_release(bool value, int move_steps) noexcept
{
    pending_jail_release = value;
    pending_move_after_debt = value ? std::max(0, move_steps) : 0;
}

bool Player::get_pending_jail_release() const noexcept
{
    return pending_jail_release;
}

int Player::get_pending_move_after_debt() const noexcept
{
    return pending_move_after_debt;
}

void Player::clear_pending_debt() noexcept
{
	pending_debt = 0;
	pending_creditor = nullptr;
}

bool Player::settle_pending_debt()
{
	if (pending_debt <= 0) return true;
	if (balance < pending_debt) return false;

    balance -= pending_debt;
    last_money_reason = "Debt paid";
    if (pending_creditor && !pending_creditor->get_is_bankrupt()) {
        pending_creditor->add_money(pending_debt, "Debt received");
    }
    clear_pending_debt();
	return true;
}

void Player::declare_bankruptcy(Player* creditor) noexcept
{
    const int jail_cards = get_out_from_jail_card;
    std::vector<PropertyField*> remaining = properties;
    properties.clear();

    if (creditor && !creditor->get_is_bankrupt()) {
        // In bankruptcy to another player, the creditor receives the
        // properties in their current state (including mortgages).
        for (auto* property : remaining) {
            if (property) creditor->add_property(property);
        }
        if (jail_cards > 0) {
            creditor->set_get_out_from_jail_card(
                creditor->get_get_out_from_jail_card() + jail_cards);
        }
    }
    else {
        // Bankruptcy to the bank: all buildings and ownership return to bank.
        for (auto* property : remaining) {
            if (property) property->reset();
        }
    }

    balance = 0;
    get_out_from_jail_card = 0;
    clear_pending_debt();
    pending_jail_release = false;
    pending_move_after_debt = 0;
    in_prison = false;
    is_bankrupt = true;
}

const std::string& Player::get_last_money_reason() const noexcept
{
	return last_money_reason;
}

const std::vector<PropertyField*>& Player::get_properties() const noexcept
{
	return properties;
}

void Player::destroy() noexcept
{
	declare_bankruptcy(nullptr);
}

void Player::set_bankrupt_state(bool value) noexcept
{
    is_bankrupt = value;
}

void Player::set_location(const int val) noexcept
{
	location = ((val % 40) + 40) % 40;
}

int Player::count_property(FieldType type) const
{
	int counter = 0;
	for (const auto* property : properties) {
		if (property && property->get_type() == type) {
			++counter;
		}
	}
	return counter;
}

int Player::count_property(ColorGroup group) const
{
	int counter = 0;
	for (const auto* property : properties) {
		if (!property || property->get_type() != FieldType::Street) {
			continue;
		}

		const auto* street = dynamic_cast<const StreetField*>(property);
		if (street && street->get_group() == group) {
			++counter;
		}
	}
	return counter;
}

void Player::add_money(int amount) noexcept
{
    add_money(amount, amount >= 0 ? "Money received" : "Payment");
}

void Player::add_money(int amount, const std::string& reason) noexcept
{
    balance += amount;
    last_money_reason = reason;
}

void Player::liquidate_property(const size_t index)
{
    if (index >= properties.size()) return;

    add_money(properties[index]->get_sale_value(), "Property sold");
    properties[index]->reset();
    properties.erase(properties.begin() + index);
}

void Player::add_property(PropertyField* property)
{
	if (!property) return;
	properties.push_back(property);
	property->set_owner(this);
}

bool Player::remove_property(PropertyField* property) noexcept
{
    if (!property) return false;
    auto it = std::find(properties.begin(), properties.end(), property);
    if (it == properties.end()) return false;
    properties.erase(it);
    return true;
}

void Player::sell_house(StreetField* property)
{
    if (!property) return;

    const bool had_building = property->has_hotel() || property->get_house_val() > 0;
    if (!had_building) return;

    const bool was_hotel = property->has_hotel();
    if (was_hotel) {
        property->sell_hotel();
        add_money(property->get_house_price() * 5 / 2, "Hotel sold");
    } else {
        property->sell_house();
        add_money(property->get_house_sale_value(), "House sold");
    }
}

bool Player::remove_money(int amount)
{
    return remove_money(amount, "Payment");
}

bool Player::remove_money(int amount, const std::string& reason)
{
    if (amount < 0) return false;
    if (amount == 0) return true;
    if (has_pending_debt()) return false;
    if (balance >= amount) {
        balance -= amount;
        last_money_reason = reason;
        return true;
    }

    // Never liquidate assets automatically. The Game opens the bankruptcy
    // menu and lets the player choose what to sell.
    pending_debt = amount;
    pending_creditor = nullptr;
    last_money_reason = reason;
    return false;
}

bool Player::pay_rent(Player* receiver, int val)
{
    if (!receiver || receiver == this || val < 0) return false;
    if (has_pending_debt()) return false;
    if (balance >= val) {
        balance -= val;
        last_money_reason = "Rent paid";
        receiver->add_money(val, "Rent received");
        return true;
    }

    pending_debt = val;
    pending_creditor = receiver;
    last_money_reason = "Rent payment required";
    return false;
}

void Player::move_steps(int steps) noexcept
{
    if (steps == 0) return;

    const int old_location = location;
    location = ((location + steps) % 40 + 40) % 40;

    // Только движение вперед через START приносит $200.
    if (steps > 0 && location < old_location) {
        add_money(200, "Passed GO (+$200)");
    }
}

void Player::set_location_with_go(int new_loc) noexcept
{
    const int target = ((new_loc % 40) + 40) % 40;
    if (target < location) {
        add_money(200, "Passed GO (+$200)");
    }
    location = target;
}

void Player::set_get_out_from_jail_card(int value) noexcept
{
	get_out_from_jail_card = value;
}

void Player::set_in_prison(bool b) noexcept
{
	in_prison = b;
	if (!b) {
		prison_score = 3;
	}
}

void Player::set_prison_score(int val) noexcept
{
	prison_score = val;
}

bool Player::owns_property(const PropertyField* prop) const {
	return std::find(properties.begin(), properties.end(), prop) != properties.end();
}

bool Player::owns_full_group(ColorGroup group, const Board& board) const {
	int total_in_group = 0;
	int owned_in_group = 0;

	for (int i = 0; i < board.size(); ++i) {
		Field* f = board.get_field_on(i);
		if (f && f->get_type() == FieldType::Street) {
			auto* street = dynamic_cast<StreetField*>(f);
			if (street && street->get_group() == group) {
				total_in_group++;
				if (owns_property(street)) {
					owned_in_group++;
				}
			}
		}
	}
	return (total_in_group > 0) && (total_in_group == owned_in_group);
}

bool Player::can_build_house(StreetField* street, const Board& board) const {
    return can_build_house(street, board, board.count_houses(), board.count_hotels());
}

bool Player::can_build_house(StreetField* street, const Board& board, int total_houses, int total_hotels) const {
	if (!street || !owns_property(street) || street->is_mortgaged()) return false;
	if (!owns_full_group(street->get_group(), board)) return false;
    for (int i = 0; i < board.size(); ++i) {
        Field* f = board.get_field_on(i);
        auto* other = f && f->get_type() == FieldType::Street ? dynamic_cast<StreetField*>(f) : nullptr;
        if (other && other->get_group() == street->get_group() && other->is_mortgaged()) return false;
    }
	if (street->has_hotel()) return false;
	if (!can_pay(street->get_house_price())) return false;

    const bool building_hotel = street->get_house_val() == 4;
    if (building_hotel) {
        if (total_hotels >= 12) return false;
    } else if (total_houses >= 32) {
        return false;
    }

	int current_level = street->get_house_val();

	for (int i = 0; i < board.size(); ++i) {
		Field* f = board.get_field_on(i);
		if (f && f->get_type() == FieldType::Street) {
			auto* other = dynamic_cast<StreetField*>(f);
			if (other && other->get_group() == street->get_group()) {
				int other_level = other->has_hotel() ? 5 : other->get_house_val();
				if (other_level < current_level) {
					return false;
				}
			}
		}
	}
	return true;
}

bool Player::can_sell_house(StreetField* street, const Board& board) const {
	if (!street || !owns_property(street)) return false;
	int current_level = street->has_hotel() ? 5 : street->get_house_val();
	if (current_level == 0) return false;

	for (int i = 0; i < board.size(); ++i) {
		Field* f = board.get_field_on(i);
		if (f && f->get_type() == FieldType::Street) {
			auto* other = dynamic_cast<StreetField*>(f);
			if (other && other->get_group() == street->get_group()) {
				int other_level = other->has_hotel() ? 5 : other->get_house_val();
				if (other_level > current_level) {
					return false;
				}
			}
		}
	}
	return true;
}

bool Player::can_sell_property(PropertyField* property, const Board& board) const
{
    if (!property || !owns_property(property)) return false;

    auto* street = dynamic_cast<StreetField*>(property);
    if (!street) return true;

    // Buildings on every property in a colour group must be removed before
    // any property in that group can be sold.
    for (int i = 0; i < board.size(); ++i) {
        Field* field = board.get_field_on(i);
        auto* other = field && field->get_type() == FieldType::Street
            ? dynamic_cast<StreetField*>(field)
            : nullptr;
        if (!other || other->get_group() != street->get_group()) continue;
        if (other->has_hotel() || other->get_house_val() > 0) return false;
    }
    return true;
}
