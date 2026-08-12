#pragma once

#include "Fields.hpp"

#include <memory>
#include <string>
#include <vector>

class Board
{
private:
    std::vector<std::unique_ptr<Field>> board;
    std::unique_ptr<Deck> community_deck;
    std::unique_ptr<Deck> chance_deck;

public:
    Board();
    ~Board() = default;

    Board(const Board&) = delete;
    Board& operator=(const Board&) = delete;
    Board(Board&&) noexcept = default;
    Board& operator=(Board&&) noexcept = default;

    void setup(CardContext context);
    Field* get_field_on(int index) const noexcept;
    int size() const noexcept;
    int count_houses() const noexcept;
    int count_hotels() const noexcept;
    bool return_jail_free_card();

    // Ищет поле строго вперед по часовой стрелке, начиная со следующей клетки.
    int find(FieldType type, int current_pos = 0, const std::string& name = {}) const;
};
