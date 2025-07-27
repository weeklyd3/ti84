#include <ti/getcsc.h>
#include <ti/screen.h>
#include <ti/real.h>
#include <sys/timers.h>
#include <time.h>
#include <ti/real.h>
#include <sys/util.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>

void float2str(float value, char *str)
{
	real_t tmp_real = os_FloatToReal(value < 0.001f ? 0.0f : value);
	os_RealToStr(str, &tmp_real, 8, 1, 2);
}

int (*new_grid(int rows))[8] {
	int (*arr)[8] = (int (*)[8])malloc(rows * sizeof(int[8]));
	if (arr == NULL) {
		exit(1);
	}
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < 8; j++) {
			arr[i][j] = 0;
		}
	}
	return arr;
}
struct ship {
	int id;
	int size;
	bool rotated;
	int x;
	int y;
	bool dead;
	int placed;
};
void draw_grid(int board[8][8], int hits[8][8], int shots[8][8], int who, int ships_to_place, struct ship ships[5], bool player_switcher) {
	os_SetCursorPos(8, 0);
	os_PutStrFull("ABCDEFGH");
	os_SetCursorPos(8, 18);
	os_PutStrFull("ABCDEFGH");
	char bottom[9];
	sprintf(bottom, "Player %d", who);
	os_SetCursorPos(8, 9);
	os_PutStrFull(bottom);
	if (player_switcher) {
		os_SetCursorPos(0, 0);
		char line_1[27];
		sprintf(line_1, "  Pass this to player %d!  ", who);
		os_PutStrFull(line_1);
		os_SetCursorPos(1, 0);
		os_PutStrFull("Then have them press ENTER");
		return;
	}
	os_SetCursorPos(0, 0);
	char buffer[2];
	for (int i = 0; i < 8; i++) {
		os_SetDrawBGColor(55);
		os_SetCursorPos(i, 8);
		sprintf(buffer, "%d", i + 1);
		os_PutStrFull(buffer);
		os_SetCursorPos(i, 17);
		os_PutStrFull(buffer);
		for (int j = 0; j < 8; j++) {
			if (who == 1) os_SetCursorPos(i, j);
			else os_SetCursorPos(i, j + 18);
			if (board[i][j] == 0) {
				os_PutStrFull(".");
			} else {
				sprintf(buffer, "%d", board[i][j]);
				os_PutStrFull(buffer);
			}
			if (who == 1) os_SetCursorPos(i, j);
			else os_SetCursorPos(i, j + 18);
			if (shots[i][j] != 0) {
				if (board[i][j] == 0) os_PutStrFull("m");
				else os_PutStrFull("X");
			}
			if (who == 2) os_SetCursorPos(i, j);
			else os_SetCursorPos(i, j + 18);
			if (hits[i][j] == 0) {
				os_SetDrawBGColor(55);
				os_PutStrFull("?");
			} else {
				os_SetDrawBGColor(55);
				if (hits[i][j] != -1) os_PutStrFull("X");
				else os_PutStrFull("m");
			}
		}
	}
	int first_ship_to_place = 5 - ships_to_place;
	int left_offset = 0;
	if (who == 2) left_offset = 18;
	char id[2];
	for (int i = 0; i <= first_ship_to_place; i++) {
		if (i == 5) break;
		if (i != first_ship_to_place) continue;
		struct ship item = ships[i];
		os_SetCursorPos(item.y, item.x + left_offset);
		int current_x = item.x;
		int current_y = item.y;
		sprintf(id, "%d", item.id);
		for (int j = 0; j < item.size; j++) {
			os_PutStrFull(id);
			if (item.rotated) current_y += 1;
			else current_x += 1;
			os_SetCursorPos(current_y, current_x + left_offset);
		}
	}
}

int player = 1;
int player_cursor_x = 0;
int player_cursor_y = 0;
int p1_ships_to_place = 5;
int p2_ships_to_place = 5;
int shots_left = 5;
bool player_switcher = 0;
int winner = 0;

