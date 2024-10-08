#include <stdio.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#define MAX_WIDTH 1480
#define MAX_HEIGHT 720
#define MAX_ENEMIES 3
#define MAX_FIREBALL 3
#define FRAME_CHANGE_MS 200
#define INTERVAL_ADD_MS 1000
#define INTERVAL_TARGET_MS 3000
#define INTERVAL_MOVE_MS 50

// Enumeration
typedef enum character
{
	Character_1,
	Character_2,
	Character_3
} Characters;

typedef enum difficulty
{
	EASY,
	MEDIUM,
	HARD
} Difficulty;

typedef enum move
{
	Right,
	Left,
	Idle,
	Jump

} Move;

typedef enum menu
{
	Play,
	Exit,
	Resume,
	Main
} Menu;

// Struct
typedef struct player
{
	int x;
	int y;
	int speed;
	int health;
	bool isAlive;
	int score;
	uint64_t damage_cooldown;
	Move move;

} Player;

typedef struct enemy
{
	bool isAlive;
	int x;
	int y;
	int dir_x;
	int dir_y;
	int enemy_index;
	int health;
	uint64_t last_tick_add;
	uint64_t last_tick_move;
	uint64_t last_tick_target;
	SDL_Texture *texture;
} Enemy;

typedef struct boss
{
	int x;
	int y;
	bool isAlive;
	int health;
} Boss;

typedef struct fireball
{
	int x;
	int y;
	int dir_x;
	int dir_y;
	int width;
	int height;
	int speed;
	bool isActive;
	uint64_t last_tick_add;
	uint64_t last_tick_move;
	uint64_t last_tick_target;
	Uint32 start_tick;

} Fireball;

typedef struct bullet
{
	int x;
	int y;
	int width;
	int height;
	int speed;
	bool isActive;

} Bullet;

// To reset the game into initial state
void initialize_game(int *move, Characters character, Difficulty *difficulty, Player *player, Bullet *bullet, Enemy *enemy_array, Boss *boss, int *enemy_counter, int *enemy_counter2)
{
	*move = 3;

	*difficulty;
	// Player
	player->x = -70;
	player->y = MAX_HEIGHT - 375;
	player->speed = 30;
	player->score = 0;
	player->isAlive = true;
	player->move = Idle;
	player->damage_cooldown = 0;

	switch (character)
	{
	case Character_1:
		player->health = 500;
		break;
	case Character_2:
		player->health = 1000;
		break;
	case Character_3:
		player->health = 300;
		break;
	default:
		break;
	}

	// Bullet
	bullet->x = -10;
	bullet->y = -10;
	bullet->isActive = false;
	bullet->width = 10;
	bullet->height = 10;

	// Enemy
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		enemy_array[i].isAlive = false;
		enemy_array[i].x = 0;
		enemy_array[i].y = 0;
		enemy_array[i].dir_x = 0;
		enemy_array[i].dir_y = 0;
		enemy_array[i].enemy_index = 0;
		enemy_array[i].health = 0;
		enemy_array[i].texture = NULL;
		enemy_array->last_tick_add = 0;
		enemy_array->last_tick_move = 0;
		enemy_array->last_tick_target = 0;
	}
	*enemy_counter = 0;
	*enemy_counter2 = 0;

	boss->x = MAX_WIDTH;
	boss->isAlive = false;
	boss->health = 8000;
}

// Draw bullet, boss and fireball
void draw_bullet(SDL_Renderer *renderer, Bullet *bullet)
{
	if (bullet->isActive)
	{
		SDL_Surface *character_surface = IMG_Load("Media/Sprites/Bullet.png");
		SDL_Texture *character_texture = SDL_CreateTextureFromSurface(renderer, character_surface);
		SDL_Rect cover_rect;
		cover_rect.x = bullet->x;
		cover_rect.y = bullet->y;
		cover_rect.w = 10;
		cover_rect.h = 10;
		SDL_RenderCopy(renderer, character_texture, NULL, &cover_rect);
		SDL_DestroyTexture(character_texture);
		SDL_FreeSurface(character_surface);
	}
}

void draw_fireball(SDL_Renderer *renderer, Fireball *fireball_array)
{
	for (int i = 0; i < MAX_FIREBALL; i++)
	{
		if (fireball_array[i].isActive)
		{
			SDL_Surface *fireball_surface = IMG_Load("Media/Sprites/Fireball.png");
			SDL_Texture *fireball_texture = SDL_CreateTextureFromSurface(renderer, fireball_surface);
			SDL_Rect cover_rect;
			cover_rect.x = fireball_array[i].x;
			cover_rect.y = fireball_array[i].y;
			cover_rect.w = 50;
			cover_rect.h = 50;
			SDL_RenderCopy(renderer, fireball_texture, NULL, &cover_rect);
			SDL_DestroyTexture(fireball_texture);
			SDL_FreeSurface(fireball_surface);
		}
	}
}

void draw_boss(SDL_Renderer *renderer, Boss *boss)
{
	boss->x -= 10;
	boss->y = 300;
	if (boss->isAlive)
	{
		SDL_Surface *character_surface3 = IMG_Load("Media/Sprites/Boss.png");
		SDL_Texture *character_texture3 = SDL_CreateTextureFromSurface(renderer, character_surface3);
		SDL_Rect cover_rect3;
		cover_rect3.x = boss->x;
		cover_rect3.y = boss->y;
		cover_rect3.w = 300;
		cover_rect3.h = 400;
		SDL_RenderCopy(renderer, character_texture3, NULL, &cover_rect3);
		SDL_DestroyTexture(character_texture3);
		SDL_FreeSurface(character_surface3);

		if (boss->x <= 1180)
		{
			boss->x = 1180;
		}
	}
}

