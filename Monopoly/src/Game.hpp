#pragma once

#include "Board.hpp"
#include "Dice.hpp"
#include "Fields.hpp"
#include "Player.hpp"
#include "TurnManager.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <vector>

enum class SceneState
{
    MainMenu,
    NameEntry,
    MainGame
};

enum class GameState
{
    Setup,
    WaitingRoll,
    JailNotification,
    JailMenu,
    RollingDice,
    MovingPlayer,
    BuyingProperty,
    ManagingHouses,
    ExchangeMenu,
    ExchangeOffer,
    AuctionMenu,
    BankruptcyMenu,
    PlayingField,
    UsingCardUI,
    UsingCard,
    FirstRoll,
    Paused,
    GameOver
};

class Game
{
private:
    SceneState scene = SceneState::MainMenu;
    bool is_setup = false;
    bool is_running = true;
    GameState state = GameState::Setup;
    GameState state_before_pause = GameState::PlayingField;

    TurnManager turn_manager;
    std::vector<Player> players;
    int selected_player_count = 2;
    std::vector<std::string> pending_player_names;
    int editing_name_index = 0;
    std::string name_input;
    bool name_typing = false;

    // First-turn tiebreaker. Only players tied for the highest roll
    // continue into the next round.
    std::vector<int> first_roll_candidates;
    std::vector<int> first_roll_results;
    int first_roll_position = 0;
    int first_roll_round = 1;
    bool first_roll_has_result = false;

    TTF_Font* font = nullptr;
    TTF_Font* board_font = nullptr;

    SDL_FRect btn_2_players{ 230, 300, 190, 72 };
    SDL_FRect btn_3_players{ 440, 300, 190, 72 };
    SDL_FRect btn_4_players{ 650, 300, 190, 72 };
    SDL_FRect btn_5_players{ 860, 300, 190, 72 };
    SDL_FRect btn_6_players{ 335, 400, 190, 72 };
    SDL_FRect btn_7_players{ 545, 400, 190, 72 };
    SDL_FRect btn_8_players{ 755, 400, 190, 72 };
    SDL_FRect btn_start{ 440, 570, 400, 64 };
    SDL_FRect btn_name_start{ 690, 690, 240, 58 };
    SDL_FRect btn_name_back{ 110, 690, 220, 58 };
    SDL_FRect btn_first_roll{ 500, 665, 280, 52 };

    SDL_FRect btn_roll_dice{ 820, 520, 410, 60 };
    SDL_FRect btn_buy_property{ 820, 590, 200, 60 };
    SDL_FRect btn_end_turn{ 1030, 590, 200, 60 };
    SDL_FRect btn_manage_houses{ 820, 660, 410, 50 };
    SDL_FRect btn_exchange{ 820, 715, 410, 50 };

    SDL_FRect modal_buy_rect{ 430.0f, 180.0f, 380.0f, 440.0f };
    SDL_FRect btn_modal_buy{ 455.0f, 540.0f, 150.0f, 50.0f };
    SDL_FRect btn_modal_pass{ 635.0f, 540.0f, 150.0f, 50.0f };

    SDL_FRect modal_jail_rect{ 430.0f, 200.0f, 400.0f, 320.0f };
    SDL_FRect btn_jail_roll{ 450.0f, 270.0f, 360.0f, 50.0f };
    SDL_FRect btn_jail_pay{ 450.0f, 340.0f, 360.0f, 50.0f };
    SDL_FRect btn_jail_card{ 450.0f, 410.0f, 360.0f, 50.0f };

    SDL_FRect modal_card_rect{ 300.0f, 170.0f, 680.0f, 420.0f };
    SDL_FRect btn_card_ok{ 520.0f, 510.0f, 240.0f, 55.0f };

    // Уведомление о попадании в тюрьму.
    SDL_FRect modal_jail_notification{ 330.0f, 250.0f, 620.0f, 300.0f };
    SDL_FRect btn_jail_notification_ok{ 520.0f, 440.0f, 240.0f, 55.0f };
    bool pending_jail = false;

    // Обмен недвижимостью.
    SDL_FRect modal_exchange_rect{ 90.0f, 55.0f, 1100.0f, 700.0f };
    PropertyField* exchange_give = nullptr;
    PropertyField* exchange_receive = nullptr;
    Player* exchange_target = nullptr;
    int exchange_cash = 0; // >0: target pays current, <0: current pays target.
    bool exchange_offer_pending = false;

