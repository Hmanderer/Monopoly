#include "Game.hpp"
#include "Fields.hpp"
#include "Dice.hpp"
#include "Board.hpp"
#include "TurnManager.hpp"
#include "Player.hpp"
#include <string>
#include <algorithm>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <fstream>

static bool is_point_in_rect(float x, float y, const SDL_FRect& rect) noexcept
{
    return (x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h);
}

Game::Game()
{
    scene = SceneState::MainMenu;
    is_setup = false;
    is_running = true;
    state = GameState::Setup;
    selected_player_count = 2;

    font = open_font("assets/fonts/ARIAL.TTF", 24);
    board_font = open_font("assets/fonts/ARIAL.TTF", 12);

    if (!font || !board_font) {
        std::cerr << "Не удалось открыть шрифт assets/fonts/ARIAL.TTF: "
                  << SDL_GetError() << std::endl;
    }
}

TTF_Font* Game::open_font(const char* path, int size)
{
    if (!path || size <= 0) return nullptr;

    if (TTF_Font* loaded = TTF_OpenFont(path, size)) {
        return loaded;
    }

    // При запуске из x64/Debug рабочей директорией иногда оказывается
    // каталог сборки, поэтому пробуем несколько относительных путей.
    const char* alternatives[] = {
        "../assets/fonts/ARIAL.TTF",
        "../../assets/fonts/ARIAL.TTF"
    };

    for (const char* alternative : alternatives) {
        if (TTF_Font* loaded = TTF_OpenFont(alternative, size)) {
            return loaded;
        }
    }

    return nullptr;
}

Game::~Game()
{
    if (font) TTF_CloseFont(font);
    if (board_font) TTF_CloseFont(board_font);
}

void Game::render_text(SDL_Renderer* renderer, const std::string& text, SDL_Color color, const SDL_FRect& rect, TTF_Font* font)
{
    if (!font || text.empty()) return;

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_FRect dst_rect;
        dst_rect.w = static_cast<float>(surface->w);
        dst_rect.h = static_cast<float>(surface->h);
        dst_rect.x = rect.x + (rect.w - dst_rect.w) / 2.0f;
        dst_rect.y = rect.y + (rect.h - dst_rect.h) / 2.0f;

        SDL_RenderTexture(renderer, texture, nullptr, &dst_rect);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

SDL_FRect Game::get_field_rect(int index) const noexcept
{
    float corner_size = 90.0f;
    float field_width = 60.0f;
    float field_height = 90.0f;
    float offset_x = 50.0f;
    float offset_y = 50.0f;
    float board_side = corner_size * 2.0f + 9.0f * field_width;

    SDL_FRect rect = { 0, 0, 0, 0 };

    if (index == 0) {
        rect = { offset_x + board_side - corner_size, offset_y + board_side - corner_size, corner_size, corner_size };
    }
    else if (index > 0 && index < 10) {
        rect = { offset_x + board_side - corner_size - index * field_width, offset_y + board_side - field_height, field_width, field_height };
    }
    else if (index == 10) {
        rect = { offset_x, offset_y + board_side - corner_size, corner_size, corner_size };
    }
    else if (index > 10 && index < 20) {
        rect = { offset_x, offset_y + board_side - corner_size - (index - 10) * field_width, field_height, field_width };
    }
    else if (index == 20) {
        rect = { offset_x, offset_y, corner_size, corner_size };
    }
    else if (index > 20 && index < 30) {
        rect = { offset_x + corner_size + (index - 21) * field_width, offset_y, field_width, field_height };
    }
    else if (index == 30) {
        rect = { offset_x + board_side - corner_size, offset_y, corner_size, corner_size };
    }
    else if (index > 30 && index < 40) {
        rect = { offset_x + board_side - field_height, offset_y + corner_size + (index - 31) * field_width, field_height, field_width };
    }

    return rect;
}

SDL_Color Game::get_group_color(ColorGroup group) const noexcept
{
    switch (group) {
    case ColorGroup::Brown:      return { 139, 69, 19, 255 };
    case ColorGroup::LightBlue:  return { 173, 216, 230, 255 };
    case ColorGroup::Pink:       return { 255, 105, 180, 255 };
    case ColorGroup::Orange:     return { 255, 140, 0, 255 };
    case ColorGroup::Red:        return { 220, 20, 60, 255 };
    case ColorGroup::Yellow:     return { 255, 215, 0, 255 };
    case ColorGroup::Green:      return { 34, 139, 34, 255 };
    case ColorGroup::DarkBlue:   return { 0, 0, 139, 255 };
    default:                     return { 200, 200, 200, 255 };
    }
}

void Game::draw_board(SDL_Renderer* renderer)
{
    Board* board = turn_manager.get_board();
    if (!board) return;

    SDL_Color text_color = { 0, 0, 0, 255 };
    SDL_Color player_colors[8] = {
        {231,76,60,255},{52,152,219,255},{46,204,113,255},{241,196,15,255},
        {155,89,182,255},{230,126,34,255},{26,188,156,255},{149,165,166,255}
    };

    for (int i = 0; i < board->size(); ++i) {
        SDL_FRect rect = get_field_rect(i);
        SDL_SetRenderDrawColor(renderer, 245, 245, 240, 255);
        SDL_RenderFillRect(renderer, &rect);

        Field* field = board->get_field_on(i);
        SDL_FRect text_rect = rect;

        if (field && field->get_type() == FieldType::Street) {
            auto* street = dynamic_cast<StreetField*>(field);
            if (street) {
                SDL_FRect strip = rect;
                float strip_size = 20.0f;

                if (i > 0 && i < 10) {
                    strip.h = strip_size;
                    text_rect.y += strip_size;
                    text_rect.h -= strip_size;
                }
                else if (i > 10 && i < 20) {
                    strip.x = rect.x + rect.w - strip_size;
                    strip.w = strip_size;
                    text_rect.w -= strip_size;
                }
                else if (i > 20 && i < 30) {
                    strip.y = rect.y + rect.h - strip_size;
                    strip.h = strip_size;
                    text_rect.h -= strip_size;
                }
                else if (i > 30 && i < 40) {
                    strip.w = strip_size;
                    text_rect.x += strip_size;
                    text_rect.w -= strip_size;
                }

                SDL_Color color = get_group_color(street->get_group());
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
                SDL_RenderFillRect(renderer, &strip);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderRect(renderer, &strip);

                if (street->has_hotel()) {
                    SDL_FRect hotel_rect;
                    if ((i > 0 && i < 10) || (i > 20 && i < 30)) {
                        hotel_rect = { strip.x + strip.w / 2.0f - 8.0f, strip.y + 3.0f, 16.0f, strip.h - 6.0f };
                    }
                    else {
                        hotel_rect = { strip.x + 3.0f, strip.y + strip.h / 2.0f - 8.0f, strip.w - 6.0f, 16.0f };
                    }
                    SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
                    SDL_RenderFillRect(renderer, &hotel_rect);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderRect(renderer, &hotel_rect);
                }
                else if (street->get_house_val() > 0) {
                    int house_count = street->get_house_val();
                    bool horizontal = (i > 0 && i < 10) || (i > 20 && i < 30);
                    float h_size = horizontal ? (strip.w / 4.0f) - 2.0f : (strip.h / 4.0f) - 2.0f;

                    for (int h = 0; h < house_count; ++h) {
                        SDL_FRect house_rect;
                        if (horizontal) {
                            house_rect = { strip.x + 2.0f + h * (h_size + 2.0f), strip.y + 3.0f, h_size, strip.h - 6.0f };
                        }
                        else {
                            house_rect = { strip.x + 3.0f, strip.y + 2.0f + h * (h_size + 2.0f), strip.w - 6.0f, h_size };
                        }
                        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
                        SDL_RenderFillRect(renderer, &house_rect);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderRect(renderer, &house_rect);
                    }
                }
            }
        }

        if (field) {
            auto* property = dynamic_cast<PropertyField*>(field);
            if (property && property->get_owner()) {
                for (size_t p_idx = 0; p_idx < players.size(); ++p_idx) {
                    if (&players[p_idx] == property->get_owner()) {
                        SDL_FRect owner_marker = { rect.x + rect.w - 12.0f, rect.y + rect.h - 12.0f, 9.0f, 9.0f };
                        SDL_Color owner_color = player_colors[p_idx % 8];
                        SDL_SetRenderDrawColor(renderer, owner_color.r, owner_color.g, owner_color.b, 255);
                        SDL_RenderFillRect(renderer, &owner_marker);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                        SDL_RenderRect(renderer, &owner_marker);
                        break;
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &rect);

        if (field) {
            // Для купленной улицы оставляем нижнюю часть клетки под
            // отображение текущей арендной платы. Значение берётся из
            // той же StreetField::get_rent(), которая используется при
            // реальном начислении аренды.
            SDL_FRect name_rect = text_rect;
            SDL_FRect rent_rect = { 0, 0, 0, 0 };
            bool show_rent = false;

            if (auto* street = dynamic_cast<StreetField*>(field);
                street && street->get_owner() != nullptr) {
                const float rent_height = 18.0f;
                if (name_rect.h > rent_height + 8.0f) {
                    name_rect.h -= rent_height;
                    rent_rect = {
                        text_rect.x,
                        text_rect.y + text_rect.h - rent_height,
                        text_rect.w,
                        rent_height
                    };
                    show_rent = true;

                    Dice preview_dice;
                    const int displayed_rent = street->get_rent(preview_dice, *board);
                    const bool has_monopoly =
                        street->get_house_val() == 0 &&
                        !street->has_hotel() &&
                        street->get_owner()->owns_full_group(street->get_group(), *board);

                    std::string rent_text = "$" + std::to_string(displayed_rent);
                    if (has_monopoly) {
                        rent_text += "  x2";
                    }

                    render_text(renderer, rent_text,
                        text_color, rent_rect, board_font);
                }
            }

            std::string name = field->get_name();
            size_t space_pos = name.find(' ');
            if (space_pos != std::string::npos && name.length() > 10) {
                std::string line1 = name.substr(0, space_pos);
                std::string line2 = name.substr(space_pos + 1);
                SDL_FRect top_half = { name_rect.x, name_rect.y, name_rect.w, name_rect.h / 2.0f };
                SDL_FRect bot_half = { name_rect.x, name_rect.y + name_rect.h / 2.0f, name_rect.w, name_rect.h / 2.0f };
                render_text(renderer, line1, text_color, top_half, board_font);
                render_text(renderer, line2, text_color, bot_half, board_font);
            }
            else {
                render_text(renderer, name, text_color, name_rect, board_font);
            }
        }
    }
}

void Game::draw_players(SDL_Renderer* renderer)
{
    SDL_Color player_colors[8] = {
        {231,76,60,255},{52,152,219,255},{46,204,113,255},{241,196,15,255},
        {155,89,182,255},{230,126,34,255},{26,188,156,255},{149,165,166,255}
    };

    for (size_t i = 0; i < players.size(); ++i) {
        if (players[i].get_is_bankrupt()) continue;

        int loc = players[i].get_location();
        SDL_FRect field_rect = get_field_rect(loc);

        float offset_x = (i % 4) * 14.0f + 6.0f;
        float offset_y = (i / 4) * 20.0f + 6.0f;

        SDL_FRect token_rect = { field_rect.x + offset_x, field_rect.y + offset_y, 16.0f, 16.0f };
        SDL_Color color = player_colors[i % 8];

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &token_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &token_rect);
    }
}

void Game::draw_dice(SDL_Renderer* renderer)
{
    const Dice& dice = turn_manager.get_dice();
    if (!dice.has_rolled_once()) return;

    const auto [first, second] = dice.get_values();

    // Кубики находятся справа от игрового поля, над панелью игроков.
    const float size = 72.0f;
    const float gap = 18.0f;
    const float x1 = 875.0f;
    const float y = 410.0f;
    const float x2 = x1 + size + gap;

    auto draw_single_die = [&](float x, int value)
    {
        SDL_FRect rect{ x, y, size, size };

        SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderRect(renderer, &rect);

        const float cx = x + size / 2.0f;
        const float cy = y + size / 2.0f;
        const float offset = 18.0f;
        const float pip_size = 10.0f;

        auto pip = [&](float px, float py)
        {
            SDL_FRect dot{ px - pip_size / 2.0f, py - pip_size / 2.0f,
                           pip_size, pip_size };
            SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
            SDL_RenderFillRect(renderer, &dot);
        };

        switch (value) {
        case 1:
            pip(cx, cy);
            break;
        case 2:
            pip(cx - offset, cy - offset);
            pip(cx + offset, cy + offset);
            break;
        case 3:
            pip(cx - offset, cy - offset);
            pip(cx, cy);
            pip(cx + offset, cy + offset);
            break;
        case 4:
            pip(cx - offset, cy - offset);
            pip(cx + offset, cy - offset);
            pip(cx - offset, cy + offset);
            pip(cx + offset, cy + offset);
            break;
        case 5:
            pip(cx - offset, cy - offset);
            pip(cx + offset, cy - offset);
            pip(cx, cy);
            pip(cx - offset, cy + offset);
            pip(cx + offset, cy + offset);
            break;
        case 6:
            pip(cx - offset, cy - offset);
            pip(cx + offset, cy - offset);
            pip(cx - offset, cy);
            pip(cx + offset, cy);
            pip(cx - offset, cy + offset);
            pip(cx + offset, cy + offset);
            break;
        default:
            break;
        }
    };

    draw_single_die(x1, first);
    draw_single_die(x2, second);

    SDL_Color white{ 255, 255, 255, 255 };
    SDL_FRect label_rect{ x1 - 25.0f, y + size + 8.0f, size * 2.0f + gap + 50.0f, 32.0f };

    if (dice.is_double()) {
        render_text(renderer,
                    "DOUBLE!  " + std::to_string(turn_manager.get_consecutive_doubles()) + "/3",
                    { 241, 196, 15, 255 },
                    label_rect,
                    font);
    }
    else {
        render_text(renderer,
                    "Sum: " + std::to_string(dice.get_sum()),
                    white,
                    label_rect,
                    font);
    }
}

void Game::draw_hud(SDL_Renderer* renderer)
{
    SDL_Color white_color = { 255, 255, 255, 255 };
    SDL_Color player_colors[8] = {
        {231,76,60,255},{52,152,219,255},{46,204,113,255},{241,196,15,255},
        {155,89,182,255},{230,126,34,255},{26,188,156,255},{149,165,166,255}
    };

    size_t current_idx = turn_manager.get_current_player_index();

    for (size_t i = 0; i < players.size(); ++i) {
        const size_t col = players.size() > 4 ? (i % 2) : 0;
        const size_t row = players.size() > 4 ? (i / 2) : i;
        const float card_w = players.size() > 4 ? 195.0f : 410.0f;
        const float card_h = players.size() > 4 ? 52.0f : 85.0f;
        const float card_x = players.size() > 4 ? 820.0f + col * 205.0f : 820.0f;
        const float card_y = players.size() > 4 ? 45.0f + row * 60.0f : 50.0f + row * 100.0f;
        SDL_FRect card_rect = { card_x, card_y, card_w, card_h };

        SDL_SetRenderDrawColor(renderer, players[i].get_is_bankrupt() ? 100 : 45, 52, 54, 255);
        SDL_RenderFillRect(renderer, &card_rect);

        SDL_FRect color_strip = { card_rect.x, card_rect.y, 15.0f, card_rect.h };
        SDL_SetRenderDrawColor(renderer, player_colors[i % 8].r, player_colors[i % 8].g, player_colors[i % 8].b, 255);
        SDL_RenderFillRect(renderer, &color_strip);

        if (i == current_idx && !players[i].get_is_bankrupt()) {
            SDL_SetRenderDrawColor(renderer, 241, 196, 15, 255);
            SDL_RenderRect(renderer, &card_rect);
        }
        else {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderRect(renderer, &card_rect);
        }

        std::string info_text = players[i].get_name() + (players[i].get_is_bankrupt() ? " (Bankrupt)" : ": $" + std::to_string(players[i].get_balance()));
        SDL_FRect text_rect = { card_rect.x + 20.0f, card_rect.y + 5.0f, card_rect.w - 30.0f, card_rect.h - 10.0f };
        render_text(renderer, info_text, white_color, text_rect, font);
    }

    draw_dice(renderer);

    if (state == GameState::WaitingRoll) {
        SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
        SDL_RenderFillRect(renderer, &btn_roll_dice);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &btn_roll_dice);
        render_text(renderer, "Roll Dice", white_color, btn_roll_dice, font);
    }

    if (state == GameState::PlayingField) {
        const bool roll_again = turn_manager.can_roll_again();

        SDL_SetRenderDrawColor(renderer,
            roll_again ? 46 : 230,
            roll_again ? 204 : 126,
            roll_again ? 113 : 34,
            255);
        SDL_RenderFillRect(renderer, &btn_roll_dice);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &btn_roll_dice);

        render_text(renderer,
                    roll_again ? "Roll Again (DOUBLE)" : "End Turn",
                    white_color,
                    btn_roll_dice,
                    font);

        SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255);
        SDL_RenderFillRect(renderer, &btn_manage_houses);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &btn_manage_houses);
        render_text(renderer, "BUILD / SELL", white_color, btn_manage_houses, font);

        SDL_SetRenderDrawColor(renderer, 155, 89, 182, 255);
        SDL_RenderFillRect(renderer, &btn_exchange);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &btn_exchange);
        render_text(renderer, "EXCHANGE", white_color, btn_exchange, font);

    }
}