// Check collision
bool check_collision_bullet(Bullet *bullet, Enemy *enemy)
{
	return (
		bullet->x < enemy->x + 300 &&
		bullet->x + bullet->width > enemy->x &&
		bullet->y < enemy->y + 250 &&
		bullet->y + bullet->height > enemy->y);
}

bool check_collision_player(Player *player, Enemy *enemy)
{
	return (
		player->x + 150 < enemy->x + 300 &&
		player->x + 150 > enemy->x &&
		player->y < enemy->y + 250 &&
		player->y + 150 > enemy->y);
}

bool check_collision_fireball(Player *player, Fireball *fireball)
{
	int playerLeft = player->x;
	int playerRight = player->x + 200;
	int playerTop = player->y + 150;
	int playerBottom = player->y + 325;

	int fireballLeft = fireball->x;
	int fireballRight = fireball->x + fireball->width;
	int fireballTop = fireball->y;
	int fireballBottom = fireball->y + fireball->height;

	return (
		playerLeft < fireballRight &&
		playerRight > fireballLeft &&
		playerTop < fireballBottom &&
		playerBottom > fireballTop);
}

void handle_collisions(Difficulty difficulty, bool *running2, bool *running, Player *player, Enemy *enemy_array, Fireball *fireball, int score)
{
	static Uint32 game_over_timestamp = 0; // Timestamp for game over

	// Collision with wall
	// Horizontally
	if (player->x < -70)
	{
		player->x = -70;
	}
	else if (player->x > MAX_WIDTH - 200)
	{
		player->x = MAX_WIDTH - 200;
	}

	// Vertically
	if (player->y < 0)
	{
		player->y = 0;
	}
	else if (player->y > 590 - 70)
	{
		player->y = 590 - 70;
	}

	// Check collisions with fireball and enemies based on difficulty
	int fireball_damage = 0;
	int enemy_damage = 0;

	switch (difficulty)
	{
	case EASY:
		fireball_damage = 10;
		enemy_damage = 25;
		break;
	case MEDIUM:
		fireball_damage = 20;
		enemy_damage = 50;
		break;
	case HARD:
		fireball_damage = 30;
		enemy_damage = 100;
		break;
	default:
		break;
	}

	// Check collisions with fireball
	for (int i = 0; i < MAX_FIREBALL; i++)
	{
		if (fireball[i].isActive && check_collision_fireball(player, &fireball[i]))
		{
			// Check if the damage cooldown has expired
			if (SDL_GetTicks() - player->damage_cooldown > 500)
			{
				// Player collides with fireball, reduce player's health
				player->health -= fireball_damage;
				fireball[i].isActive = false;

				// Set the damage cooldown to the current time
				player->damage_cooldown = SDL_GetTicks();

				// Check for game over
				// Player health reaches zero
				if (player->health <= 0)
				{
					player->isAlive = false;
					if (game_over_timestamp == 0)
					{
						player->health = 0;
						game_over_timestamp = SDL_GetTicks();
					}
					else
					{
						Uint32 current_time = SDL_GetTicks();
						if (current_time - game_over_timestamp > 500)
						{
							printf("Game Over! Your final score: %d\n", player->score);
							*running = false;
							*running2 = true;
						}
					}
				}
				else
				{
					// Reset the game over timestamp if the player's health is not zero
					game_over_timestamp = 0;
				}
			}
		}
	}

	// Check collisions with enemies
	for (int i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemy_array[i].isAlive && check_collision_player(player, &enemy_array[i]))
		{
			// Check if the damage cooldown has expired
			if (SDL_GetTicks() - player->damage_cooldown > 500)
			{
				// Player collides with enemy, reduce player's health
				player->health -= enemy_damage;

				// Set the damage cooldown to the current time
				player->damage_cooldown = SDL_GetTicks();

				// Check for game over
				// Player health reaches zero
				if (player->health <= 0)
				{
					player->isAlive = false;
					if (game_over_timestamp == 0)
					{
						player->health = 0;
						game_over_timestamp = SDL_GetTicks();
					}
					else
					{
						Uint32 current_time = SDL_GetTicks();
						player->health = 0;
						if (current_time - game_over_timestamp > 500)
						{
							printf("Game Over! Your final score: %d\n", player->score);
							*running = false;
							*running2 = true;
						}
					}
				}
				else
				{
					// Reset the game over timestamp if the player's health is not zero
					game_over_timestamp = 0;
				}
			}
		}
	}
}

