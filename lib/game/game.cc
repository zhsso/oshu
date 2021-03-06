/**
 * \file game/game.cc
 * \ingroup game
 *
 * Implemention game initialization, destruction and the main loop.
 */

#include "config.h"

#include "game/game.h"
#include "game/tty.h"
#include "graphics/texture.h"
#include "log.h"
#include "./screens/screens.h"

#include <assert.h>
#include <stdio.h>

#include <SDL2/SDL_image.h>

static int open_beatmap(const char *beatmap_path, struct oshu_game *game)
{
	if (oshu_load_beatmap(beatmap_path, &game->beatmap) < 0) {
		oshu_log_error("no beatmap, aborting");
		return -1;
	}
	if (game->beatmap.mode != OSHU_OSU_MODE) {
		oshu_log_error("unsupported game mode");
		return -1;
	}
	assert (game->beatmap.hits != NULL);
	game->hit_cursor = game->beatmap.hits;
	return 0;
}

static int open_audio(struct oshu_game *game)
{
	assert (game->beatmap.audio_filename != NULL);
	if (oshu_open_audio(game->beatmap.audio_filename, &game->audio) < 0) {
		oshu_log_error("no audio, aborting");
		return -1;
	}
	oshu_open_sound_library(&game->library, &game->audio.device_spec);
	oshu_populate_library(&game->library, &game->beatmap);
	return 0;
}

static int open_display(struct oshu_game *game)
{
	if (oshu_open_display(&game->display) < 0) {
		oshu_log_error("no display, aborting");
		return -1;
	}
	struct oshu_metadata *meta = &game->beatmap.metadata;
	char *title;
	if (asprintf(&title, "%s - %s ♯ %s 𝄞 oshu!", meta->artist, meta->title, meta->version) >= 0) {
		SDL_SetWindowTitle(game->display.window, title);
		free(title);
	}
	oshu_reset_view(&game->display);
	return 0;
}

static int create_ui(struct oshu_game *game)
{
	if (game->beatmap.background_filename)
		oshu_load_background(&game->display, game->beatmap.background_filename, &game->ui.background);
	oshu_create_metadata_frame(&game->display, &game->beatmap, &game->clock.system, &game->ui.metadata);
	if (oshu_create_audio_progress_bar(&game->display, &game->audio.music, &game->ui.audio_progress_bar) < 0)
		return -1;
	return 0;
}

int oshu_create_game(const char *beatmap_path, struct oshu_game *game)
{
	game->screen = &oshu_play_screen;
	if (open_beatmap(beatmap_path, game) < 0)
		goto fail;
	if (open_audio(game) < 0)
		goto fail;
	if (open_display(game) < 0)
		goto fail;
	if (create_ui(game) < 0)
		goto fail;
	if (game->initialize() < 0)
		goto fail;
	return 0;
fail:
	oshu_destroy_game(game);
	return -1;
}

static void draw(struct oshu_game *game)
{
	SDL_SetRenderDrawColor(game->display.renderer, 0, 0, 0, 255);
	SDL_RenderClear(game->display.renderer);
	game->screen->draw(game);
	SDL_RenderPresent(game->display.renderer);
}

int oshu_run_game(struct oshu_game *game)
{
	oshu_welcome(game);
	oshu_initialize_clock(game);

	SDL_Event event;
	int missed_frames = 0;

	while (!game->stop) {
		oshu_update_clock(game);
		oshu_reset_view(&game->display);
		while (SDL_PollEvent(&event))
			game->screen->on_event(game, &event);
		game->screen->update(game);
		draw(game);

		/* Calling oshu_print_state before draw causes some flickering
		 * on the tty, for some reason. */
		if (game->screen == &oshu_play_screen)
			oshu_print_state(game);

		double advance = game->display.frame_duration - (SDL_GetTicks() / 1000. - game->clock.system);
		if (advance > 0) {
			SDL_Delay(advance * 1000);
		} else {
			missed_frames++;
			if (missed_frames == 1000) {
				oshu_log_warning("your computer is having a hard time keeping up");
				if (game->display.features)
					oshu_log_warning("try running oshu! with OSHU_QUALITY=low (see the man page)");
			}
		}
	}

	oshu_log_debug("%d missed frames", missed_frames);
	return 0;
}

static void destroy_ui(struct oshu_game *game)
{
	oshu_destroy_background(&game->ui.background);
	oshu_destroy_metadata_frame(&game->ui.metadata);
	oshu_destroy_score_frame(&game->ui.score);
	oshu_destroy_audio_progress_bar(&game->ui.audio_progress_bar);
}

void oshu_destroy_game(struct oshu_game *game)
{
	assert (game != NULL);
	game->destroy();
	oshu_destroy_beatmap(&game->beatmap);
	oshu_close_audio(&game->audio);
	oshu_close_sound_library(&game->library);
	destroy_ui(game);
	oshu_close_display(&game->display);
}
