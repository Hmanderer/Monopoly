#pragma once

#include <cstddef>
#include <string>
#include <vector>

class PropertyField;
class StreetField;
class Board;
enum class FieldType;
enum class ColorGroup;

class Player
{
private:
    std::string name;
    int balance = 0;
    int prison_score = 3;
    int location = 0;
    int get_out_from_jail_card = 0;
    bool in_prison = false;
    bool is_bankrupt = false;
    std::vector<PropertyField*> properties;
    int pending_debt = 0;
    Player* pending_creditor = nullptr;
    std::string last_money_reason = "";
    bool pending_jail_release = false;
    int pending_move_after_debt = 0;

public:
    explicit Player(std::string name);
    ~Player() = default;

    const std::string& get_name() const noexcept;
    int get_balance() const noexcept;
    int get_location() const noexcept;
    int get_index(std::vector<Player>* players);
    bool get_is_bankrupt() const noexcept;
    bool can_pay(int value) const noexcept;
    int get_pending_debt() const noexcept;
    Player* get_pending_creditor() const noexcept;
    bool has_pending_debt() const noexcept;
    const std::string& get_last_money_reason() const noexcept;
    void clear_pending_debt() noexcept;
    void set_pending_jail_release(bool value, int move_steps = 0) noexcept;
    bool get_pending_jail_release() const noexcept;
    int get_pending_move_after_debt() const noexcept;
    bool settle_pending_debt();
    void declare_bankruptcy(Player* creditor) noexcept;

    const std::vector<PropertyField*>& get_properties() const noexcept;

    void set_location(int value) noexcept;
    void set_bankrupt_state(bool value) noexcept;

    bool is_in_prison() const noexcept;
    void set_in_prison(bool value) noexcept;

    int get_prison_score() const noexcept;
    void set_prison_score(int value) noexcept;

    int get_get_out_from_jail_card() const noexcept;
    void set_get_out_from_jail_card(int value) noexcept;

    void destroy() noexcept;
    int count_property(FieldType type) const;
    int count_property(ColorGroup group) const;

    void add_money(int amount) noexcept;
    void add_money(int amount, const std::string& reason) noexcept;
    bool remove_money(int amount);
    bool remove_money(int amount, const std::string& reason);
    void liquidate_property(std::size_t index);
    void add_property(PropertyField* property);
    bool remove_property(PropertyField* property) noexcept;
    void sell_house(StreetField* property);
    bool pay_rent(Player* receiver, int value);
    void move_steps(int steps) noexcept;
    void set_location_with_go(int new_loc) noexcept;

    bool owns_property(const PropertyField* property) const;
    bool owns_full_group(ColorGroup group, const Board& board) const;
    bool can_build_house(StreetField* street, const Board& board) const;
    bool can_build_house(StreetField* street, const Board& board, int total_houses, int total_hotels) const;
    bool can_sell_house(StreetField* street, const Board& board) const;
    bool can_sell_property(PropertyField* property, const Board& board) const;
};