// Updating bullet on enemy
void update_bullet(Mix_Chunk *audio_zombie, Characters character, Bullet *bullet, Player *player, Enemy *enemy_array, Boss *boss, int *enemy_counter)
{
	switch (character)
	{
	case Character_1:
		bullet->speed = 25;
		if (bullet->isActive)
		{
			bullet->x += bullet->speed;

			if (bullet->x > MAX_WIDTH)
			{
				bullet->isActive = false;
			}

			if (boss->isAlive)
			{
				if (bullet->x < boss->x + 300 && bullet->x + bullet->width > boss->x && bullet->y < boss->y + 400 && bullet->y + bullet->height > boss->y)
				{
					bullet->isActive = false;
					boss->health -= 100;
				}
			}

			// Check collision with enemies
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (enemy_array[i].isAlive && check_collision_bullet(bullet, &enemy_array[i]))
				{
					// Enemy is hit, reduce its health
					enemy_array[i].health -= 100;

					if (enemy_array[i].health <= 0)
					{
						// Enemy is killed, mark as inactive
						enemy_array[i].isAlive = false;
						player->score += 10;
						Mix_PlayChannel(-1, audio_zombie, 0);

						(*enemy_counter)++;
					}

					bullet->isActive = false;
				}
			}
		}
		break;
	case Character_2:
		bullet->speed = 100;
		if (bullet->isActive)
		{
			bullet->x += bullet->speed;

			if (bullet->x > MAX_WIDTH)
			{
				bullet->isActive = false;
			}

			if (boss->isAlive)
			{
				if (bullet->x < boss->x + 300 && bullet->x + bullet->width > boss->x && bullet->y < boss->y + 400 && bullet->y + bullet->height > boss->y)
				{
					bullet->isActive = false;
					boss->health -= 10;
				}
			}

			// Check collision with enemies
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (enemy_array[i].isAlive && check_collision_bullet(bullet, &enemy_array[i]))
				{
					// Enemy is hit, reduce its health
					enemy_array[i].health -= 25;

					if (enemy_array[i].health <= 0)
					{
						// Enemy is killed, mark as inactive
						enemy_array[i].isAlive = false;
						player->score += 10;
						Mix_PlayChannel(-1, audio_zombie, 0);

						(*enemy_counter)++;
					}

					bullet->isActive = false;
				}
			}
		}
		break;
	case Character_3:
		bullet->speed = 300;
		if (bullet->isActive)
		{
			bullet->x += bullet->speed;

			if (bullet->x > MAX_WIDTH)
			{
				bullet->isActive = false;
			}

			if (boss->isAlive)
			{
				if (bullet->x < boss->x + 300 && bullet->x + bullet->width > boss->x && bullet->y < boss->y + 400 && bullet->y + bullet->height > boss->y)
				{
					bullet->isActive = false;
					boss->health -= 50;
				}
			}

			// Check collision with enemies
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (enemy_array[i].isAlive && check_collision_bullet(bullet, &enemy_array[i]))
				{
					// Enemy is hit, reduce its health
					enemy_array[i].health -= 50;

					if (enemy_array[i].health <= 0)
					{
						// Enemy is killed, mark as inactive
						enemy_array[i].isAlive = false;
						player->score += 10;
						Mix_PlayChannel(-1, audio_zombie, 0);

						(*enemy_counter)++;
					}

					bullet->isActive = false;
				}
			}
		}
		break;
	}
}

// Spawn fireball when boss come
void adding_fireball_on_screen(Move move, Difficulty difficulty, Fireball *fireball_array, Boss *boss, Player *player)
{
	bool should_move_fireball = false;
	bool should_add_fireball = false;

	// Move fireball logic
	should_move_fireball = false;

	if (SDL_GetTicks() - fireball_array->last_tick_move > INTERVAL_MOVE_MS)
	{
		should_move_fireball = true;
		fireball_array->last_tick_move = SDL_GetTicks();
	}

	if (should_move_fireball)
	{
		for (int i = 0; i < MAX_FIREBALL; i++)
		{
			if (fireball_array[i].isActive)
			{
				int dir_x = player->x - fireball_array[i].x;
				int dir_y = (player->y + 300) - fireball_array[i].y;

				// Normalize the direction
				float length = sqrt(dir_x * dir_x + dir_y * dir_y);
				if (length != 0)
				{
					dir_x = (int)(dir_x / length * 10);
					dir_y = (int)(dir_y / length * 10);
				}

				// Update fireball position
				fireball_array[i].x += dir_x;

				// If the player is not jumping, update the fireball's position in the y-axis
				if (move != Jump)
				{
					fireball_array[i].y += dir_y;
				}

				fireball_array[i].dir_x = dir_x;
				fireball_array[i].dir_y = dir_y;

				// Check if the fireball has reached the ground
				if (fireball_array[i].y >= 620)
				{
					player->health += 0;
					fireball_array[i].isActive = false;
				}

				if (move != Jump && dir_x == 0)
				{
					fireball_array[i].isActive = false;
				}

				if (SDL_GetTicks() - fireball_array[i].start_tick > 15000)
				{
					fireball_array[i].isActive = false;
				}
			}
		}
	}

	// Add fireball logic
	should_add_fireball = false;

	if (SDL_GetTicks() - fireball_array->last_tick_add > INTERVAL_ADD_MS)
	{
		should_add_fireball = true;
		fireball_array->last_tick_add = SDL_GetTicks();
	}

	if (should_add_fireball)
	{
		for (int i = 0; i < MAX_FIREBALL; i++)
		{
			if (!fireball_array[i].isActive)
			{
				fireball_array[i].isActive = true;
				fireball_array[i].x = MAX_WIDTH;
				fireball_array[i].y = rand() % MAX_HEIGHT;
				fireball_array[i].dir_x = (player->x - boss->x) / 15;

				// If the player is jumping, make fireball fall straight down
				if (move == Jump)
				{
					fireball_array[i].dir_y = 10;
				}
				else
				{
					fireball_array[i].dir_y = 10;
				}

				fireball_array[i].start_tick = SDL_GetTicks();
			}
		}
	}
}