void Game::draw_jail_menu(SDL_Renderer* renderer)
{
    Player* current = turn_manager.get_current_player();
    if (!current) return;

    SDL_Color white = { 255, 255, 255, 255 };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen_overlay = { 0, 0, 1280, 820 };
    SDL_RenderFillRect(renderer, &screen_overlay);

    SDL_SetRenderDrawColor(renderer, 45, 52, 54, 255);
    SDL_RenderFillRect(renderer, &modal_jail_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_jail_rect);

    SDL_FRect header_rect = { modal_jail_rect.x, modal_jail_rect.y + 10, modal_jail_rect.w, 40.0f };
    render_text(renderer, "IN PRISON (Turns left: " + std::to_string(current->get_prison_score()) + ")", white, header_rect, font);

    // Кнопка: Бросить дубль
    SDL_SetRenderDrawColor(renderer, 52, 152, 219, 255);
    SDL_RenderFillRect(renderer, &btn_jail_roll);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_jail_roll);
    render_text(renderer, "Roll for Double", white, btn_jail_roll, font);

    // Кнопка: Заплатить 50$
    bool can_pay = current->can_pay(50);
    SDL_SetRenderDrawColor(renderer, can_pay ? 46 : 120, can_pay ? 204 : 120, can_pay ? 113 : 120, 255);
    SDL_RenderFillRect(renderer, &btn_jail_pay);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_jail_pay);
    render_text(renderer, "Pay $50", white, btn_jail_pay, font);

    // Кнопка: Карточка выхода
    bool has_card = current->get_get_out_from_jail_card() > 0;
    SDL_SetRenderDrawColor(renderer, has_card ? 241 : 120, has_card ? 196 : 120, has_card ? 15 : 120, 255);
    SDL_RenderFillRect(renderer, &btn_jail_card);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_jail_card);
    render_text(renderer, "Use Out of Jail Card", white, btn_jail_card, font);
}

void Game::draw_buy_menu(SDL_Renderer* renderer)
{
    Player* current_player = turn_manager.get_current_player();
    if (!current_player) return;

    Field* field = turn_manager.get_board()->get_field_on(current_player->get_location());
    auto* property = dynamic_cast<PropertyField*>(field);
    if (!property) return;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color black = { 0, 0, 0, 255 };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_FRect screen_overlay = { 0, 0, 1280, 820 };
    SDL_RenderFillRect(renderer, &screen_overlay);

    SDL_SetRenderDrawColor(renderer, 245, 245, 240, 255);
    SDL_RenderFillRect(renderer, &modal_buy_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_buy_rect);

    SDL_FRect header_rect = { modal_buy_rect.x, modal_buy_rect.y, modal_buy_rect.w, 70.0f };
    auto* street = dynamic_cast<StreetField*>(property);
    if (street) {
        SDL_Color group_color = get_group_color(street->get_group());
        SDL_SetRenderDrawColor(renderer, group_color.r, group_color.g, group_color.b, 255);
    }
    else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    }
    SDL_RenderFillRect(renderer, &header_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &header_rect);

    render_text(renderer, property->get_name(), white, header_rect, font);

    int cost = property->get_price();
    bool can_afford = current_player->can_pay(cost);

    SDL_FRect info_rect_1 = { modal_buy_rect.x + 20, modal_buy_rect.y + 90, modal_buy_rect.w - 40, 40 };
    SDL_FRect info_rect_2 = { modal_buy_rect.x + 20, modal_buy_rect.y + 140, modal_buy_rect.w - 40, 40 };
    SDL_FRect info_rect_3 = { modal_buy_rect.x + 20, modal_buy_rect.y + 280, modal_buy_rect.w - 40, 40 };

    render_text(renderer, "Price: $" + std::to_string(cost), black, info_rect_1, font);
    render_text(renderer, "Base Rent: $" + std::to_string(property->get_rent(turn_manager.get_dice())), black, info_rect_2, font);

    std::string balance_str = "Your cash: $" + std::to_string(current_player->get_balance());
    render_text(renderer, balance_str, can_afford ? SDL_Color{ 46, 204, 113, 255 } : SDL_Color{ 231, 76, 60, 255 }, info_rect_3, font);

    SDL_SetRenderDrawColor(renderer, can_afford ? 46 : 189, can_afford ? 204 : 195, can_afford ? 113 : 199, 255);
    SDL_RenderFillRect(renderer, &btn_modal_buy);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_modal_buy);
    render_text(renderer, "BUY", white, btn_modal_buy, font);

    SDL_SetRenderDrawColor(renderer, 231, 76, 60, 255);
    SDL_RenderFillRect(renderer, &btn_modal_pass);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_modal_pass);
    render_text(renderer, "PASS", white, btn_modal_pass, font);
}

void Game::draw_card_menu(SDL_Renderer* renderer)
{
    Player* player = turn_manager.get_current_player();
    Board* board = turn_manager.get_board();
    if (!player || !board) return;

    Field* field = board->get_field_on(player->get_location());
    auto* card_field = dynamic_cast<CardField*>(field);
    if (!card_field) return;

    const Card* card = card_field->get_current();
    if (!card) return;

    const SDL_Color white{ 255, 255, 255, 255 };
    const SDL_Color black{ 20, 20, 20, 255 };

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    const SDL_FRect overlay{ 0, 0, 1280, 820 };
    SDL_RenderFillRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 250, 248, 235, 255);
    SDL_RenderFillRect(renderer, &modal_card_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_card_rect);

    const SDL_FRect header{ modal_card_rect.x, modal_card_rect.y,
                            modal_card_rect.w, 85.0f };
    SDL_SetRenderDrawColor(renderer,
                           field->get_type() == FieldType::Chance ? 52 : 46,
                           field->get_type() == FieldType::Chance ? 152 : 204,
                           field->get_type() == FieldType::Chance ? 219 : 113,
                           255);
    SDL_RenderFillRect(renderer, &header);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &header);

    const std::string title = field->get_type() == FieldType::Chance
        ? "CHANCE"
        : "COMMUNITY CHEST";
    render_text(renderer, title, white, header, font);

    // Simple word wrapping so long card descriptions remain readable.
    std::string line;
    float y = modal_card_rect.y + 115.0f;
    const float line_height = 38.0f;
    const float left = modal_card_rect.x + 35.0f;
    const float width = modal_card_rect.w - 70.0f;

    for (char ch : card->get_event_name()) {
        if (ch == ' ') {
            const std::string candidate = line.empty() ? line : line + ' ';
            int text_w = 0;
            int text_h = 0;
            TTF_GetStringSize(font, candidate.c_str(), candidate.size(), &text_w, &text_h);
            if (text_w > static_cast<int>(width) && !line.empty()) {
                render_text(renderer, line, black, { left, y, width, line_height }, font);
                y += line_height;
                line.clear();
            } else {
                line = candidate;
            }
        } else {
            line += ch;
        }
    }
    if (!line.empty()) {
        render_text(renderer, line, black, { left, y, width, line_height }, font);
    }

    SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
    SDL_RenderFillRect(renderer, &btn_card_ok);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_card_ok);
    render_text(renderer, "OK", white, btn_card_ok, font);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}


void Game::begin_name_entry()
{
    pending_player_names.assign(static_cast<std::size_t>(selected_player_count), "");
    editing_name_index = 0;
    name_input.clear();
    name_typing = true;
    scene = SceneState::NameEntry;
    // The actual SDL window is captured from the mouse event before this
    // method is called. Do not assume its ID is 1.
    if (text_input_window) SDL_StartTextInput(text_input_window);
}

void Game::begin_first_roll()
{
    first_roll_candidates.clear();
    first_roll_results.assign(players.size(), 0);
    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        first_roll_candidates.push_back(i);
    }
    first_roll_position = 0;
    first_roll_round = 1;
    first_roll_has_result = false;
    turn_manager.get_dice().reset();
    state = GameState::FirstRoll;
}

void Game::roll_for_starting_player()
{
    if (first_roll_candidates.empty() || first_roll_position >= static_cast<int>(first_roll_candidates.size())) return;

    turn_manager.get_dice().roll();
    const int player_index = first_roll_candidates[static_cast<std::size_t>(first_roll_position)];
    first_roll_results[static_cast<std::size_t>(player_index)] = turn_manager.get_dice().get_sum();
    first_roll_has_result = true;
    ++first_roll_position;

    if (first_roll_position >= static_cast<int>(first_roll_candidates.size())) {
        resolve_first_roll_round();
    }
}