int main(void) {
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Welcome! Here you can play Battleship. Remember that only the player number in the middle of the screen should be looking at a particular time.");
	os_SetCursorPos(7, 0);
	os_PutStrFull("Press any key...");

	int (*player_1)[8] = new_grid(8);
	int (*player_2)[8] = new_grid(8);
	int (*player_1_hits)[8] = new_grid(8);
	int (*player_2_hits)[8] = new_grid(8);
	struct ship player_1_ships[5];
	struct ship player_2_ships[5];
	for (int i = 0; i < 2; i++) {
		for (int j = 1; j < 6; j++) {
			int size = j;
			if (size == 2) size = 3;
			if (size == 1) size = 2;
			struct ship to_add;
			to_add.size = size;
			to_add.x = 0;
			to_add.y = 0;
			to_add.id = j;
			to_add.dead = false;
			to_add.rotated = 0;
			if (i == 0) player_1_ships[j - 1] = to_add;
			else player_2_ships[j - 1] = to_add;
		}
	}

	while (!os_GetCSC());
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("How to Play");
	os_SetCursorPos(1, 0);
	os_PutStrFull("You will see two grids. The one with dots is your ocean, the one with question marks is your hits.");
	os_SetCursorPos(8, 0);
	os_PutStrFull("Press any key...");
	while (!os_GetCSC());
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("How to Play");
	os_SetCursorPos(1, 0);
	os_PutStrFull("Use the arrow keys to aim and hit a spot on your opponent's grid. X marks hits, m marks miss.");
	os_SetCursorPos(5, 0);
	os_PutStrFull("During the game, you can press 9 to bail out of this program.");
	os_SetCursorPos(8, 0);
	os_PutStrFull("Press any key...");
	while (!os_GetCSC());
	int active_ships_to_place;
	struct ship* active_ships;
	int(* active_grid)[8];
	int(* inactive_grid)[8];
	int(* active_hits)[8];
	int(* inactive_hits)[8];
	bool is_valid = true;
	while (true) {
		if (player == 1) {
			active_ships_to_place = p1_ships_to_place;
			active_ships = player_1_ships;
			active_grid = player_1;
			inactive_grid = player_2;
			active_hits = player_1_hits;
			inactive_hits = player_2_hits;
		} else {
			active_ships_to_place = p2_ships_to_place;
			active_ships = player_2_ships;
			active_grid = player_2;
			inactive_grid = player_1;
			active_hits = player_2_hits;
			inactive_hits = player_1_hits;
		}
		os_ClrHome();

		if (winner != 0) {
			char grid_number[9];
			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 8; j++) {
					os_SetCursorPos(i, j);
					if (player_1[i][j] != 0) {
						sprintf(grid_number, "%d", player_1[i][j]);
						os_PutStrFull(grid_number);
					} else os_PutStrFull(".");
					os_SetCursorPos(i, j);
					if (player_1_hits[i][j] != 0) {
						if (player_1_hits[i][j] == -1) os_PutStrFull("m");
						else os_PutStrFull("X");
					}

					os_SetCursorPos(i, j + 18);
					if (player_2[i][j] != 0) {
						sprintf(grid_number, "%d", player_2[i][j]);
						os_PutStrFull(grid_number);
					} else os_PutStrFull(".");
					os_SetCursorPos(i, j + 18);
					if (player_2_hits[i][j] != 0) {
						if (player_2_hits[i][j] == -1) os_PutStrFull("m");
						else os_PutStrFull("X");
					}
				}
			}
			sprintf(grid_number, "P%d wins!", winner);
			os_SetCursorPos(0, 9);
			os_PutStrFull(grid_number);
			os_SetCursorPos(2, 8);
			os_PutStrFull(" Press any");
			os_SetCursorPos(3, 8);
			os_PutStrFull(" key to ");
			os_SetCursorPos(4, 8);
			os_PutStrFull(" exit!");
			os_SetCursorPos(8, 0);
			os_PutStrFull("github.com/weeklyd3/ti84");
			while (!os_GetCSC());
			return 0;
		}

		draw_grid(active_grid, inactive_hits, active_hits, player, active_ships_to_place, active_ships, player_switcher);

		struct ship* to_place;
		if (active_ships_to_place > 0 && !player_switcher) {
			is_valid = true;
			to_place = &active_ships[5 - active_ships_to_place];
			if (!to_place->rotated && (to_place->x + to_place->size) > 8) is_valid = false;
			if (to_place->rotated && (to_place->y + to_place->size) > 8) is_valid = false;
			if (is_valid) {
				int current_x = to_place->x;
				int current_y = to_place->y;
				for (int i = 0; i < to_place->size; i++) {
					if (active_grid[current_y][current_x] != 0) {
						is_valid = false;
						break;
					}
					if (!to_place->rotated) current_x += 1;
					else current_y += 1;
				}
			}
			if (is_valid) {
				os_SetCursorPos(0, 9);
				os_PutStrFull(" ENTER= ");
				os_SetCursorPos(1, 9);
				os_PutStrFull("Confirm ");
			} else {
				os_SetCursorPos(0, 9);
				os_PutStrFull("INVALID!");
				os_SetCursorPos(1, 9);
				os_PutStrFull("INVALID!");
			}
			os_SetCursorPos(2, 9);
			os_PutStrFull(" ARROW= ");
			os_SetCursorPos(3, 9);
			os_PutStrFull("  MOVE  ");
			os_SetCursorPos(4, 9);
			os_PutStrFull("  2ND=  ");
			os_SetCursorPos(5, 9);
			os_PutStrFull(" ROTATE ");
			if (active_ships_to_place < 5) {
				os_SetCursorPos(6, 9);
				os_PutStrFull(" CLEAR= ");
				os_SetCursorPos(7, 9);
				os_PutStrFull("Nuke it!");
			}
		}
		if (active_ships_to_place == 0 && !player_switcher) {
			os_SetCursorPos(0, 9);
			os_PutStrFull(" Target ");
			os_SetCursorPos(1, 12);
			char column[2];
			switch (player_cursor_x) {
				case 0:
					strcpy(column, "A");
					break;
				case 1:
					strcpy(column, "B");
					break;
				case 2:
					strcpy(column, "C");
					break;
				case 3:
					strcpy(column, "D");
					break;
				case 4:
					strcpy(column, "E");
					break;
				case 5:
					strcpy(column, "F");
					break;
				case 6:
					strcpy(column, "G");
					break;
				case 7:
					strcpy(column, "H");
					break;
			}
			os_PutStrFull(column);
			os_SetCursorPos(1, 13);
			sprintf(column, "%d", player_cursor_y + 1);
			os_PutStrFull(column);
			os_SetCursorPos(2, 9);
			os_PutStrFull(" ENTER= ");
			os_SetCursorPos(3, 9);
			os_PutStrFull("  FIRE");
			sprintf(column, "%d", shots_left);
			os_SetCursorPos(6, 9);
			os_PutStrFull(" Shots:");
			os_SetCursorPos(7, 13);
			os_PutStrFull(column);
		}

		uint8_t key;
		while (true) {
			key = os_GetCSC();
			if (key) break;
		}
		if (key == sk_9) return 1;
		if (player_switcher && key == sk_Enter) {
			player_switcher = false;
			shots_left = 0;
			if (active_ships_to_place == 0) {
				player_cursor_x = 0;
				player_cursor_y = 0;
				struct ship* check_ship;
				for (int i = 0; i < 5; i++) {
					check_ship = &active_ships[i];
					int current_x = check_ship->x;
					int current_y = check_ship->y;
					check_ship->dead = true;
					for (int j = 0; j < check_ship->size; j++) {
						if (active_hits[current_y][current_x] == 0) {
							check_ship->dead = false;
							shots_left += 1;
							break;
						}
						if (check_ship->rotated) current_y += 1;
						else current_x += 1;
					}
				}
				if (shots_left == 0) {
					if (player == 2) winner = 1;
					else winner = 2;
				}
			}
			continue;
		}
		if (player_switcher) continue;
		if (active_ships_to_place == 0) {
			if (key == sk_Down) {
				player_cursor_y += 1;
			}
			if (key == sk_Up) {
				player_cursor_y -= 1;
			}
			if (key == sk_Left) {
				player_cursor_x -= 1;
			}
			if (key == sk_Right) {
				player_cursor_x += 1;
			}
			if (player_cursor_x < 0) player_cursor_x = 0;
			if (player_cursor_y < 0) player_cursor_y = 0;
			if (player_cursor_x > 7) player_cursor_x = 7;
			if (player_cursor_y > 7) player_cursor_y = 7;

			if (key == sk_Enter) {
				// FIRE!!!
				if (shots_left <= 0) {
					player_switcher = true;
					if (player == 2) player = 1;
					else player = 2;
					continue;
				}
				if (inactive_hits[player_cursor_y][player_cursor_x] != 0) {
					// hold up, you already fired here
					// but i'm still punishing you for it.
					shots_left -= 1;
				} else {
					inactive_hits[player_cursor_y][player_cursor_x] = inactive_grid[player_cursor_y][player_cursor_x];
					if (inactive_hits[player_cursor_y][player_cursor_x] == 0) inactive_hits[player_cursor_y][player_cursor_x] = -1;
					shots_left -= 1;
				}
			}
		}
		if (active_ships_to_place > 0 && !player_switcher) {
			if (key == sk_Down) {
				to_place->y += 1;
			} else if (key == sk_Up) {
				to_place->y -= 1;
			} else if (key == sk_Left) {
				to_place->x -= 1;
			} else if (key == sk_Right) {
				to_place->x += 1;
			} else if (key == sk_2nd) {
				if (!to_place->rotated) to_place->rotated = 1;
				else to_place->rotated = 0;
			}
			if (key == sk_Clear) {
				if (player == 1) p1_ships_to_place = 5;
				else p2_ships_to_place = 5;
				for (int i = 0; i < 8; i++) {
					for (int j = 0; j < 8; j++) {
						active_grid[i][j] = 0;
					}
				}
			}
			if (key == sk_Down || key == sk_Up ||
			    key == sk_Left || key == sk_Right) {
				if (to_place->x < 0) to_place->x = 0;
				if (to_place->x > 7) to_place->x = 7;
				if (to_place->y < 0) to_place->y = 0;
				if (to_place->y > 7) to_place->y = 7;
				continue;
			}
			if (is_valid && (key == sk_Enter)) {
				int current_x = to_place->x;
				int current_y = to_place->y;
				for (int i = 0; i < to_place->size; i++) {
					if (player == 1) player_1[current_y][current_x] = to_place->id;
					else player_2[current_y][current_x] = to_place->id;
					if (!to_place->rotated) current_x += 1;
					else current_y += 1;
				}

				if (player == 1) {
					p1_ships_to_place -= 1;
					if (p1_ships_to_place == 0) {
						player_switcher = true;
						player = 2;
					}
				} else {
					p2_ships_to_place -= 1;
					if (p2_ships_to_place == 0) {
						player_switcher = true;
						player = 1;
					}
				}
			}
		}
	}

}