// Spawn enemies on game screen
void adding_enemy_on_screen(Difficulty difficulty, Enemy *enemy_array, SDL_Texture **texture_enemy, int *enemy_index, Player *player, int *alive_enemy_counter)
{
	bool should_move_enemy = false;
	bool should_add_enemy = false;

	// Move enemy logic
	should_move_enemy = false;

	if (SDL_GetTicks() - enemy_array->last_tick_move > INTERVAL_MOVE_MS)
	{
		should_move_enemy = true;
		enemy_array->last_tick_move = SDL_GetTicks();
	}

	if (should_move_enemy)
	{
		for (int i = 0; i < MAX_ENEMIES; i++)
		{
			if (enemy_array[i].isAlive)
			{
				// Calculate direction towards the player
				int dir_x = (player->x + 100) - enemy_array[i].x;

				// Normalize the direction
				float length = sqrt(dir_x * dir_x);
				if (length != 0)
				{
					dir_x = (int)(dir_x / length * 7);
				}

				enemy_array[i].x += dir_x;

				// Update the direction variables in the enemy struct
				enemy_array[i].dir_x = dir_x;

				// Update the y-coordinate to match the player's y-coordinate
				enemy_array[i].y = MAX_HEIGHT - 300;
			}
		}
	}

	// Add enemy logic
	should_add_enemy = false;

	if (SDL_GetTicks() - enemy_array->last_tick_add > INTERVAL_ADD_MS)
	{
		should_add_enemy = true;
		enemy_array->last_tick_add = SDL_GetTicks();
	}

	switch (difficulty)
	{
	case EASY:
		if (should_add_enemy && *alive_enemy_counter < 20)
		{
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (!enemy_array[i].isAlive)
				{
					// Initialize a new enemy if it's not alive
					enemy_array[i].isAlive = true;
					enemy_array[i].enemy_index = rand() % 3;
					enemy_array[i].texture = texture_enemy[enemy_array[i].enemy_index];
					enemy_array[i].x = MAX_WIDTH + (*enemy_index) * 20;

					enemy_array[i].y = MAX_HEIGHT - 200;
					enemy_array[i].dir_x = -10 + rand() % 10;

					enemy_array[i].health = 150;

					if (*enemy_index < MAX_ENEMIES - 1)
					{
						(*enemy_index) += 1;
					}
					else
					{
						(*enemy_index) = 0;
					}

					(*alive_enemy_counter)++; // Increment only when an enemy is added

					break;
				}
			}
		}
		break;

	case MEDIUM:
		if (should_add_enemy && *alive_enemy_counter < 50)
		{
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (!enemy_array[i].isAlive)
				{
					// Initialize a new enemy if it's not alive
					enemy_array[i].isAlive = true;
					enemy_array[i].enemy_index = rand() % 3; // Assuming you have 3 types of enemies
					enemy_array[i].texture = texture_enemy[enemy_array[i].enemy_index];

					// Set the initial x-coordinate with a gap between enemies
					enemy_array[i].x = MAX_WIDTH + (*enemy_index) * 20; // Adjust the gap as needed

					enemy_array[i].y = MAX_HEIGHT - 70; // Set y-coordinate to player's y-coordinate

					// Set the same initial direction for all enemies
					enemy_array[i].dir_x = -50 + rand() % 10;

					enemy_array[i].health = 250;

					if (*enemy_index < MAX_ENEMIES - 1)
					{
						(*enemy_index) += 1;
					}
					else
					{
						(*enemy_index) = 0;
					}

					(*alive_enemy_counter)++; // Increment only when an enemy is added

					break;
				}
			}
		}
		break;

	case HARD:
		if (should_add_enemy && *alive_enemy_counter < 100)
		{
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (!enemy_array[i].isAlive)
				{
					// Initialize a new enemy if it's not alive
					enemy_array[i].isAlive = true;
					enemy_array[i].enemy_index = rand() % 3; // Assuming you have 3 types of enemies
					enemy_array[i].texture = texture_enemy[enemy_array[i].enemy_index];

					// Set the initial x-coordinate with a gap between enemies
					enemy_array[i].x = MAX_WIDTH + (*enemy_index) * 20; // Adjust the gap as needed

					enemy_array[i].y = MAX_HEIGHT - 70;
					enemy_array[i].dir_x = -100 + rand() % 10;

					enemy_array[i].health = 350;

					if (*enemy_index < MAX_ENEMIES - 1)
					{
						(*enemy_index) += 1;
					}
					else
					{
						(*enemy_index) = 0;
					}

					(*alive_enemy_counter)++; // Increment only when an enemy is added

					break;
				}
			}
		}
		break;
	}
}