void Game::resolve_first_roll_round()
{
    if (first_roll_candidates.empty()) return;

    int highest = -1;
    for (int index : first_roll_candidates) {
        highest = std::max(highest, first_roll_results[static_cast<std::size_t>(index)]);
    }

    std::vector<int> leaders;
    for (int index : first_roll_candidates) {
        if (first_roll_results[static_cast<std::size_t>(index)] == highest) {
            leaders.push_back(index);
        }
    }

    if (leaders.size() == 1) {
        turn_manager.set_current_player_index(static_cast<std::size_t>(leaders.front()));
        first_roll_candidates.clear();
        first_roll_position = 0;
        state = GameState::WaitingRoll;
        return;
    }

    // Tie for the highest roll: ONLY tied players continue.
    first_roll_candidates = leaders;
    first_roll_position = 0;
    ++first_roll_round;
    first_roll_has_result = false;
    for (int index : first_roll_candidates) {
        first_roll_results[static_cast<std::size_t>(index)] = 0;
    }
    turn_manager.get_dice().reset();
}

void Game::handle_event(const SDL_Event* event, SDL_Renderer* renderer)
{
    if (!event) return;

    // Keep the SDL window that owns the current input event so text input
    // is activated for the actual game window.
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event->type == SDL_EVENT_MOUSE_BUTTON_UP ||
        event->type == SDL_EVENT_MOUSE_MOTION) {
        const Uint32 window_id = (event->type == SDL_EVENT_MOUSE_MOTION)
            ? event->motion.windowID : event->button.windowID;
        if (window_id != 0) text_input_window = SDL_GetWindowFromID(window_id);
    } else if (event->type == SDL_EVENT_TEXT_INPUT && event->text.windowID != 0) {
        text_input_window = SDL_GetWindowFromID(event->text.windowID);
    }

    if (event->type == SDL_EVENT_QUIT) {
        is_running = false;
    }
    else if (event->type == SDL_EVENT_TEXT_INPUT && scene == SceneState::NameEntry && name_typing) {
        for (const char* c = event->text.text; c && *c; ++c) {
            unsigned char ch = static_cast<unsigned char>(*c);
            if (ch >= 32 && ch != 127 && name_input.size() < 18) {
                name_input.push_back(*c);
            }
        }
    }
    else if (event->type == SDL_EVENT_TEXT_INPUT && state == GameState::AuctionMenu && auction_typing) {
        for (const char* c = event->text.text; c && *c; ++c) {
            if (*c >= '0' && *c <= '9') auction_bid_input.push_back(*c);
        }
        if (auction_bid_input.size() > 7) auction_bid_input.resize(7);
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && scene == SceneState::NameEntry && name_typing) {
        if (event->key.key == SDLK_BACKSPACE) {
            if (!name_input.empty()) name_input.pop_back();
        } else if (event->key.key == SDLK_RETURN) {
            if (!name_input.empty()) {
                pending_player_names[static_cast<std::size_t>(editing_name_index)] = name_input;
                if (editing_name_index + 1 < selected_player_count) {
                    ++editing_name_index;
                    name_input = pending_player_names[static_cast<std::size_t>(editing_name_index)];
                } else {
                    name_typing = false;
                    if (text_input_window) SDL_StopTextInput(text_input_window);
                }
            }
        } else if (event->key.key == SDLK_ESCAPE) {
            name_typing = false;
            if (text_input_window) SDL_StopTextInput(text_input_window);
            scene = SceneState::MainMenu;
        }
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && state == GameState::AuctionMenu && auction_typing) {
        if (event->key.key == SDLK_BACKSPACE && !auction_bid_input.empty()) {
            auction_bid_input.pop_back();
        } else if (event->key.key == SDLK_RETURN) {
            try {
                int value = std::stoi(auction_bid_input);
                Player* bidder = (auction_current_player >= 0 && auction_current_player < static_cast<int>(players.size()))
                    ? &players[static_cast<size_t>(auction_current_player)] : nullptr;
                if (bidder && value > auction_bid && bidder->can_pay(value)) {
                    auction_bid = value;
                    auction_highest_player = auction_current_player;
                    auction_passed[static_cast<size_t>(auction_current_player)] = false;
                    auction_typing = false;
                    SDL_StopTextInput(text_input_window);
                    auction_bid_input.clear();
                    advance_auction();
                }
            } catch (...) {}
        } else if (event->key.key == SDLK_ESCAPE) {
            auction_typing = false;
            auction_bid_input.clear();
            SDL_StopTextInput(text_input_window);
        }
    }
    else if (event->type == SDL_EVENT_KEY_DOWN && scene == SceneState::MainGame) {
        if (event->key.key == SDLK_ESCAPE && state != GameState::GameOver && state != GameState::FirstRoll) {
            if (state == GameState::Paused) {
                state = state_before_pause;
            } else {
                state_before_pause = state;
                state = GameState::Paused;
            }
        }
        else if (state != GameState::GameOver && event->key.key == SDLK_F5) {
            save_game("monopoly_save.txt");
        }
        else if (state != GameState::GameOver && event->key.key == SDLK_F9) {
            load_game("monopoly_save.txt");
        }
        else if (state == GameState::BankruptcyMenu) {
            Player* bankruptcy_player = turn_manager.get_current_player();
            if (bankruptcy_player && bankruptcy_player->has_pending_debt()) {
                const int visible = 9;
                const int property_count = static_cast<int>(bankruptcy_player->get_properties().size());
                // SDL3: use scancode for physical arrow keys. This is independent
                // of keyboard layout and is more reliable than comparing keycodes.
                if (event->key.scancode == SDL_SCANCODE_UP) {
                    if (bankruptcy_scroll > 0) --bankruptcy_scroll;
                }
                else if (event->key.scancode == SDL_SCANCODE_DOWN) {
                    if (bankruptcy_scroll + visible < property_count) ++bankruptcy_scroll;
                }
            }
        }
        else if (state == GameState::ManagingHouses) {
            if (event->key.scancode == SDL_SCANCODE_UP) {
                if (build_scroll > 0) --build_scroll;
            }
            else if (event->key.scancode == SDL_SCANCODE_DOWN) {
                Player* p = turn_manager.get_current_player();
                if (p && build_scroll + 10 < static_cast<int>(p->get_properties().size())) ++build_scroll;
            }
            else if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                state = GameState::PlayingField;
            }
        }
        else if (state == GameState::ExchangeMenu) {
            if (event->key.scancode == SDL_SCANCODE_UP) {
                if (exchange_keyboard_right) {
                    if (exchange_receive_scroll > 0) --exchange_receive_scroll;
                } else if (exchange_give_scroll > 0) {
                    --exchange_give_scroll;
                }
            }
            else if (event->key.scancode == SDL_SCANCODE_DOWN) {
                Player* p = turn_manager.get_current_player();
                if (p) {
                    if (exchange_keyboard_right) {
                        if (exchange_target && exchange_receive_scroll + 7 < static_cast<int>(exchange_target->get_properties().size())) ++exchange_receive_scroll;
                    } else if (exchange_give_scroll + 7 < static_cast<int>(p->get_properties().size())) {
                        ++exchange_give_scroll;
                    }
                }
            }
            else if (event->key.scancode == SDL_SCANCODE_LEFT) {
                exchange_keyboard_right = false;
            }
            else if (event->key.scancode == SDL_SCANCODE_RIGHT) {
                exchange_keyboard_right = true;
            }
            else if (event->key.scancode == SDL_SCANCODE_ESCAPE) {
                reset_exchange();
                state = GameState::PlayingField;
            }
        }
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        float mouse_x = event->button.x;
        float mouse_y = event->button.y;

        if (scene == SceneState::MainMenu) {
            if (is_point_in_rect(mouse_x, mouse_y, btn_2_players)) selected_player_count = 2;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_3_players)) selected_player_count = 3;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_4_players)) selected_player_count = 4;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_5_players)) selected_player_count = 5;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_6_players)) selected_player_count = 6;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_7_players)) selected_player_count = 7;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_8_players)) selected_player_count = 8;
            else if (is_point_in_rect(mouse_x, mouse_y, btn_start)) begin_name_entry();
        }
        else if (scene == SceneState::NameEntry) {
            SDL_FRect name_field{ 270.0f, 165.0f, 740.0f, 70.0f };
            if (is_point_in_rect(mouse_x, mouse_y, name_field)) {
                name_typing = true;
                if (text_input_window) SDL_StartTextInput(text_input_window);
            }
            else if (is_point_in_rect(mouse_x, mouse_y, btn_name_back)) {
                name_typing = false;
                if (text_input_window) SDL_StopTextInput(text_input_window);
                scene = SceneState::MainMenu;
            }
            else if (is_point_in_rect(mouse_x, mouse_y, btn_name_start)) {
                if (pending_player_names.size() == static_cast<std::size_t>(selected_player_count)) {
                    if (!name_input.empty() && editing_name_index >= 0 && editing_name_index < selected_player_count) {
                        pending_player_names[static_cast<std::size_t>(editing_name_index)] = name_input;
                    }
                    bool all_named = true;
                    for (const auto& n : pending_player_names) if (n.empty()) all_named = false;
                    if (all_named) {
                        name_typing = false;
                        if (text_input_window) SDL_StopTextInput(text_input_window);
                        setup(selected_player_count, pending_player_names);
                    }
                }
            }
            else {
                const float row_y = 285.0f;
                const float row_h = 54.0f;
                for (int i = 0; i < selected_player_count; ++i) {
                    const int col = i % 2;
                    const int row = i / 2;
                    SDL_FRect r{ 170.0f + col * 470.0f, row_y + row * row_h, 430.0f, 44.0f };
                    if (is_point_in_rect(mouse_x, mouse_y, r)) {
                        pending_player_names[static_cast<std::size_t>(editing_name_index)] = name_input;
                        editing_name_index = i;
                        name_input = pending_player_names[static_cast<std::size_t>(editing_name_index)];
                        name_typing = true;
                        if (text_input_window) SDL_StartTextInput(text_input_window);
                        break;
                    }
                }
            }
        }
        else if (scene == SceneState::MainGame) {
            Player* current = turn_manager.get_current_player();

            if (state == GameState::Paused) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_pause_resume)) {
                    state = state_before_pause;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_pause_save)) {
                    save_game("monopoly_save.txt");
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_pause_load)) {
                    if (load_game("monopoly_save.txt")) state = GameState::Paused;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_pause_menu)) {
                    scene = SceneState::MainMenu;
                    state = GameState::Setup;
                    state_before_pause = GameState::PlayingField;
                    is_setup = false;
                    reset_exchange();
                    auction_property = nullptr;
                    auction_current_player = -1;
                    auction_highest_player = -1;
                    auction_bid = 0;
                    auction_bid_input.clear();
                    auction_typing = false;
                    auction_passed.clear();
                }
            }
            else if (state == GameState::FirstRoll) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_first_roll)) {
                    roll_for_starting_player();
                }
            }
            else if (state == GameState::JailNotification) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_jail_notification_ok) && current) {
                    Board* board = turn_manager.get_board();
                    const int jail = board ? board->find(FieldType::Prison, current->get_location()) : 10;
                    current->set_location(jail >= 0 ? jail : 10);
                    current->set_in_prison(true);
                    current->set_prison_score(3);
                    pending_jail = false;
                    state = GameState::PlayingField;
                }
            }
            else if (state == GameState::GameOver) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_game_over_menu)) {
                    scene = SceneState::MainMenu;
                    state = GameState::Setup;
                    is_setup = false;
                    reset_exchange();
                    auction_property = nullptr;
                    auction_passed.clear();
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_game_over_quit)) {
                    is_running = false;
                }
            }
            else if (state == GameState::BankruptcyMenu) {
                if (!current || !current->has_pending_debt()) return;

                const float list_x = 210.0f;
                const float list_y = 195.0f;
                const float row_h = 43.0f;
                const int visible = 9;
                const auto& props = current->get_properties();
                Board* board = turn_manager.get_board();

                if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{920.0f, 185.0f, 90.0f, 55.0f})) {
                    if (bankruptcy_scroll > 0) --bankruptcy_scroll;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{920.0f, 605.0f, 90.0f, 55.0f})) {
                    if (bankruptcy_scroll + visible < static_cast<int>(props.size())) ++bankruptcy_scroll;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_bankruptcy_pay)) {
                    if (current->can_pay(current->get_pending_debt())) {
                        finish_pending_payment();
                    }
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_bankruptcy_declare)) {
                    // Bankruptcy can only be declared after no further
                    // legal liquidation is possible.
                    bool can_raise_more = false;
                    for (PropertyField* property : props) {
                        if (!property) continue;
                        if (auto* street = dynamic_cast<StreetField*>(property)) {
                            if (street->has_hotel() || street->get_house_val() > 0) {
                                if (board && current->can_sell_house(street, *board)) can_raise_more = true;
                            } else if (!property->is_mortgaged()) {
                                can_raise_more = true; // can be mortgaged
                            }
                        } else if (!property->is_mortgaged()) {
                            can_raise_more = true;
                        }
                        if (can_raise_more) break;
                    }
                    if (current->get_balance() < current->get_pending_debt() && !can_raise_more) {
                        current->declare_bankruptcy(current->get_pending_creditor());
                        state = GameState::PlayingField;
                    }
                }
                else {
                    for (int visible_row = 0; visible_row < visible; ++visible_row) {
                        const int index = bankruptcy_scroll + visible_row;
                        if (index >= static_cast<int>(props.size())) break;

                        PropertyField* property = props[static_cast<size_t>(index)];
                        if (!property) continue;

                        const float y = list_y + visible_row * row_h;
                        SDL_FRect action_rect{list_x + 500, y + 3, 185, 30};
                        if (!is_point_in_rect(mouse_x, mouse_y, action_rect)) continue;

                        if (auto* street = dynamic_cast<StreetField*>(property)) {
                            if (street->has_hotel() || street->get_house_val() > 0) {
                                if (board && current->can_sell_house(street, *board)) current->sell_house(street);
                            } else if (!property->is_mortgaged()) {
                                property->mortgage(current);
                            }
                        } else if (!property->is_mortgaged()) {
                            property->mortgage(current);
                        }

                        if (bankruptcy_scroll >= static_cast<int>(current->get_properties().size()) && bankruptcy_scroll > 0) {
                            bankruptcy_scroll = static_cast<int>(current->get_properties().size()) - 1;
                            if (bankruptcy_scroll < 0) bankruptcy_scroll = 0;
                        }
                        break;
                    }
                }
            }
            else if (state == GameState::ExchangeOffer) {
                if (!exchange_target || (!exchange_give && !exchange_receive && exchange_cash == 0)) return;

                SDL_FRect accept{ 380, 610, 220, 55 };
                SDL_FRect reject{ 680, 610, 220, 55 };

                if (is_point_in_rect(mouse_x, mouse_y, accept)) {
                    if (execute_exchange()) {
                        reset_exchange();
                        state = GameState::PlayingField;
                    }
                }
                else if (is_point_in_rect(mouse_x, mouse_y, reject)) {
                    reset_exchange();
                    state = GameState::PlayingField;
                }
            }
            else if (state == GameState::ExchangeMenu) {
                if (!current) return;
                const int visible = 7;
                const float list_y = 190.0f;
                const float row_h = 48.0f;

                float target_x = 310.0f;
                for (size_t i = 0; i < players.size(); ++i) {
                    Player* p = &players[i];
                    if (p == current || p->get_is_bankrupt()) continue;
                    SDL_FRect r{ target_x, 100.0f, 190.0f, 42.0f };
                    if (is_point_in_rect(mouse_x, mouse_y, r)) {
                        exchange_target = p;
                        exchange_keyboard_right = true;
                        exchange_receive = nullptr;
                        exchange_receive_scroll = 0;
                    }
                    target_x += 205.0f;
                }

                const auto& mine = current->get_properties();
                const int mine_count = static_cast<int>(mine.size());
                if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{595,190,40,34})) {
                    if (exchange_give_scroll > 0) --exchange_give_scroll;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{595,490,40,34})) {
                    if (exchange_give_scroll + visible < mine_count) ++exchange_give_scroll;
                }
                else if (exchange_target && is_point_in_rect(mouse_x, mouse_y, SDL_FRect{1125,190,40,34})) {
                    if (exchange_receive_scroll > 0) --exchange_receive_scroll;
                }
                else if (exchange_target && is_point_in_rect(mouse_x, mouse_y, SDL_FRect{1125,490,40,34})) {
                    if (exchange_receive_scroll + visible < static_cast<int>(exchange_target->get_properties().size())) ++exchange_receive_scroll;
                }
                else {
                    for (int row = 0; row < visible; ++row) {
                        int idx = exchange_give_scroll + row;
                        if (idx >= mine_count) break;
                        SDL_FRect r{120, list_y + row * row_h, 470, 40};
                        if (is_point_in_rect(mouse_x, mouse_y, r)) {
                            exchange_keyboard_right = false;
                            exchange_give = mine[static_cast<size_t>(idx)];
                            break;
                        }
                    }
                    if (exchange_target) {
                        const auto& theirs = exchange_target->get_properties();
                        for (int row = 0; row < visible; ++row) {
                            int idx = exchange_receive_scroll + row;
                            if (idx >= static_cast<int>(theirs.size())) break;
                            SDL_FRect r{650, list_y + row * row_h, 470, 40};
                            if (is_point_in_rect(mouse_x, mouse_y, r)) {
                                exchange_keyboard_right = true;
                                exchange_receive = theirs[static_cast<size_t>(idx)];
                                break;
                            }
                        }
                    }

                    if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{650,535,220,38})) {
                        exchange_receive = nullptr;
                    }
                    SDL_FRect minus{120,635,55,38}, plus{185,635,55,38}, offer{825,635,135,38}, close{975,635,135,38};
                    if (is_point_in_rect(mouse_x, mouse_y, minus)) exchange_cash -= 50;
                    else if (is_point_in_rect(mouse_x, mouse_y, plus)) exchange_cash += 50;
                    else if (is_point_in_rect(mouse_x, mouse_y, offer)) {
                        // At least one side must provide an asset or money.
                        if (exchange_target && (exchange_give || exchange_receive || exchange_cash != 0)) {
                            // Validate that the cash payer can afford the offer before presenting it.
                            const bool payer_ok = exchange_cash >= 0
                                ? exchange_target->can_pay(exchange_cash)
                                : current->can_pay(-exchange_cash);
                            if (payer_ok) {
                                exchange_offer_pending = true;
                                state = GameState::ExchangeOffer;
                            }
                        }
                    }
                    else if (is_point_in_rect(mouse_x, mouse_y, close)) {
                        reset_exchange();
                        state = GameState::PlayingField;
                    }
                }
            }
            else if (state == GameState::AuctionMenu) {
                if (!auction_property || auction_current_player < 0 ||
                    auction_current_player >= static_cast<int>(players.size())) return;

                Player* bidder = &players[static_cast<size_t>(auction_current_player)];
                SDL_FRect bid_input{320, 285, 480, 55};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bid_input);
    SDL_SetRenderDrawColor(renderer, auction_typing ? 52 : 120, auction_typing ? 152 : 120, auction_typing ? 219 : 120, 255);
    SDL_RenderRect(renderer, &bid_input);
    render_text(renderer, auction_bid_input.empty() ? "Enter amount" : auction_bid_input,
                {20,20,20,255}, bid_input, board_font);

    SDL_SetRenderDrawColor(renderer, auction_typing ? 30 : 52, auction_typing ? 180 : 152, auction_typing ? 90 : 219, 255);
    SDL_RenderFillRect(renderer, &btn_auction_exact_bid);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_auction_exact_bid);
    render_text(renderer, auction_typing ? "ENTER BID" : "TYPE EXACT BID",
                {255,255,255,255}, btn_auction_exact_bid, board_font);

    SDL_FRect b50{ 320, 365, 150, 55 };
                SDL_FRect b100{ 485, 365, 150, 55 };
                SDL_FRect b200{ 650, 365, 150, 55 };
                SDL_FRect pass{ 815, 365, 150, 55 };

                // The exact-bid control is a real button. Clicking it activates
                // SDL text input and opens a numeric entry mode.
                if (is_point_in_rect(mouse_x, mouse_y, btn_auction_exact_bid)) {
                    auction_typing = true;
                    auction_bid_input.clear();
                    SDL_StartTextInput(text_input_window);
                    return;
                }

                // Clicking directly into the text field also focuses it.
                SDL_FRect bid_input_field{320, 285, 480, 55};
                if (is_point_in_rect(mouse_x, mouse_y, bid_input_field)) {
                    auction_typing = true;
                    SDL_StartTextInput(text_input_window);
                    return;
                }

                int increment = 0;
                if (is_point_in_rect(mouse_x, mouse_y, b50)) increment = 50;
                else if (is_point_in_rect(mouse_x, mouse_y, b100)) increment = 100;
                else if (is_point_in_rect(mouse_x, mouse_y, b200)) increment = 200;
                else if (is_point_in_rect(mouse_x, mouse_y, pass)) {
                    auction_passed[static_cast<size_t>(auction_current_player)] = true;
                    advance_auction();
                    return;
                }

                if (increment > 0 && bidder->can_pay(auction_bid + increment)) {
                    auction_typing = false;
                    auction_bid_input.clear();
                    SDL_StopTextInput(text_input_window);
                    auction_bid += increment;
                    auction_highest_player = auction_current_player;
                    advance_auction();
                }
            }
            else if (state == GameState::JailMenu) {
                if (!current) return;

                if (is_point_in_rect(mouse_x, mouse_y, btn_jail_roll)) {
                    state = GameState::RollingDice;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_jail_pay)) {
                    if (current->can_pay(50)) {
                        current->remove_money(50, "Jail fee");
                        current->set_in_prison(false);
                        state = GameState::WaitingRoll;
                    }
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_jail_card)) {
                    if (current->get_get_out_from_jail_card() > 0) {
                        current->set_get_out_from_jail_card(-1);
                        if (turn_manager.get_board()) turn_manager.get_board()->return_jail_free_card();
                        current->set_in_prison(false);
                        state = GameState::WaitingRoll;
                    }
                }
            }
            else if (state == GameState::BuyingProperty) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_modal_buy)) {
                    if (current) {
                        Field* field = turn_manager.get_board()->get_field_on(current->get_location());
                        auto* prop = dynamic_cast<PropertyField*>(field);
                        if (prop && current->can_pay(prop->get_price())) {
                            prop->buy(current);
                        }
                    }
                    state = GameState::PlayingField;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_modal_pass)) {
                    if (current) {
                        Field* field = turn_manager.get_board()->get_field_on(current->get_location());
                        auto* prop = dynamic_cast<PropertyField*>(field);
                        if (prop && !prop->get_owner()) {
                            start_auction(prop);
                            state = GameState::AuctionMenu;
                        } else {
                            state = GameState::PlayingField;
                        }
                    } else {
                        state = GameState::PlayingField;
                    }
                }
            }
            else if (state == GameState::WaitingRoll) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_roll_dice)) {
                    if (current && current->is_in_prison()) {
                        state = GameState::JailMenu;
                    }
                    else {
                        state = GameState::RollingDice;
                    }
                }
            }
            else if (state == GameState::PlayingField) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_roll_dice)) {
                    if (turn_manager.can_roll_again()) {
                        // Дубль даёт дополнительный бросок тому же игроку.
                        state = GameState::RollingDice;
                    }
                    else {
                        turn_manager.change_current_player();
                        Player* next_p = turn_manager.get_current_player();
                        if (next_p && next_p->is_in_prison()) {
                            state = GameState::JailMenu;
                        }
                        else {
                            state = GameState::WaitingRoll;
                        }
                    }
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_manage_houses)) {
                    state = GameState::ManagingHouses;
                }
                else if (is_point_in_rect(mouse_x, mouse_y, btn_exchange)) {
                    reset_exchange();
                    state = GameState::ExchangeMenu;
                }
            }
            else if (state == GameState::UsingCardUI) {
                if (is_point_in_rect(mouse_x, mouse_y, btn_card_ok)) {
                    state = GameState::UsingCard;
                }
            }
            else if (state == GameState::ManagingHouses) {
                Board* board = turn_manager.get_board();
                SDL_FRect modal{170, 45, 940, 700};
                const float list_y = modal.y + 95;
                const float row_h = 52.0f;
                const int visible = 10;

                if (current && board) {
                    const auto& props = current->get_properties();
                    if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{modal.x + 840, modal.y + 85, 65, 55})) {
                        if (build_scroll > 0) --build_scroll;
                    }
                    else if (is_point_in_rect(mouse_x, mouse_y, SDL_FRect{modal.x + 840, modal.y + 605, 65, 55})) {
                        if (build_scroll + visible < static_cast<int>(props.size())) ++build_scroll;
                    }
                    else {
                        for (int row = 0; row < visible; ++row) {
                            const int index = build_scroll + row;
                            if (index >= static_cast<int>(props.size())) break;
                            PropertyField* property = props[static_cast<size_t>(index)];
                            if (!property) continue;
                            const float y = list_y + row * row_h;

                            SDL_FRect plus{modal.x + 505, y + 7, 42, 32};
                            SDL_FRect minus{modal.x + 553, y + 7, 42, 32};
                            SDL_FRect mort{modal.x + 610, y + 7, 125, 32};

                            if (auto* street = dynamic_cast<StreetField*>(property)) {
                                if (is_point_in_rect(mouse_x, mouse_y, plus)) {
                                    if (current->can_build_house(street, *board, board->count_houses(), board->count_hotels())) {
                                        if (current->remove_money(street->get_house_price(), "House purchased")) {
                                            street->build_house();
                                        }
                                    }
                                    break;
                                }
                                if (is_point_in_rect(mouse_x, mouse_y, minus)) {
                                    if (current->can_sell_house(street, *board)) current->sell_house(street);
                                    break;
                                }
                            }
                            if (is_point_in_rect(mouse_x, mouse_y, mort)) {
                                if (property->is_mortgaged()) {
                                    property->unmortgage(current);
                                } else if (current->can_sell_property(property, *board)) {
                                    property->mortgage(current);
                                }
                                break;
                            }
                        }
                    }
                }

                SDL_FRect btn_close = { modal.x + 380, modal.y + 655, 140, 35 };
                if (is_point_in_rect(mouse_x, mouse_y, btn_close)) {
                    state = GameState::PlayingField;
                }
            }
        }
    }
}

