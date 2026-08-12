#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdlib>
#include <ctime>
#include "Game.hpp"

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static Game* game = nullptr;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	srand(static_cast<unsigned int>(time(nullptr)));

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		return SDL_APP_FAILURE;
	}

	if (!TTF_Init()) {
		return SDL_APP_FAILURE;
	}

	if (!SDL_CreateWindowAndRenderer("Monopoly", 1280, 820, 0, &window, &renderer)) {
		TTF_Quit();
		SDL_Quit();
		return SDL_APP_FAILURE;
	}

	game = new Game();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	if (!game || !game->get_is_running()) {
		return SDL_APP_SUCCESS;
	}

	static Uint64 last = SDL_GetPerformanceCounter();
	Uint64 now = SDL_GetPerformanceCounter();

	float deltaTime = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
	last = now;

	game->update(deltaTime);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	game->draw(renderer);

	SDL_RenderPresent(renderer);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	if (game) game->handle_event(event, renderer);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	delete game;
	game = nullptr;

	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();
}