// Function for text and buttons
void draw_health(const char *text, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Player *player)
{
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	int texW = 0, texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = {(player->x + 125), (player->y + 150), texW, texH};
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

void draw_health_boss(const char *text, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, Boss *boss)
{
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	int texW = 0, texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = {(boss->x + 80), (boss->y - 15), texW, texH};
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

void draw_score(const char *text, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color)
{
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	int texW = 0, texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = {1000, 10, texW, texH};
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

void show_score(const char *text, SDL_Renderer *renderer, TTF_Font *font, SDL_Color color)
{
	SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	int texW = 0, texH = 0;
	SDL_QueryTexture(texture, NULL, NULL, &texW, &texH);
	SDL_Rect dstrect = {900, 290, texW, texH};
	SDL_RenderCopy(renderer, texture, NULL, &dstrect);
	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

void button_music(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Button_musics.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 10;
	cover_rect.y = 10;
	cover_rect.w = 40;
	cover_rect.h = 40;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
}

void escape_button(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Button_pause.png");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 60;
	cover_rect.y = 10;
	cover_rect.w = 40;
	cover_rect.h = 40;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
}

// Screen
void menu_screen(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Menu.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

void game_screen(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Sprites/Final_BG.png");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);

	// Button
	escape_button(renderer);
	button_music(renderer);
}

void pause_screen(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Pause.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

void change_difficulty(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Difficulty.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

void change_character(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Character_selection.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

void game_over_screen(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Game_over.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

void winning_screen(SDL_Renderer *renderer)
{
	SDL_Surface *cover_surface = IMG_Load("Media/Photos/Winning_screen.jpg");
	SDL_Texture *cover_texture = SDL_CreateTextureFromSurface(renderer, cover_surface);
	SDL_Rect cover_rect;
	cover_rect.x = 0;
	cover_rect.y = 0;
	cover_rect.w = MAX_WIDTH;
	cover_rect.h = MAX_HEIGHT;
	SDL_RenderCopy(renderer, cover_texture, NULL, &cover_rect);
	SDL_DestroyTexture(cover_texture);
	SDL_FreeSurface(cover_surface);
	button_music(renderer);
}

// Change screen
void change_screen(int mx, int my, Menu menu, bool *isRunning)
{
	switch (menu)
	{
	case Play:
		if (mx > 540 && mx < 940 && my > 360 && my < 440)
		{
			printf("Play program\n");
			*isRunning = false;
		}
		break;
	case Exit:
		if (mx > 540 && mx < 940 && my > 480 && my < 560 || mx > 540 && mx < 945 && my > 440 && my < 520)
		{
			printf("Exit program\n");
			*isRunning = false;
		}
		else if (mx > 60 && mx < 100 && my > 10 && my < 50)
		{
			printf("Goes back to menu\n");
			*isRunning = false;
		}

		break;
	case Resume:
		if (mx > 540 && mx < 945 && my > 160 && my < 270)
		{
			printf("Resumes the game\n");
			*isRunning = false;
		}
		break;
	case Main:
		if (mx > 540 && mx < 945 && my > 295 && my < 375 || mx > 395 && mx < 740 && my > 480 && my < 580 || mx > 295 && mx < 740 && my > 515 && my < 615)
		{
			printf("Main screen\n");
			*isRunning = false;
		}

		break;
	}
}

// Change Music
void change_music(int mx, int my, int *song_queue, char *song[], Mix_Music **music)
{
	if (mx > 10 && mx < 50 && my > 10 && my < 50)
	{
		Mix_HaltMusic();
		if (*music != NULL)
		{
			Mix_FreeMusic(*music);
		}

		*song_queue = (*song_queue + 1) % 3;

		*music = Mix_LoadMUS(song[*song_queue]);
		Mix_VolumeMusic(25);
		Mix_PlayMusic(*music, -1);
		printf("Playing song %d\n", *song_queue + 1);
	}
}

int main(int argc, char *args[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	Mix_Init(MIX_INIT_MP3);
	TTF_Init();

	SDL_Window *window = SDL_CreateWindow(
		"RUINS GAME",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MAX_WIDTH, MAX_HEIGHT, 0);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

	// Game Default music, music array and chunk
	Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096);
	Mix_Music *music = Mix_LoadMUS("Media/Musics/Fairy-Tail-OP.mp3");
	Mix_VolumeMusic(25);
	Mix_PlayMusic(music, -1);

	char *musics[] = {"Media/Musics/Fairy-Tail-OP.mp3", "Media/Musics/Masayume-chasing.mp3", "Media/Musics/mute.mp3"};
	int currentSongIndex = 0;

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
	Mix_Chunk *audio_zombie = Mix_LoadWAV("Media/Sound_Effects/Zombie_sound2.mp3");
	Mix_VolumeChunk(audio_zombie, 35);

	Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
	Mix_Chunk *bullet_effect = Mix_LoadWAV("Media/Sound_Effects/Bullet_effect.mp3");
	Mix_VolumeChunk(bullet_effect, 10);

	// Text Related
	TTF_Font *font = TTF_OpenFont("Media/Fonts/Beyonders.ttf", 10);
	TTF_Font *font_score = TTF_OpenFont("Media/Fonts/Beyonders.ttf", 30);
	TTF_Font *font_showscore = TTF_OpenFont("Media/Fonts/Beyonders.ttf", 100);
	SDL_Color color = {255, 255, 255};
	SDL_Color color_health = {0, 255, 0};
	SDL_Color color_lowhealth = {255, 0, 0};

	// Load All Sprite (Enemy)
	SDL_Surface *img_zombie1 = IMG_Load("Media/Sprites/Run_Zombie1.png");
	SDL_Surface *img_zombi2 = IMG_Load("Media/Sprites/Run_Zombie2.png");
	SDL_Surface *img_zombie3 = IMG_Load("Media/Sprites/Run_Zombie3.png");
	SDL_Texture *texture_enemy[3] = {
		SDL_CreateTextureFromSurface(renderer, img_zombie1),
		SDL_CreateTextureFromSurface(renderer, img_zombi2),
		SDL_CreateTextureFromSurface(renderer, img_zombie3)};

	SDL_Rect enemyRect[3];

	// Set dimensions for each enemy type
	enemyRect[0].x = 0;
	enemyRect[0].y = 0;
	SDL_QueryTexture(texture_enemy[0], NULL, NULL, &enemyRect[0].w, &enemyRect[0].h);
	enemyRect[0].w /= 8;
	enemyRect[0].h /= 1;

	enemyRect[1].x = 0;
	enemyRect[1].y = 0;
	SDL_QueryTexture(texture_enemy[1], NULL, NULL, &enemyRect[1].w, &enemyRect[1].h);
	enemyRect[1].w /= 7;
	enemyRect[1].h /= 1;

	enemyRect[2].x = 0;
	enemyRect[2].y = 0;
	SDL_QueryTexture(texture_enemy[2], NULL, NULL, &enemyRect[2].w, &enemyRect[2].h);
	enemyRect[2].w /= 7;
	enemyRect[2].h /= 1;

	SDL_Rect rect_enemy[3];
	rect_enemy[0].x = 0;
	rect_enemy[0].y = 0;
	rect_enemy[0].w = 300;
	rect_enemy[0].h = 250;

	rect_enemy[1].x = 0;
	rect_enemy[1].y = 0;
	rect_enemy[1].w = 300;
	rect_enemy[1].h = 250;

	rect_enemy[2].x = 0;
	rect_enemy[2].y = 0;
	rect_enemy[2].w = 300;
	rect_enemy[2].h = 250;

	// Clear Sprite
	SDL_FreeSurface(img_zombie1);
	SDL_FreeSurface(img_zombi2);
	SDL_FreeSurface(img_zombie3);

	// Load all sprites (Character)
	SDL_Surface *img_character1 = IMG_Load("Media/Sprites/C1.png");
	SDL_Surface *img_character2 = IMG_Load("Media/Sprites/C2.png");
	SDL_Surface *img_character3 = IMG_Load("Media/Sprites/C3.png");
	SDL_Texture *texture_character[3] = {
		SDL_CreateTextureFromSurface(renderer, img_character1),
		SDL_CreateTextureFromSurface(renderer, img_character2),
		SDL_CreateTextureFromSurface(renderer, img_character3)};

	SDL_FreeSurface(img_character1);
	SDL_FreeSurface(img_character2);
	SDL_FreeSurface(img_character3);

	SDL_Rect characterRect[3];

	// Set dimensions for each character
	characterRect[0].x = 0;
	characterRect[0].y = 0;
	SDL_QueryTexture(texture_character[0], NULL, NULL, &characterRect[0].w, &characterRect[0].h);
	characterRect[0].w /= 6;
	characterRect[0].h /= 5;

	characterRect[1].x = 0;
	characterRect[1].y = 0;
	SDL_QueryTexture(texture_character[1], NULL, NULL, &characterRect[1].w, &characterRect[1].h);
	characterRect[1].w /= 8;
	characterRect[1].h /= 5;

	characterRect[2].x = 0;
	characterRect[2].y = 0;
	SDL_QueryTexture(texture_character[2], NULL, NULL, &characterRect[2].w, &characterRect[2].h);
	characterRect[2].w /= 8;
	characterRect[2].h /= 5;

	SDL_Rect rect_character[3];
	rect_character[0].x = 0;
	rect_character[0].y = 0;
	rect_character[0].w = 350;
	rect_character[0].h = 325;

	rect_character[1].x = 0;
	rect_character[1].y = 0;
	rect_character[1].w = 350;
	rect_character[1].h = 325;

	rect_character[2].x = 0;
	rect_character[2].y = 0;
	rect_character[2].w = 350;
	rect_character[2].h = 325;

	int enemy_renewal_idx = 0;
	Enemy enemy_array[MAX_ENEMIES] = {
		{.isAlive = false},
		{.isAlive = false},
		{.isAlive = false}};

	Fireball fireball[MAX_FIREBALL] = {
		{.isActive = false},
		{.isActive = false},
		{.isActive = false},
	};

	// Application state
	uint64_t frame = 0;
	bool run_whole = true;
	bool isRunning = true;
	bool difficulty_screen = true;
	bool character_selection = true;
	bool game_over = true;
	bool winning = true;
	bool running = true;
	bool running2 = true;
	SDL_Event event;

	// Struct and enum
	Bullet playerBullet;
	playerBullet.isActive = false;
	Player player;
	Boss boss;
	boss.isAlive = false;
	Menu menu;
	Difficulty difficulty;
	Characters character;

	int move = 3;
	int enemy_counter = 0;
	int enemy_counter2 = 0;

	while (run_whole)
	{
		while (isRunning)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{

				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 540 && mouseX < 940 && mouseY > 480 && mouseY < 560)
						{
							menu = Exit;
							change_screen(mouseX, mouseY, menu, &isRunning);
							running = false;
							running2 = false;
							isRunning = false;
							run_whole = false;
							difficulty_screen = false;
							character_selection = false;
							game_over = false;
							winning = false;
						}
						else if (mouseX > 540 && mouseX < 940 && mouseY > 360 && mouseY < 440)
						{
							menu = Play;
							change_screen(mouseX, mouseY, menu, &isRunning);
							difficulty_screen = true;
							running = false;
							running2 = false;
						}
					}
					break;
				}
			}

			// Menu
			menu_screen(renderer);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (difficulty_screen)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 530 && mouseX < 950 && mouseY > 225 && mouseY < 325)
						{
							difficulty = EASY;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							difficulty_screen = false;
							character_selection = true;
						}
						else if (mouseX > 530 && mouseX < 950 && mouseY > 370 && mouseY < 470)
						{
							difficulty = MEDIUM;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							difficulty_screen = false;
							character_selection = true;
						}
						else if (mouseX > 530 && mouseX < 950 && mouseY > 515 && mouseY < 615)
						{
							difficulty = HARD;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							difficulty_screen = false;
							character_selection = true;
						}
					}
					break;
				}
			}

			change_difficulty(renderer);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (character_selection)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 165 && mouseX < 455 && mouseY > 540 && mouseY < 615)
						{
							character = Character_1;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							character_selection = false;
							running = true;
						}
						else if (mouseX > 565 && mouseX < 860 && mouseY > 540 && mouseY < 615)
						{
							character = Character_2;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							character_selection = false;
							running = true;
						}
						else if (mouseX > 970 && mouseX < 1260 && mouseY > 540 && mouseY < 615)
						{
							character = Character_3;
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							character_selection = false;
							running = true;
						}
					}
					break;
				}
			}

			change_character(renderer);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (running)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 60 && mouseX < 100 && mouseY > 10 && mouseY < 50)
						{
							menu = Exit;
							change_screen(mouseX, mouseY, menu, &running);
							game_over = false;
							running2 = true;
						}
					}
					break;
				case SDL_KEYDOWN:
					switch (event.key.keysym.scancode)
					{
					case SDL_SCANCODE_ESCAPE:
						running = false;
						game_over = false;
						winning = false;
						running2 = true;
						break;
					case SDL_SCANCODE_A:
						player.move = Left;
						move = 1;
						break;
					case SDL_SCANCODE_D:
						player.move = Right;
						move = 0;
						break;
					case SDL_SCANCODE_RIGHT:
						player.move = Jump;
						break;
					case SDL_SCANCODE_SPACE:
						if (!playerBullet.isActive)
						{
							playerBullet.isActive = true;
							playerBullet.x = player.x + 305;
							playerBullet.y = player.y + 190;
							if (character == Character_2)
							{
								playerBullet.x = player.x + 305;
								playerBullet.y = player.y + 220;
							}
							else if (character == Character_3)
							{
								playerBullet.x = player.x + 280;
								playerBullet.y = player.y + 205;
							}
						}
						move = 2;
						Mix_PlayChannel(-1, bullet_effect, 0);
						break;
					}
					break;

				case SDL_KEYUP:
					// Handle key release events
					switch (event.key.keysym.scancode)
					{
					case SDL_SCANCODE_A:
						player.move = Idle;
						move = 3;
						break;
					case SDL_SCANCODE_D:
						player.move = Idle;
						move = 3;
						break;
					case SDL_SCANCODE_RIGHT:
						player.move = Idle;
						break;
					case SDL_SCANCODE_SPACE:
						move = 3;
					}
					break;
				}
			}

			// Update player position based on movement
			int movement_x = 0;
			int jump_y = 0;

			switch (player.move)
			{
			case Right:
				movement_x = player.speed;
				break;
			case Left:
				movement_x = -player.speed;
				break;
			case Jump:
				if (player.y >= 345)
				{
					jump_y -= 200;
					movement_x += 200;
				}
				break;
			case Idle:
				if (player.y < 345)
				{
					player.y = 345;
				}

				break;
			}

			// Game screen
			game_screen(renderer);

			if (!player.isAlive)
			{
				move = 4;
			}

			// Player
			player.x += movement_x;
			player.y += jump_y;
			switch (character)
			{
			case Character_1:
				int frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 6;
				characterRect[0].x = frame_animate * characterRect[0].w;
				characterRect[0].y = move * characterRect[0].h;
				if (move == 2)
				{
					int frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 4;
					characterRect[0].x = frame_animate * characterRect[0].w;
					characterRect[0].y = move * characterRect[0].h;
				}
				if (!player.isAlive)
				{
					int frame_animate = (SDL_GetTicks() / 400) % 4;
					characterRect[0].x = frame_animate * characterRect[0].w;
					characterRect[0].y = 4 * characterRect[0].h;
				}

				rect_character[0].x = player.x;
				rect_character[0].y = player.y;
				SDL_RenderCopy(renderer, texture_character[0], &characterRect[0], &rect_character[0]);
				break;
			case Character_2:
				frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 8;
				characterRect[1].x = frame_animate * characterRect[1].w;
				characterRect[1].y = move * characterRect[1].h;
				if (move == 3)
				{
					frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 7;
					characterRect[1].x = frame_animate * characterRect[1].w;
					characterRect[1].y = move * characterRect[1].h;
				}

				if (move == 2)
				{
					frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 4;
					characterRect[1].x = frame_animate * characterRect[1].w;
					characterRect[1].y = move * characterRect[1].h;
				}
				if (!player.isAlive)
				{
					frame_animate = (SDL_GetTicks() / 400) % 4;
					characterRect[1].x = frame_animate * characterRect[1].w;
					characterRect[1].y = 4 * characterRect[1].h;
				}

				rect_character[1].x = player.x;
				rect_character[1].y = player.y;
				SDL_RenderCopy(renderer, texture_character[1], &characterRect[1], &rect_character[1]);
				break;
			case Character_3:
				frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 8;
				characterRect[2].x = frame_animate * characterRect[2].w;
				characterRect[2].y = move * characterRect[2].h;
				if (move == 2)
				{
					frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 4;
					characterRect[2].x = frame_animate * characterRect[2].w;
					characterRect[2].y = move * characterRect[2].h;
				}
				if (!player.isAlive)
				{
					frame_animate = (SDL_GetTicks() / 400) % 4;
					characterRect[2].x = frame_animate * characterRect[2].w;
					characterRect[2].y = 4 * characterRect[2].h;
				}

				rect_character[2].x = player.x;
				rect_character[2].y = player.y;
				SDL_RenderCopy(renderer, texture_character[2], &characterRect[2], &rect_character[2]);
				break;

			default:
				break;
			}

			// Score
			int scores = player.score;
			char text2[256];
			sprintf(text2, "Score %d", scores);
			draw_score(text2, renderer, font_score, color);

			// Collision
			handle_collisions(difficulty, &game_over, &running, &player, enemy_array, fireball, scores);

			// Player's health
			int health = player.health;
			char text[256];
			sprintf(text, "Health %d", health);
			if (health > 100)
			{
				draw_health(text, renderer, font, color_health, &player);
			}
			else
			{
				draw_health(text, renderer, font, color_lowhealth, &player);
			}

			// Bullet
			draw_bullet(renderer, &playerBullet);
			update_bullet(audio_zombie, character, &playerBullet, &player, enemy_array, &boss, &enemy_counter);

			// Enemy
			adding_enemy_on_screen(difficulty, enemy_array, texture_enemy, &enemy_renewal_idx, &player, &enemy_counter2);
			for (int i = 0; i < MAX_ENEMIES; i++)
			{
				if (enemy_array[i].isAlive)
				{
					// Animate character
					int frame_animate = (SDL_GetTicks() / FRAME_CHANGE_MS) % 7;
					enemyRect[i].x = frame_animate * enemyRect[i].w;
					enemyRect[i].y = 0 * enemyRect[i].h;

					rect_enemy[i].x = enemy_array[i].x;
					rect_enemy[i].y = enemy_array[i].y;
					SDL_RenderCopy(renderer, texture_enemy[i], &enemyRect[i], &rect_enemy[i]);
				}
			}

			int health_boss = boss.health;
			switch (difficulty)
			{
			case EASY:
				if (enemy_counter == 20)
				{
					// Boss
					boss.isAlive = true;
					draw_boss(renderer, &boss);
					adding_fireball_on_screen(player.move, difficulty, fireball, &boss, &player);
					char text2[256];
					sprintf(text2, "Health %d", health_boss);
					draw_health_boss(text2, renderer, font, color_health, &boss);
					draw_fireball(renderer, fireball);
					if (boss.health <= 0)
					{
						boss.isAlive = false;
						player.score += 100;
						printf("You win! All enemies are defeated.\n");
						running = false;
						game_over = false;
						winning = true;
					}
				}
				break;
			case MEDIUM:
				if (enemy_counter == 50)
				{
					// Boss
					boss.isAlive = true;
					draw_boss(renderer, &boss);
					adding_fireball_on_screen(player.move, difficulty, fireball, &boss, &player);
					char text2[256];
					sprintf(text2, "Health %d", health_boss);
					draw_health_boss(text2, renderer, font, color_health, &boss);
					draw_fireball(renderer, fireball);
					if (boss.health <= 0)
					{
						boss.isAlive = false;
						player.score += 100;
						printf("You win! All enemies are defeated.\n");
						running = false;
						game_over = false;
						winning = true;
					}
				}
				break;
			case HARD:
				if (enemy_counter == 100)
				{
					// Boss
					boss.isAlive = true;
					draw_boss(renderer, &boss);
					adding_fireball_on_screen(player.move, difficulty, fireball, &boss, &player);
					char text2[256];
					sprintf(text2, "Health %d", health_boss);
					draw_health_boss(text2, renderer, font, color_health, &boss);
					draw_fireball(renderer, fireball);
					if (boss.health <= 0)
					{
						boss.isAlive = false;
						player.score += 100;
						printf("You win! All enemies are defeated.\n");
						running = false;
						game_over = false;
						winning = true;
					}
				}
				break;
			default:
				break;
			}

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (game_over)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 395 && mouseX < 740 && mouseY > 480 && mouseY < 580)
						{
							menu = Main;
							change_screen(mouseX, mouseY, menu, &game_over);
							isRunning = true;
							running2 = false;
							winning = false;
						}
						else if (mouseX > 755 && mouseX < 1110 && mouseY > 480 && mouseY < 580)
						{
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							game_over = false;
							winning = false;
							running2 = false;
							isRunning = false;
							running = true;
						}
					}
					break;
				}
			}

			game_over_screen(renderer);
			int scores = player.score;
			char text2[256];
			sprintf(text2, "%d", scores);
			show_score(text2, renderer, font_showscore, color);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (winning)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 295 && mouseX < 740 && mouseY > 515 && mouseY < 615)
						{
							menu = Main;
							change_screen(mouseX, mouseY, menu, &winning);
							isRunning = true;
							running2 = false;
						}
						else if (mouseX > 755 && mouseX < 1110 && mouseY > 515 && mouseY < 615)
						{
							initialize_game(&move, character, &difficulty, &player, &playerBullet, enemy_array, &boss, &enemy_counter, &enemy_counter2);
							winning = false;
							game_over = false;
							running2 = false;
							isRunning = false;
							running = true;
						}
					}
					break;
				}
			}

			winning_screen(renderer);
			int scores = player.score;
			char text2[256];
			sprintf(text2, "%d", scores);
			show_score(text2, renderer, font_showscore, color);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}

		while (running2)
		{
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				// Handles OS "Exit" event
				case SDL_QUIT:
					running = false;
					running2 = false;
					isRunning = false;
					run_whole = false;
					difficulty_screen = false;
					character_selection = false;
					game_over = false;
					winning = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					if (event.button.button == SDL_BUTTON_LEFT)
					{
						int mouseX = event.motion.x;
						int mouseY = event.motion.y;

						change_music(mouseX, mouseY, &currentSongIndex, musics, &music);
						if (mouseX > 540 && mouseX < 945 && mouseY > 160 && mouseY < 270)
						{
							menu = Resume;
							change_screen(mouseX, mouseY, menu, &running2);
							running = true;
						}
						else if (mouseX > 540 && mouseX < 945 && mouseY > 295 && mouseY < 375)
						{
							menu = Main;
							change_screen(mouseX, mouseY, menu, &running2);
							isRunning = true;
						}
						else if (mouseX > 540 && mouseX < 945 && mouseY > 440 && mouseY < 520)
						{
							menu = Exit;
							change_screen(mouseX, mouseY, menu, &running2);
							running = false;
							running2 = false;
							isRunning = false;
							run_whole = false;
							difficulty_screen = false;
							game_over = false;
							winning = false;
						}
					}
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
					{
						running2 = false;
						running = true;
						isRunning = false;
					}
				}
			}

			// Pause
			pause_screen(renderer);

			// Present Render to screen
			SDL_RenderPresent(renderer);
		}
	}

	// Free music when done
	if (music != NULL)
	{
		Mix_FreeMusic(music);
	}

	// Cleanup
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	Mix_FreeChunk(audio_zombie);
	Mix_FreeChunk(bullet_effect);

	TTF_CloseFont(font);
	TTF_CloseFont(font_score);
	TTF_CloseFont(font_showscore);
	TTF_Quit();
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();

	return 0;
}