void Game::setup(int player_count, const std::vector<std::string>& names)
{
    if (player_count < 2 || player_count > 8 ||
        names.size() < static_cast<std::size_t>(player_count)) {
        is_setup = false;
        state = GameState::Setup;
        return;
    }

    players.clear();
    players.reserve(static_cast<std::size_t>(player_count));

    for (int i = 0; i < player_count; ++i) {
        const std::string player_name = names[static_cast<std::size_t>(i)].empty()
            ? "Player " + std::to_string(i + 1)
            : names[static_cast<std::size_t>(i)];

        players.emplace_back(player_name);
        players.back().add_money(1500, "Starting cash");
    }

    CardContext context{ &players, this, nullptr };
    turn_manager.setup(context);
    context.board = turn_manager.get_board();

    // Board::setup() created the card fields with the same context object,
    // so the decks already point at the current player list and board.
    turn_manager.set_players(players);

    scene = SceneState::MainGame;
    is_setup = true;
    state_before_pause = GameState::PlayingField;
    reset_money_tracking();
    pending_jail = false;
    reset_exchange();
    auction_property = nullptr;
    auction_current_player = -1;
    auction_highest_player = -1;
    auction_bid = 0;
    auction_bid_input.clear();
    auction_typing = false;
    auction_passed.clear();
    bankruptcy_scroll = 0;
    build_scroll = 0;
    exchange_give_scroll = 0;
    exchange_receive_scroll = 0;
    exchange_keyboard_right = false;

    // Fully initialize the first-roll state. Merely setting GameState::FirstRoll
    // is not enough because the candidate list and round counters must exist
    // before the first frame is rendered.
    begin_first_roll();
}