    // Аукцион.
    SDL_FRect modal_auction_rect{ 250.0f, 110.0f, 780.0f, 600.0f };
    PropertyField* auction_property = nullptr;
    int auction_current_player = -1;
    int auction_highest_player = -1;
    int auction_bid = 0;
    std::string auction_bid_input;
    bool auction_typing = false;
    SDL_Window* text_input_window = nullptr;
    SDL_FRect btn_auction_exact_bid{ 820.0f, 285.0f, 145.0f, 55.0f };

    // Меню паузы (ESC во время игры).
    SDL_FRect modal_pause_rect{ 350.0f, 155.0f, 580.0f, 510.0f };
    SDL_FRect btn_pause_resume{ 420.0f, 285.0f, 440.0f, 58.0f };
    SDL_FRect btn_pause_save{ 420.0f, 355.0f, 440.0f, 58.0f };
    SDL_FRect btn_pause_load{ 420.0f, 425.0f, 440.0f, 58.0f };
    SDL_FRect btn_pause_menu{ 420.0f, 495.0f, 440.0f, 58.0f };
    std::vector<bool> auction_passed;

    // Окончание игры.
    SDL_FRect modal_game_over_rect{ 300.0f, 170.0f, 680.0f, 480.0f };
    SDL_FRect btn_game_over_menu{ 390.0f, 510.0f, 220.0f, 55.0f };
    SDL_FRect btn_game_over_quit{ 670.0f, 510.0f, 220.0f, 55.0f };
    // Меню ручной ликвидации при нехватке денег.
    SDL_FRect modal_bankruptcy_rect{ 160.0f, 80.0f, 960.0f, 650.0f };
    SDL_FRect btn_bankruptcy_up{ 920.0f, 185.0f, 90.0f, 55.0f };
    SDL_FRect btn_bankruptcy_down{ 920.0f, 605.0f, 90.0f, 55.0f };
    SDL_FRect btn_bankruptcy_pay{ 680.0f, 665.0f, 180.0f, 45.0f };
    SDL_FRect btn_bankruptcy_declare{ 875.0f, 665.0f, 210.0f, 45.0f };
    int bankruptcy_scroll = 0;
    int build_scroll = 0;
    int exchange_give_scroll = 0;
    int exchange_receive_scroll = 0;
    bool exchange_keyboard_right = false;

    struct MoneyNotification
    {
        int player_index = -1;
        int amount = 0;
        std::string reason;
        int counterparty = -1;
        bool hidden = false;
        float timer = 0.0f;
        float lifetime = 2.2f;
    };

    std::vector<MoneyNotification> money_notifications;
    std::vector<int> observed_balances;

    static TTF_Font* open_font(const char* path, int size);

    void render_text(SDL_Renderer* renderer, const std::string& text,
                     SDL_Color color, const SDL_FRect& rect, TTF_Font* font);
    SDL_FRect get_field_rect(int index) const noexcept;
    SDL_Color get_group_color(ColorGroup group) const noexcept;

    void draw_board(SDL_Renderer* renderer);
    void draw_players(SDL_Renderer* renderer);
    void draw_buy_menu(SDL_Renderer* renderer);
    void draw_jail_menu(SDL_Renderer* renderer);
    void draw_card_menu(SDL_Renderer* renderer);
    void draw_hud(SDL_Renderer* renderer);
    void draw_dice(SDL_Renderer* renderer);
    void draw_build_menu(SDL_Renderer* renderer);
    void draw_jail_notification(SDL_Renderer* renderer);
    void draw_exchange_menu(SDL_Renderer* renderer);
    void draw_auction_menu(SDL_Renderer* renderer);
    void draw_game_over(SDL_Renderer* renderer);
    void draw_bankruptcy_menu(SDL_Renderer* renderer);
    void draw_pause_menu(SDL_Renderer* renderer);
    void draw_money_notifications(SDL_Renderer* renderer);
    void update_money_notifications(float delta_time);
    void sync_money_notifications();
    void reset_money_tracking();
    void open_bankruptcy_menu();
    void finish_pending_payment();
    void start_auction(PropertyField* property);
    void advance_auction();
    void finish_auction();
    void reset_exchange();
    bool execute_exchange();
    bool save_game(const std::string& path) const;
    bool load_game(const std::string& path);
    void begin_name_entry();
    void begin_first_roll();
    void roll_for_starting_player();
    void resolve_first_roll_round();

public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    bool get_is_running() const noexcept { return is_running; }
    void setup(int player_count, const std::vector<std::string>& player_names);
    void handle_event(const SDL_Event* event, SDL_Renderer* renderer);
    void update(float delta_time);
    void draw_main_menu(SDL_Renderer* renderer);
    void draw_name_entry(SDL_Renderer* renderer);
    void draw_first_roll(SDL_Renderer* renderer);
    void draw(SDL_Renderer* renderer);
};