void Game::open_bankruptcy_menu()
{
    Player* current = turn_manager.get_current_player();
    if (!current || !current->has_pending_debt() || current->get_is_bankrupt()) return;

    // update() is called every frame while the debt is pending. Do not
    // reset the scroll position if the bankruptcy menu is already open,
    // otherwise the Up/Down buttons appear to do nothing.
    if (state != GameState::BankruptcyMenu) {
        bankruptcy_scroll = 0;
        state = GameState::BankruptcyMenu;
    }
}

void Game::finish_pending_payment()
{
    Player* current = turn_manager.get_current_player();
    if (!current || !current->has_pending_debt()) return;

    if (current->settle_pending_debt()) {
        if (current->get_pending_jail_release()) {
            const int steps = current->get_pending_move_after_debt();
            current->set_pending_jail_release(false);
            current->set_in_prison(false);
            current->move_steps(steps);
            turn_manager.turn(current, turn_manager.get_dice(), state);
        }
        state = GameState::PlayingField;
    }
}

void Game::draw_bankruptcy_menu(SDL_Renderer* renderer)
{
    Player* current = turn_manager.get_current_player();
    Board* board = turn_manager.get_board();
    if (!current || !board || !current->has_pending_debt()) return;

    const SDL_Color white{255, 255, 255, 255};
    const SDL_Color black{20, 20, 20, 255};
    const SDL_Color green{46, 204, 113, 255};
    const SDL_Color red{231, 76, 60, 255};
    const SDL_Color gray{120, 120, 120, 255};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 205);
    SDL_FRect overlay{0, 0, 1280, 820};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 248, 248, 242, 255);
    SDL_RenderFillRect(renderer, &modal_bankruptcy_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_bankruptcy_rect);

    SDL_FRect title{modal_bankruptcy_rect.x, modal_bankruptcy_rect.y + 10,
                    modal_bankruptcy_rect.w, 45};
    render_text(renderer, "PAYMENT REQUIRED - SELL ASSETS", red, title, font);

    const int debt = current->get_pending_debt();
    const int balance = current->get_balance();
    const int missing = std::max(0, debt - balance);
    render_text(renderer, "Debt: $" + std::to_string(debt), black,
                {210, 125, 270, 40}, font);
    render_text(renderer, "Cash: $" + std::to_string(balance),
                balance >= debt ? green : red,
                {500, 125, 250, 40}, font);
    render_text(renderer, "Still needed: $" + std::to_string(missing),
                missing == 0 ? green : red,
                {755, 125, 300, 40}, font);

    render_text(renderer,
                "Choose what to sell yourself. Buildings must be sold before the property.",
                black, {205, 155, 750, 30}, board_font);

    const float list_x = 210.0f;
    const float list_y = 195.0f;
    const float row_h = 43.0f;
    const int visible = 9;
    const auto& props = current->get_properties();

    for (int visible_row = 0; visible_row < visible; ++visible_row) {
        const int index = bankruptcy_scroll + visible_row;
        if (index >= static_cast<int>(props.size())) break;

        PropertyField* property = props[static_cast<size_t>(index)];
        if (!property) continue;

        const float y = list_y + visible_row * row_h;
        SDL_FRect row{list_x, y, 700, 36};
        SDL_SetRenderDrawColor(renderer, 232, 232, 226, 255);
        SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &row);

        std::string action = "MORTGAGE";
        int value = property->get_mortgage_value();
        bool can_sell = !property->is_mortgaged();

        if (auto* street = dynamic_cast<StreetField*>(property)) {
            if (street->has_hotel() || street->get_house_val() > 0) {
                action = street->has_hotel() ? "SELL HOTEL" : "SELL HOUSE";
                value = street->has_hotel() ? street->get_house_price() * 5 / 2 : street->get_house_sale_value();
                can_sell = current->can_sell_house(street, *board);
            }
        }
        if (property->is_mortgaged()) {
            action = "MORTGAGED";
            can_sell = false;
            value = 0;
        }

        render_text(renderer, property->get_name(), black,
                    {list_x + 12, y, 350, 36}, board_font);
        render_text(renderer, "$" + std::to_string(value), black,
                    {list_x + 365, y, 110, 36}, board_font);

        SDL_FRect action_rect{list_x + 500, y + 3, 185, 30};
        SDL_SetRenderDrawColor(renderer,
            can_sell ? green.r : gray.r,
            can_sell ? green.g : gray.g,
            can_sell ? green.b : gray.b, 255);
        SDL_RenderFillRect(renderer, &action_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &action_rect);
        render_text(renderer, can_sell ? action : "SELL IN ORDER",
                    white, action_rect, board_font);
    }

    // Scroll controls.
    SDL_SetRenderDrawColor(renderer, bankruptcy_scroll > 0 ? green.r : gray.r,
                           bankruptcy_scroll > 0 ? green.g : gray.g,
                           bankruptcy_scroll > 0 ? green.b : gray.b, 255);
    SDL_RenderFillRect(renderer, &btn_bankruptcy_up);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_bankruptcy_up);
    render_text(renderer, "^", white, btn_bankruptcy_up, font);

    const bool can_scroll_down = bankruptcy_scroll + visible < static_cast<int>(props.size());
    SDL_SetRenderDrawColor(renderer, can_scroll_down ? green.r : gray.r,
                           can_scroll_down ? green.g : gray.g,
                           can_scroll_down ? green.b : gray.b, 255);
    SDL_RenderFillRect(renderer, &btn_bankruptcy_down);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_bankruptcy_down);
    render_text(renderer, "v", white, btn_bankruptcy_down, font);

    const bool can_pay_now = balance >= debt;
    SDL_SetRenderDrawColor(renderer, can_pay_now ? green.r : gray.r,
                           can_pay_now ? green.g : gray.g,
                           can_pay_now ? green.b : gray.b, 255);
    SDL_RenderFillRect(renderer, &btn_bankruptcy_pay);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_bankruptcy_pay);
    render_text(renderer, "PAY DEBT", white, btn_bankruptcy_pay, font);

    bool can_raise_more = false;
    for (PropertyField* property : props) {
        if (!property) continue;
        if (auto* street = dynamic_cast<StreetField*>(property)) {
            if (street->has_hotel() || street->get_house_val() > 0) {
                if (current->can_sell_house(street, *board)) can_raise_more = true;
            } else if (!property->is_mortgaged()) can_raise_more = true;
        } else if (!property->is_mortgaged()) can_raise_more = true;
        if (can_raise_more) break;
    }
    const bool can_declare = balance < debt && !can_raise_more;
    SDL_SetRenderDrawColor(renderer, can_declare ? red.r : gray.r,
                           can_declare ? red.g : gray.g,
                           can_declare ? red.b : gray.b, 255);
    SDL_RenderFillRect(renderer, &btn_bankruptcy_declare);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_bankruptcy_declare);
    render_text(renderer, "DECLARE BANKRUPTCY", white,
                btn_bankruptcy_declare, board_font);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::reset_money_tracking()
{
    observed_balances.clear();
    observed_balances.reserve(players.size());
    for (const Player& player : players) {
        observed_balances.push_back(player.get_balance());
    }
    money_notifications.clear();
}

void Game::sync_money_notifications()
{
    if (observed_balances.size() != players.size()) {
        reset_money_tracking();
        return;
    }

    std::vector<int> deltas(players.size(), 0);
    const size_t first_notification = money_notifications.size();

    for (size_t i = 0; i < players.size(); ++i) {
        const int current_balance = players[i].get_balance();
        deltas[i] = current_balance - observed_balances[i];
        if (deltas[i] != 0) {
            MoneyNotification notification;
            notification.player_index = static_cast<int>(i);
            notification.amount = deltas[i];
            notification.reason = players[i].get_last_money_reason();
            if (notification.reason.empty()) {
                notification.reason = deltas[i] > 0 ? "Money received" : "Payment";
            }
            money_notifications.push_back(std::move(notification));
        }
        observed_balances[i] = current_balance;
    }

    // Pair simultaneous outgoing/incoming balances so a transfer can be
    // displayed as "Player A -> Player B" instead of two unrelated popups.
    for (size_t i = first_notification; i < money_notifications.size(); ++i) {
        MoneyNotification& outgoing = money_notifications[i];
        if (outgoing.amount >= 0 || outgoing.hidden) continue;

        const bool is_transfer = outgoing.reason.find("Rent") != std::string::npos ||
                                 outgoing.reason.find("Trade") != std::string::npos ||
                                 outgoing.reason.find("Debt") != std::string::npos;
        if (!is_transfer) continue;

        for (size_t j = first_notification; j < money_notifications.size(); ++j) {
            MoneyNotification& incoming = money_notifications[j];
            if (i == j || incoming.hidden || incoming.amount != -outgoing.amount || incoming.amount <= 0) continue;

            const bool is_matching = incoming.reason.find("received") != std::string::npos ||
                                     incoming.reason.find("Rent") != std::string::npos ||
                                     incoming.reason.find("Debt") != std::string::npos ||
                                     incoming.reason.find("Trade") != std::string::npos;
            if (!is_matching) continue;

            outgoing.counterparty = incoming.player_index;
            incoming.hidden = true;
            break;
        }
    }

    if (money_notifications.size() > 8) {
        money_notifications.erase(money_notifications.begin(),
                                   money_notifications.end() - 8);
    }
}

void Game::update_money_notifications(float delta_time)
{
    for (auto& notification : money_notifications) {
        notification.timer += delta_time;
    }

    money_notifications.erase(
        std::remove_if(money_notifications.begin(), money_notifications.end(),
            [](const MoneyNotification& notification) {
                return notification.timer >= notification.lifetime;
            }),
        money_notifications.end());

    sync_money_notifications();
}

void Game::draw_money_notifications(SDL_Renderer* renderer)
{
    if (money_notifications.empty()) return;

    const float start_x = 830.0f;
    const float start_y = 415.0f;
    const float width = 390.0f;
    const float height = 62.0f;
    const float gap = 8.0f;

    const size_t begin = money_notifications.size() > 4
        ? money_notifications.size() - 4 : 0;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int row = 0;
    for (size_t i = begin; i < money_notifications.size(); ++i) {
        const MoneyNotification& n = money_notifications[i];
        if (n.hidden) continue;
        const float progress = std::clamp(n.timer / n.lifetime, 0.0f, 1.0f);
        const float fade = progress > 0.70f ? (1.0f - progress) / 0.30f : 1.0f;
        const float slide = progress < 0.15f ? (1.0f - progress / 0.15f) * 18.0f : 0.0f;

        SDL_FRect rect{
            start_x + slide,
            start_y + row * (height + gap),
            width,
            height
        };

        SDL_Color accent = n.amount >= 0
            ? SDL_Color{46, 204, 113, static_cast<Uint8>(235.0f * fade)}
            : SDL_Color{231, 76, 60, static_cast<Uint8>(235.0f * fade)};
        SDL_Color white{255, 255, 255, static_cast<Uint8>(255.0f * fade)};
        SDL_Color soft{235, 235, 235, static_cast<Uint8>(255.0f * fade)};

        SDL_SetRenderDrawColor(renderer, 30, 34, 36, static_cast<Uint8>(230.0f * fade));
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, accent.r, accent.g, accent.b, accent.a);
        SDL_RenderRect(renderer, &rect);

        std::string player_name = "Player";
        if (n.player_index >= 0 && n.player_index < static_cast<int>(players.size())) {
            player_name = players[static_cast<size_t>(n.player_index)].get_name();
        }

        const std::string amount_text = (n.amount >= 0 ? "+$" : "-$") +
                                         std::to_string(std::abs(n.amount));
        render_text(renderer, amount_text, accent,
                    {rect.x + 12.0f, rect.y + 5.0f, 120.0f, 28.0f}, font);
        std::string title = player_name;
        if (n.counterparty >= 0 && n.counterparty < static_cast<int>(players.size())) {
            title += "  ->  " + players[static_cast<size_t>(n.counterparty)].get_name();
        }
        render_text(renderer, title + "  •  " + n.reason,
                    soft,
                    {rect.x + 130.0f, rect.y + 7.0f, rect.w - 142.0f, 25.0f},
                    board_font);
        render_text(renderer,
                    "Balance: $" + std::to_string(players[static_cast<size_t>(n.player_index)].get_balance()),
                    white,
                    {rect.x + 130.0f, rect.y + 31.0f, rect.w - 142.0f, 22.0f},
                    board_font);
        ++row;
        if (row >= 4) break;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::update(float deltaTime)
{
    update_money_notifications(deltaTime);

    switch (scene) {
    case SceneState::MainMenu:
        break;
    case SceneState::NameEntry:
        break;
    case SceneState::MainGame:
        if (state != GameState::BankruptcyMenu && state != GameState::FirstRoll) {
            turn_manager.update(deltaTime, state);
        }

        // Any failed payment creates a pending debt. Do not automatically
        // liquidate assets: interrupt the game and let the player decide.
        if (state != GameState::GameOver) {
            Player* current = turn_manager.get_current_player();
            if (current && current->has_pending_debt() && !current->get_is_bankrupt()) {
                open_bankruptcy_menu();
            }
        }
        break;
    }
}


void Game::draw_build_menu(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_FRect overlay{0, 0, 1280, 820};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_FRect modal{170, 45, 940, 700};
    SDL_SetRenderDrawColor(renderer, 240, 240, 235, 255);
    SDL_RenderFillRect(renderer, &modal);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal);

    render_text(renderer, "PROPERTY MANAGEMENT", {0,0,0,255},
                {modal.x + 20, modal.y + 12, modal.w - 100, 40}, font);
    render_text(renderer, "Build/sell buildings, mortgage or redeem properties.", {60,60,60,255},
                {modal.x + 20, modal.y + 52, modal.w - 100, 28}, board_font);

    Player* current = turn_manager.get_current_player();
    Board* board = turn_manager.get_board();
    if (!current || !board) return;

    const auto& props = current->get_properties();
    const float list_y = modal.y + 95;
    const float row_h = 52.0f;
    const int visible = 10;

    auto button = [&](const SDL_FRect& r, SDL_Color c, const std::string& label) {
        SDL_SetRenderDrawColor(renderer, c.r,c.g,c.b,c.a);
        SDL_RenderFillRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderRect(renderer,&r);
        render_text(renderer,label,{255,255,255,255},r,board_font);
    };

    for (int row = 0; row < visible; ++row) {
        const int index = build_scroll + row;
        if (index >= static_cast<int>(props.size())) break;
        PropertyField* property = props[static_cast<size_t>(index)];
        if (!property) continue;

        const float y = list_y + row * row_h;
        SDL_FRect bg{modal.x + 25, y, 870, 45};
        SDL_SetRenderDrawColor(renderer, 230, 230, 225, 255);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &bg);

        std::string status;
        bool can_build = false;
        bool can_sell = false;
        if (auto* street = dynamic_cast<StreetField*>(property)) {
            if (street->has_hotel()) status = "HOTEL";
            else status = std::to_string(street->get_house_val()) + " houses";
            can_build = current->can_build_house(street, *board, board->count_houses(), board->count_hotels());
            can_sell = current->can_sell_house(street, *board);
        } else {
            status = property->get_type() == FieldType::Railroad ? "RAILROAD" : "UTILITY";
        }
        if (property->is_mortgaged()) status += "  [MORTGAGED]";

        render_text(renderer, property->get_name(), {20,20,20,255},
                    {modal.x + 40, y + 3, 270, 38}, board_font);
        render_text(renderer, status, property->is_mortgaged() ? SDL_Color{192,57,43,255} : SDL_Color{60,60,60,255},
                    {modal.x + 315, y + 3, 180, 38}, board_font);

        SDL_FRect plus{modal.x + 505, y + 7, 42, 32};
        SDL_FRect minus{modal.x + 553, y + 7, 42, 32};
        button(plus, can_build ? SDL_Color{46,204,113,255} : SDL_Color{160,160,160,255}, "+");
        button(minus, can_sell ? SDL_Color{231,76,60,255} : SDL_Color{160,160,160,255}, "-");

        const bool can_mortgage = !property->is_mortgaged() && current->can_sell_property(property, *board);
        const bool can_redeem = property->is_mortgaged() && current->can_pay(property->get_mortgage_value() + (property->get_mortgage_value()*10 + 99)/100);
        SDL_FRect mort{modal.x + 610, y + 7, 125, 32};
        button(mort, (property->is_mortgaged() ? can_redeem : can_mortgage) ? SDL_Color{52,152,219,255} : SDL_Color{160,160,160,255},
               property->is_mortgaged() ? "REDEEM" : "MORTGAGE");

        render_text(renderer, "$" + std::to_string(property->get_mortgage_value()), {70,70,70,255},
                    {modal.x + 750, y + 3, 125, 38}, board_font);
    }

    SDL_FRect up{modal.x + 850, modal.y + 95, 45, 35};
    SDL_FRect down{modal.x + 850, modal.y + 615, 45, 35};
    const bool can_up = build_scroll > 0;
    const bool can_down = build_scroll + visible < static_cast<int>(props.size());
    button(up, can_up ? SDL_Color{52,152,219,255} : SDL_Color{160,160,160,255}, "^");
    button(down, can_down ? SDL_Color{52,152,219,255} : SDL_Color{160,160,160,255}, "v");

    render_text(renderer, props.empty() ? "0/0" : std::to_string(std::min(build_scroll + 1, static_cast<int>(props.size()))) + "/" + std::to_string(props.size()),
                {50,50,50,255}, {modal.x + 790, modal.y + 340, 55, 30}, board_font);

    SDL_FRect close{modal.x + 380, modal.y + 655, 140, 35};
    button(close,{100,100,100,255},"Close");
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
}

void Game::draw_jail_notification(SDL_Renderer* renderer)
{
    Player* current = turn_manager.get_current_player();
    if (!current) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_FRect overlay{ 0, 0, 1280, 820 };
    SDL_RenderFillRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 45, 52, 54, 255);
    SDL_RenderFillRect(renderer, &modal_jail_notification);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_jail_notification);

    render_text(renderer, "YOU ARE GOING TO JAIL",
                { 241, 196, 15, 255 },
                { modal_jail_notification.x, modal_jail_notification.y + 25,
                  modal_jail_notification.w, 55 }, font);
    render_text(renderer, current->get_name() + " must go to Prison.",
                { 255, 255, 255, 255 },
                { modal_jail_notification.x + 30, modal_jail_notification.y + 105,
                  modal_jail_notification.w - 60, 55 }, font);
    render_text(renderer, "Press OK to move the player to the jail cell.",
                { 220, 220, 220, 255 },
                { modal_jail_notification.x + 30, modal_jail_notification.y + 155,
                  modal_jail_notification.w - 60, 45 }, board_font);

    SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
    SDL_RenderFillRect(renderer, &btn_jail_notification_ok);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_jail_notification_ok);
    render_text(renderer, "OK", { 255, 255, 255, 255 }, btn_jail_notification_ok, font);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::draw_exchange_menu(SDL_Renderer* renderer)
{
    Player* current = turn_manager.get_current_player();
    if (!current) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_FRect overlay{0,0,1280,820};
    SDL_RenderFillRect(renderer,&overlay);
    SDL_SetRenderDrawColor(renderer,245,245,240,255);
    SDL_RenderFillRect(renderer,&modal_exchange_rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderRect(renderer,&modal_exchange_rect);

    render_text(renderer,"PROPERTY EXCHANGE",{20,20,20,255},
                {modal_exchange_rect.x,modal_exchange_rect.y+10,modal_exchange_rect.w,45},font);

    render_text(renderer,"Choose a player:",{20,20,20,255},{120,105,180,35},board_font);
    float target_x=310;
    for(size_t i=0;i<players.size();++i){
        Player* p=&players[i]; if(p==current||p->get_is_bankrupt()) continue;
        SDL_FRect r{target_x,100,190,42};
        SDL_Color c=(exchange_target==p)?SDL_Color{46,204,113,255}:SDL_Color{52,152,219,255};
        SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a); SDL_RenderFillRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderRect(renderer,&r);
        render_text(renderer,p->get_name(),{255,255,255,255},r,board_font); target_x+=205;
    }

    const float list_y=190; const float row_h=48; const int visible=7;
    render_text(renderer,"YOUR PROPERTY",{20,20,20,255},{120,155,400,30},board_font);
    render_text(renderer,"THEIR PROPERTY",{20,20,20,255},{650,155,400,30},board_font);

    const auto& mine=current->get_properties();
    for(int r=0;r<visible;++r){
        int idx=exchange_give_scroll+r; if(idx>=static_cast<int>(mine.size())) break;
        PropertyField* p=mine[static_cast<size_t>(idx)]; if(!p) continue;
        SDL_FRect rect{120,list_y+r*row_h,470,40};
        bool selected=exchange_give==p;
        SDL_SetRenderDrawColor(renderer,selected?46:230,selected?204:230,selected?113:230,255); SDL_RenderFillRect(renderer,&rect);
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderRect(renderer,&rect);
        render_text(renderer,p->get_name()+" ($"+std::to_string(p->get_price())+")",
                    selected?SDL_Color{255,255,255,255}:SDL_Color{20,20,20,255},rect,board_font);
    }

    if(exchange_target){
        const auto& theirs=exchange_target->get_properties();
        for(int r=0;r<visible;++r){
            int idx=exchange_receive_scroll+r; if(idx>=static_cast<int>(theirs.size())) break;
            PropertyField* p=theirs[static_cast<size_t>(idx)]; if(!p) continue;
            SDL_FRect rect{650,list_y+r*row_h,470,40};
            bool selected=exchange_receive==p;
            SDL_SetRenderDrawColor(renderer,selected?46:230,selected?204:230,selected?113:230,255); SDL_RenderFillRect(renderer,&rect);
            SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderRect(renderer,&rect);
            render_text(renderer,p->get_name()+" ($"+std::to_string(p->get_price())+")",
                        selected?SDL_Color{255,255,255,255}:SDL_Color{20,20,20,255},rect,board_font);
        }
    }

    auto button=[&](const SDL_FRect&r,SDL_Color c,const std::string&label){
        SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,c.a); SDL_RenderFillRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderRect(renderer,&r);
        render_text(renderer,label,{255,255,255,255},r,board_font);
    };

    SDL_FRect up_left{595,190,40,34}, down_left{595,490,40,34};
    SDL_FRect up_right{1125,190,40,34}, down_right{1125,490,40,34};
    button(up_left,exchange_give_scroll>0?SDL_Color{52,152,219,255}:SDL_Color{160,160,160,255},"^");
    button(down_left,exchange_give_scroll+visible<(int)mine.size()?SDL_Color{52,152,219,255}:SDL_Color{160,160,160,255},"v");
    const int their_count=exchange_target?static_cast<int>(exchange_target->get_properties().size()):0;
    button(up_right,exchange_receive_scroll>0?SDL_Color{52,152,219,255}:SDL_Color{160,160,160,255},"^");
    button(down_right,exchange_receive_scroll+visible<their_count?SDL_Color{52,152,219,255}:SDL_Color{160,160,160,255},"v");

    SDL_FRect no_property{650,535,220,38};
    button(no_property,exchange_receive==nullptr?SDL_Color{100,100,100,255}:SDL_Color{130,130,130,255},"No property");

    render_text(renderer,"Cash difference:",{20,20,20,255},{120,585,180,35},board_font);
    render_text(renderer,(exchange_cash>=0?"Their payment: $":"Your payment: $")+std::to_string(std::abs(exchange_cash)),
                {20,20,20,255},{280,585,260,35},board_font);

    SDL_FRect minus{120,635,55,38}, plus{185,635,55,38}, offer{825,635,135,38}, close{975,635,135,38};
    button(minus,{231,76,60,255},"-50"); button(plus,{46,204,113,255},"+50");
    button(offer,{52,152,219,255},"MAKE OFFER"); button(close,{100,100,100,255},"CLOSE");

    if(state==GameState::ExchangeOffer && exchange_target){
        SDL_SetRenderDrawColor(renderer,0,0,0,170); SDL_FRect ov{0,0,1280,820}; SDL_RenderFillRect(renderer,&ov);
        SDL_FRect m{250,170,780,440}; SDL_SetRenderDrawColor(renderer,45,52,54,255); SDL_RenderFillRect(renderer,&m);
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderRect(renderer,&m);
        render_text(renderer,"EXCHANGE OFFER",{241,196,15,255},{m.x,m.y+25,m.w,55},font);
        const std::string give_prop=exchange_give?exchange_give->get_name():"nothing";
        const std::string recv_prop=exchange_receive?exchange_receive->get_name():"nothing";
        const std::string cash_text=exchange_cash>=0?exchange_target->get_name()+" pays $"+std::to_string(exchange_cash):current->get_name()+" pays $"+std::to_string(-exchange_cash);
        render_text(renderer,current->get_name()+" gives: "+give_prop,{255,255,255,255},{m.x+40,m.y+110,m.w-80,45},board_font);
        render_text(renderer,exchange_target->get_name()+" gives: "+recv_prop,{255,255,255,255},{m.x+40,m.y+160,m.w-80,45},board_font);
        render_text(renderer,cash_text,{220,220,220,255},{m.x+40,m.y+215,m.w-80,45},board_font);
        SDL_FRect accept{380,610,220,55}, reject{680,610,220,55};
        button(accept,{46,204,113,255},"ACCEPT"); button(reject,{231,76,60,255},"REJECT");
    }
    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
}

void Game::draw_auction_menu(SDL_Renderer* renderer)
{
    Player* bidder = (auction_current_player >= 0 &&
                      auction_current_player < static_cast<int>(players.size()))
        ? &players[static_cast<size_t>(auction_current_player)] : nullptr;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 190);
    SDL_FRect overlay{ 0, 0, 1280, 820 };
    SDL_RenderFillRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 245, 245, 240, 255);
    SDL_RenderFillRect(renderer, &modal_auction_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &modal_auction_rect);

    render_text(renderer, "AUCTION",
                {20,20,20,255},
                {modal_auction_rect.x, modal_auction_rect.y + 15, modal_auction_rect.w, 45}, font);

    if (auction_property) {
        render_text(renderer, auction_property->get_name() +
                    "  |  Bank price: $" + std::to_string(auction_property->get_price()),
                    {20,20,20,255},
                    {modal_auction_rect.x + 35, modal_auction_rect.y + 80,
                     modal_auction_rect.w - 70, 40}, board_font);
    }

    std::string bid = "Current bid: $" + std::to_string(auction_bid);
    if (auction_highest_player >= 0) {
        bid += "  |  Leader: " + players[static_cast<size_t>(auction_highest_player)].get_name();
    } else {
        bid += "  |  No bids yet";
    }
    render_text(renderer, bid, {20,20,20,255},
                {modal_auction_rect.x + 35, modal_auction_rect.y + 125,
                 modal_auction_rect.w - 70, 45}, font);

    render_text(renderer, bidder ? ("Your bid, " + bidder->get_name() + ":") : "Auction",
                {20,20,20,255},
                {modal_auction_rect.x + 35, modal_auction_rect.y + 185,
                 modal_auction_rect.w - 70, 40}, board_font);

    SDL_FRect bid_input{320, 285, 645, 55};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bid_input);
    SDL_SetRenderDrawColor(renderer, auction_typing ? 52 : 0, auction_typing ? 152 : 0, auction_typing ? 219 : 0, 255);
    SDL_RenderRect(renderer, &bid_input);
    render_text(renderer, auction_bid_input.empty() ? "Type exact bid and press Enter" : auction_bid_input,
                {20,20,20,255}, bid_input, board_font);

    SDL_FRect b50{ 320, 365, 150, 55 };
    SDL_FRect b100{ 485, 365, 150, 55 };
    SDL_FRect b200{ 650, 365, 150, 55 };
    SDL_FRect pass{ 815, 365, 150, 55 };

    auto auction_button = [&](const SDL_FRect& r, int increment, const std::string& label) {
        bool enabled = bidder && bidder->can_pay(auction_bid + increment);
        SDL_Color c = enabled ? SDL_Color{46,204,113,255} : SDL_Color{150,150,150,255};
        SDL_SetRenderDrawColor(renderer, c.r,c.g,c.b,c.a);
        SDL_RenderFillRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
        SDL_RenderRect(renderer,&r);
        render_text(renderer,label,{255,255,255,255},r,board_font);
    };

    auction_button(b50, 50, "BID +$50");
    auction_button(b100, 100, "BID +$100");
    auction_button(b200, 200, "BID +$200");

    SDL_SetRenderDrawColor(renderer,231,76,60,255);
    SDL_RenderFillRect(renderer,&pass);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderRect(renderer,&pass);
    render_text(renderer,"PASS",{255,255,255,255},pass,board_font);

    int y = 455;
    for (size_t i = 0; i < players.size(); ++i) {
        std::string status = players[i].get_name();
        if (players[i].get_is_bankrupt()) status += " (bankrupt)";
        else if (auction_passed.size() > i && auction_passed[i]) status += " (passed)";
        if (static_cast<int>(i) == auction_highest_player) status += "  <- leader";
        render_text(renderer,status,{20,20,20,255},{320.0f,static_cast<float>(y),500.0f,32},board_font);
        y += 32;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::draw_game_over(SDL_Renderer* renderer)
{
    const Player* winner = nullptr;
    for (const Player& p : players) {
        if (!p.get_is_bankrupt()) {
            winner = &p;
            break;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 210);
    SDL_FRect overlay{0,0,1280,820};
    SDL_RenderFillRect(renderer,&overlay);

    SDL_SetRenderDrawColor(renderer,45,52,54,255);
    SDL_RenderFillRect(renderer,&modal_game_over_rect);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderRect(renderer,&modal_game_over_rect);

    render_text(renderer,"GAME OVER",
                {241,196,15,255},
                {modal_game_over_rect.x,modal_game_over_rect.y+30,modal_game_over_rect.w,60},font);

    if (winner) {
        render_text(renderer,"Winner: " + winner->get_name(),
                    {255,255,255,255},
                    {modal_game_over_rect.x+40,modal_game_over_rect.y+135,
                     modal_game_over_rect.w-80,55},font);
        render_text(renderer,"Cash: $" + std::to_string(winner->get_balance()),
                    {220,220,220,255},
                    {modal_game_over_rect.x+40,modal_game_over_rect.y+195,
                     modal_game_over_rect.w-80,45},board_font);
    } else {
        render_text(renderer,"No winner",
                    {255,255,255,255},
                    {modal_game_over_rect.x+40,modal_game_over_rect.y+150,
                     modal_game_over_rect.w-80,55},font);
    }

    SDL_SetRenderDrawColor(renderer,52,152,219,255);
    SDL_RenderFillRect(renderer,&btn_game_over_menu);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderRect(renderer,&btn_game_over_menu);
    render_text(renderer,"MAIN MENU",{255,255,255,255},btn_game_over_menu,font);

    SDL_SetRenderDrawColor(renderer,192,57,43,255);
    SDL_RenderFillRect(renderer,&btn_game_over_quit);
    SDL_SetRenderDrawColor(renderer,0,0,0,255);
    SDL_RenderRect(renderer,&btn_game_over_quit);
    render_text(renderer,"QUIT",{255,255,255,255},btn_game_over_quit,font);

    SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
}

void Game::reset_exchange()
{
    exchange_give = nullptr;
    exchange_receive = nullptr;
    exchange_target = nullptr;
    exchange_cash = 0;
    exchange_offer_pending = false;
    exchange_give_scroll = 0;
    exchange_receive_scroll = 0;
}

bool Game::execute_exchange()
{
    Player* current = turn_manager.get_current_player();
    if (!current || !exchange_target || exchange_target == current || exchange_target->get_is_bankrupt()) return false;
    if (!exchange_give && !exchange_receive && exchange_cash == 0) return false;

    if (exchange_give && !current->owns_property(exchange_give)) return false;
    if (exchange_receive && !exchange_target->owns_property(exchange_receive)) return false;
    if (exchange_cash > 0 && !exchange_target->can_pay(exchange_cash)) return false;
    if (exchange_cash < 0 && !current->can_pay(-exchange_cash)) return false;
    // A mortgaged property may be traded. The new owner must immediately
    // pay 10% of the mortgage value to the bank.
    const int mortgage_fee_to_bank = (exchange_receive && exchange_receive->is_mortgaged())
        ? (exchange_receive->get_mortgage_value() * 10 + 99) / 100 : 0;
    if (mortgage_fee_to_bank > 0 && !current->can_pay(mortgage_fee_to_bank)) return false;
    const int mortgage_fee_from_current = (exchange_give && exchange_give->is_mortgaged())
        ? (exchange_give->get_mortgage_value() * 10 + 99) / 100 : 0;
    if (mortgage_fee_from_current > 0 && !exchange_target->can_pay(mortgage_fee_from_current)) return false;

    // Validate that properties with buildings cannot be traded away.
    auto has_buildings = [](PropertyField* p) {
        if (auto* street = dynamic_cast<StreetField*>(p)) {
            return street->has_hotel() || street->get_house_val() > 0;
        }
        return false;
    };
    if (has_buildings(exchange_give) || has_buildings(exchange_receive)) return false;

    if (exchange_give) current->remove_property(exchange_give);
    if (exchange_receive) exchange_target->remove_property(exchange_receive);

    if (exchange_cash > 0) {
        exchange_target->remove_money(exchange_cash, "Trade payment");
        current->add_money(exchange_cash, "Trade received");
    } else if (exchange_cash < 0) {
        current->remove_money(-exchange_cash, "Trade payment");
        exchange_target->add_money(-exchange_cash, "Trade received");
    }

    if (mortgage_fee_to_bank > 0) current->remove_money(mortgage_fee_to_bank, "Mortgage transfer fee");
    if (mortgage_fee_from_current > 0) exchange_target->remove_money(mortgage_fee_from_current, "Mortgage transfer fee");

    if (exchange_give) { current->remove_property(exchange_give); exchange_target->add_property(exchange_give); }
    if (exchange_receive) { exchange_target->remove_property(exchange_receive); current->add_property(exchange_receive); }
    return true;
}

void Game::start_auction(PropertyField* property)
{
    if (!property || property->get_owner()) return;

    auction_property = property;
    auction_bid = 0;
    auction_bid_input.clear();
    auction_typing = false;
    auction_highest_player = -1;
    auction_passed.assign(players.size(), false);
    const int current_index = static_cast<int>(turn_manager.get_current_player_index());
    auction_current_player = current_index - 1;

    // Первый участник аукциона — игрок, который отказался от покупки.
    advance_auction();
}

void Game::advance_auction()
{
    if (auction_property == nullptr) return;

    int active = 0;
    for (size_t i = 0; i < players.size(); ++i) {
        if (!players[i].get_is_bankrupt() &&
            (auction_passed.size() <= i || !auction_passed[i])) {
            ++active;
        }
    }

    if (active <= 1 && auction_highest_player >= 0) {
        finish_auction();
        return;
    }
    if (active == 0) {
        finish_auction();
        return;
    }

    const int count = static_cast<int>(players.size());
    for (int step = 0; step < count; ++step) {
        auction_current_player = (auction_current_player + 1 + count) % count;
        if (!players[static_cast<size_t>(auction_current_player)].get_is_bankrupt() &&
            !auction_passed[static_cast<size_t>(auction_current_player)]) {
            return;
        }
    }

    finish_auction();
}

void Game::finish_auction()
{
    if (auction_property && auction_highest_player >= 0 &&
        auction_bid > 0) {
        Player& winner = players[static_cast<size_t>(auction_highest_player)];
        if (winner.can_pay(auction_bid)) {
            winner.remove_money(auction_bid, "Auction purchase");
            winner.add_property(auction_property);
        }
    }

    auction_property = nullptr;
    auction_current_player = -1;
    auction_highest_player = -1;
    auction_bid = 0;
    auction_bid_input.clear();
    auction_typing = false;
    auction_passed.clear();
    state = GameState::PlayingField;
}


bool Game::save_game(const std::string& path) const
{
    std::ofstream out(path);
    if (!out || players.empty()) return false;
    out << players.size() << "\n";
    out << turn_manager.get_current_player_index() << "\n";
    for (const Player& p : players) {
        out << p.get_name() << "\n";
        out << p.get_balance() << ' ' << p.get_location() << ' ' << p.get_prison_score() << ' '
            << p.is_in_prison() << ' ' << p.get_get_out_from_jail_card() << ' ' << p.get_is_bankrupt() << "\n";
        out << p.get_properties().size() << "\n";
        for (PropertyField* property : p.get_properties()) {
            int index = -1;
            for (int i = 0; i < turn_manager.get_board()->size(); ++i)
                if (turn_manager.get_board()->get_field_on(i) == property) { index = i; break; }
            auto* street = dynamic_cast<StreetField*>(property);
            out << index << ' ' << property->is_mortgaged();
            if (street) out << ' ' << street->get_house_val() << ' ' << street->has_hotel();
            else out << " 0 0";
            out << "\n";
        }
    }
    return static_cast<bool>(out);
}

bool Game::load_game(const std::string& path)
{
    std::ifstream in(path);
    if (!in) return false;
    std::size_t count = 0, current_index = 0;
    if (!(in >> count >> current_index) || count < 2 || count > 8) return false;
    std::string dummy; std::getline(in, dummy);
    players.clear(); players.reserve(count);
    struct SavedProperty { int index; bool mortgage; int houses; bool hotel; };
    struct SavedPlayer { std::string name; int balance, location, prison, card; bool jail, bankrupt; std::vector<SavedProperty> props; };
    std::vector<SavedPlayer> saved;
    saved.reserve(count);
    for (std::size_t i=0;i<count;++i) {
        SavedPlayer sp;
        if (!std::getline(in, sp.name)) return false;
        if (!(in >> sp.balance >> sp.location >> sp.prison >> sp.jail >> sp.card >> sp.bankrupt)) return false;
        std::size_t pc=0; if (!(in >> pc)) return false;
        sp.props.resize(pc);
        for (auto& x: sp.props) if (!(in >> x.index >> x.mortgage >> x.houses >> x.hotel)) return false;
        std::getline(in, dummy);
        saved.push_back(std::move(sp));
    }
    players.clear();
    for (const auto& sp : saved) {
        players.emplace_back(sp.name);
        players.back().add_money(sp.balance, "Loaded cash");
        players.back().set_location(sp.location);
        players.back().set_prison_score(sp.prison);
        players.back().set_in_prison(sp.jail);
        players.back().set_get_out_from_jail_card(sp.card);
        players.back().set_bankrupt_state(sp.bankrupt);
    }
    CardContext context{&players, this, nullptr};
    turn_manager.setup(context);
    context.board = turn_manager.get_board();
    turn_manager.set_players(players);
    turn_manager.set_current_player_index(current_index);
    Board* board = turn_manager.get_board();
    for (std::size_t i=0;i<saved.size();++i) {
        for (const auto& x : saved[i].props) {
            PropertyField* property = board->get_field_on(x.index) ? dynamic_cast<PropertyField*>(board->get_field_on(x.index)) : nullptr;
            if (!property) continue;
            players[i].add_property(property);
            property->set_mortgaged(x.mortgage);
            if (auto* street = dynamic_cast<StreetField*>(property)) {
                street->set_hotel(false); street->add_house(x.houses); if (x.hotel) street->set_hotel(true);
            }
        }
    }
    state = GameState::WaitingRoll;
    scene = SceneState::MainGame;
    is_setup = true;
    reset_money_tracking();
    return true;
}

void Game::draw_main_menu(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 18, 24, 38, 255);
    SDL_RenderClear(renderer);

    SDL_FRect panel{ 95, 75, 1090, 690 };
    SDL_SetRenderDrawColor(renderer, 28, 38, 58, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 73, 93, 120, 255);
    SDL_RenderRect(renderer, &panel);

    render_text(renderer, "MONOPOLY", {245, 210, 65, 255}, {180, 105, 920, 72}, font);
    render_text(renderer, "CHOOSE PLAYERS", {220, 230, 245, 255}, {250, 185, 780, 40}, board_font);

    const SDL_FRect buttons[] = { btn_2_players, btn_3_players, btn_4_players, btn_5_players, btn_6_players, btn_7_players, btn_8_players };
    const int counts[] = {2,3,4,5,6,7,8};
    for (int i = 0; i < 7; ++i) {
        const bool selected = selected_player_count == counts[i];
        SDL_SetRenderDrawColor(renderer,
            selected ? 52 : 43,
            selected ? 152 : 57,
            selected ? 219 : 79,
            255);
        SDL_RenderFillRect(renderer, &buttons[i]);
        SDL_SetRenderDrawColor(renderer, selected ? 245 : 83, selected ? 211 : 101, selected ? 115 : 124, 255);
        SDL_RenderRect(renderer, &buttons[i]);
        render_text(renderer, std::to_string(counts[i]) + " PLAYERS", {255,255,255,255}, buttons[i], font);
    }

    SDL_SetRenderDrawColor(renderer, 46, 204, 113, 255);
    SDL_RenderFillRect(renderer, &btn_start);
    render_text(renderer, "CONTINUE", {255,255,255,255}, btn_start, font);
}

void Game::draw_name_entry(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 15, 21, 34, 255);
    SDL_RenderClear(renderer);

    SDL_FRect panel{ 90, 50, 1100, 720 };
    SDL_SetRenderDrawColor(renderer, 27, 37, 56, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 73, 93, 120, 255);
    SDL_RenderRect(renderer, &panel);

    render_text(renderer, "PLAYER SETUP", {245,210,65,255}, {180, 85, 920, 58}, font);
    render_text(renderer, "Click a player and type a custom name. ENTER moves to the next player.", {220,230,245,255}, {180, 145, 920, 32}, board_font);

    SDL_FRect input{270,165,740,70};
    SDL_SetRenderDrawColor(renderer, name_typing ? 52 : 65, name_typing ? 152 : 75, name_typing ? 219 : 88, 255);
    SDL_RenderFillRect(renderer, &input);
    SDL_SetRenderDrawColor(renderer, 230,235,245,255);
    SDL_RenderRect(renderer, &input);
    render_text(renderer, name_input.empty() ? "Type player name..." : name_input,
                name_input.empty() ? SDL_Color{160,170,185,255} : SDL_Color{255,255,255,255}, input, font);

    const float row_y = 285.0f;
    const float row_h = 54.0f;
    for (int i = 0; i < selected_player_count; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        SDL_FRect r{170.0f + col * 470.0f, row_y + row * row_h, 430.0f, 44.0f};
        bool active = (i == editing_name_index);
        SDL_SetRenderDrawColor(renderer, active ? 52 : 40, active ? 152 : 54, active ? 219 : 74, 255);
        SDL_RenderFillRect(renderer, &r);
        render_text(renderer, "PLAYER " + std::to_string(i + 1) + ": " +
                    (pending_player_names[static_cast<std::size_t>(i)].empty() ? "<not set>" : pending_player_names[static_cast<std::size_t>(i)]),
                    {255,255,255,255}, r, board_font);
    }

    SDL_SetRenderDrawColor(renderer, 90, 99, 115, 255);
    SDL_RenderFillRect(renderer, &btn_name_back);
    render_text(renderer, "BACK", {255,255,255,255}, btn_name_back, font);

    bool complete = true;
    for (int i = 0; i < selected_player_count; ++i) {
        std::string n = pending_player_names[static_cast<std::size_t>(i)];
        if (i == editing_name_index && !name_input.empty()) n = name_input;
        if (n.empty()) complete = false;
    }

    SDL_SetRenderDrawColor(renderer, complete ? 46 : 70, complete ? 204 : 78, complete ? 113 : 88, 255);
    SDL_RenderFillRect(renderer, &btn_name_start);
    render_text(renderer, "START GAME", {255,255,255,255}, btn_name_start, font);
}

void Game::draw_first_roll(SDL_Renderer* renderer)
{
    if (!renderer || players.empty() || first_roll_candidates.empty() ||
        first_roll_results.size() != players.size()) {
        return;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 12, 20, 245);
    SDL_FRect overlay{0, 0, 1280, 820};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_FRect modal{250, 70, 780, 700};
    SDL_SetRenderDrawColor(renderer, 30, 40, 58, 255);
    SDL_RenderFillRect(renderer, &modal);
    SDL_SetRenderDrawColor(renderer, 245, 210, 70, 255);
    SDL_RenderRect(renderer, &modal);

    render_text(renderer, "WHO GOES FIRST?", {245,210,70,255}, {300, 95, 680, 58}, font);
    render_text(renderer,
                "Highest dice roll starts. If the highest result is tied, only those players roll again.",
                {220,230,245,255}, {300, 152, 680, 42}, board_font);
    render_text(renderer,
                "ROUND " + std::to_string(first_roll_round),
                {170,190,210,255}, {300, 205, 680, 28}, board_font);

    int active_index = -1;
    if (first_roll_position >= 0 &&
        first_roll_position < static_cast<int>(first_roll_candidates.size())) {
        active_index = first_roll_candidates[static_cast<std::size_t>(first_roll_position)];
        if (active_index < 0 || active_index >= static_cast<int>(players.size())) {
            active_index = -1;
        }
    }

    const float list_x = 325.0f;
    const float list_w = 630.0f;
    const float list_y = 245.0f;
    const float row_h = 42.0f;
    const float gap = 5.0f;
    int shown = 0;

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        bool is_candidate = std::find(first_roll_candidates.begin(), first_roll_candidates.end(), i) != first_roll_candidates.end();
        if (!is_candidate && first_roll_round > 1 && first_roll_results[static_cast<std::size_t>(i)] == 0) {
            continue;
        }

        const float y = list_y + shown * (row_h + gap);
        SDL_FRect row{list_x, y, list_w, row_h};
        const bool active = (i == active_index);
        SDL_SetRenderDrawColor(renderer,
            active ? 52 : 45,
            active ? 152 : 61,
            active ? 219 : 78, 255);
        SDL_RenderFillRect(renderer, &row);
        SDL_SetRenderDrawColor(renderer, active ? 245 : 90, active ? 210 : 100, active ? 70 : 110, 255);
        SDL_RenderRect(renderer, &row);

        const int result_value = first_roll_results[static_cast<std::size_t>(i)];
        const std::string result = result_value > 0 ? std::to_string(result_value) : "—";
        render_text(renderer,
                    players[static_cast<std::size_t>(i)].get_name() + "   •   " + result,
                    {255,255,255,255}, row, board_font);
        ++shown;
    }

    const float button_y = std::min(675.0f, list_y + shown * (row_h + gap) + 18.0f);
    btn_first_roll.y = button_y;

    std::string prompt = active_index >= 0
        ? "ROLL: " + players[static_cast<std::size_t>(active_index)].get_name()
        : "RESOLVING...";

    SDL_SetRenderDrawColor(renderer,
        active_index >= 0 ? 46 : 90,
        active_index >= 0 ? 204 : 99,
        active_index >= 0 ? 113 : 115, 255);
    SDL_RenderFillRect(renderer, &btn_first_roll);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &btn_first_roll);
    render_text(renderer, prompt, {255,255,255,255}, btn_first_roll, board_font);

    if (active_index < 0 && first_roll_candidates.size() > 1) {
        render_text(renderer,
                    "Tie! The tied players will roll again.",
                    {245,210,70,255}, {300, 725, 680, 30}, board_font);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::draw_pause_menu(SDL_Renderer* renderer)
{
    if (!renderer) return;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 185);
    SDL_FRect overlay{0, 0, 1280, 820};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_SetRenderDrawColor(renderer, 30, 40, 58, 255);
    SDL_RenderFillRect(renderer, &modal_pause_rect);
    SDL_SetRenderDrawColor(renderer, 245, 205, 70, 255);
    SDL_RenderRect(renderer, &modal_pause_rect);

    render_text(renderer, "PAUSED", {245,205,70,255},
                {modal_pause_rect.x, modal_pause_rect.y + 35.0f, modal_pause_rect.w, 58.0f}, font);
    render_text(renderer, "Game is paused", {220,230,245,255},
                {modal_pause_rect.x, modal_pause_rect.y + 95.0f, modal_pause_rect.w, 35.0f}, board_font);

    const auto draw_btn = [&](const SDL_FRect& r, const SDL_Color& color, const char* text) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_RenderFillRect(renderer, &r);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &r);
        render_text(renderer, text, {255,255,255,255}, r, font);
    };

    draw_btn(btn_pause_resume, {46,204,113,255}, "RESUME");
    draw_btn(btn_pause_save,   {52,152,219,255}, "SAVE GAME");
    draw_btn(btn_pause_load,   {155,89,182,255}, "LOAD GAME");
    draw_btn(btn_pause_menu,   {192,57,43,255}, "EXIT TO MENU");

    render_text(renderer, "ESC", {180,190,205,255},
                {modal_pause_rect.x, modal_pause_rect.y + 450.0f, modal_pause_rect.w, 25.0f}, board_font);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void Game::draw(SDL_Renderer* renderer)
{
    if (!renderer) return;

    if (scene == SceneState::MainGame && state == GameState::FirstRoll) {
        SDL_SetRenderDrawColor(renderer, 8, 12, 20, 255);
        SDL_RenderClear(renderer);
        draw_first_roll(renderer);
        return;
    }

    SDL_RenderClear(renderer);

    switch (scene) {
    case SceneState::MainMenu:
        draw_main_menu(renderer);
        break;
    case SceneState::NameEntry:
        draw_name_entry(renderer);
        break;
    case SceneState::MainGame:
        draw_board(renderer);
        draw_players(renderer);
        draw_hud(renderer);

        if (state == GameState::JailNotification) {
            draw_jail_notification(renderer);
        }
        else if (state == GameState::JailMenu) {
            draw_jail_menu(renderer);
        }
        else if (state == GameState::BuyingProperty) {
            draw_buy_menu(renderer);
        }
        else if (state == GameState::ManagingHouses) {
            draw_build_menu(renderer);
        }
        else if (state == GameState::ExchangeMenu || state == GameState::ExchangeOffer) {
            draw_exchange_menu(renderer);
        }
        else if (state == GameState::AuctionMenu) {
            draw_auction_menu(renderer);
        }
        else if (state == GameState::UsingCardUI) {
            draw_card_menu(renderer);
        }
        else if (state == GameState::BankruptcyMenu) {
            draw_bankruptcy_menu(renderer);
        }
        else if (state == GameState::GameOver) {
            draw_game_over(renderer);
        }

        draw_money_notifications(renderer);

        if (state == GameState::Paused) {
            draw_pause_menu(renderer);
        }
        break;
    }
}

