#include <stdlib.h>
#include <string.h>
#include <ti/getcsc.h>
#include <ti/screen.h>
#include <ti/real.h>
#include <sys/timers.h>
#include <time.h>
#include <ti/real.h>
#include <sys/util.h>
#include <debug.h>
#include <graphx.h>

struct card {
	int suit;
	int number;
	int value;
	char indicated[3];
	// number = card number
	// 0 = 2
	// 1 = 3
	// ...
	// 7 = 9
	// 8 = 10
	// 9 = J
	// 10 = Q
	// 11 = K
	// 12 = A
	bool hidden;
};
void shuffle(struct card** arr, int size) {
	// Seed the random number generator with current time
	//srand(time(NULL));
	
	// Fisher-Yates shuffle
	for (int i = size - 1; i > 0; i--) {
		// Generate random index between 0 and i
		int j = rand() % (i + 1);
		
		// Swap elements at i and j
		struct card* temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
}
struct card** new_deck(bool fill) {
	// Allocate array of 52 pointers to struct card
	struct card** deck = (struct card**)malloc(sizeof(struct card*) * 52);
	dbg_printf("Deck pointer: %p\n", deck);
	if (deck == NULL) {
		gfx_End();
		dbg_printf("No memory for deck, bailing out!\n");
		exit(1);
	}

	// Initialize all pointers to NULL
	for (int i = 0; i < 52; i++) {
		deck[i] = NULL;
	}

	if (fill) {
		// Iterate over values (2 through Ace) and suits
		for (int i = 0; i < 13; i++) { // Values: 0=2, 1=3, ..., 9=J, 10=Q, 11=K, 12=A
			for (int j = 0; j < 4; j++) { // Suits: 0=clubs, 1=diamonds, 2=hearts, 3=spades
				// Allocate a new card
				struct card* to_add = (struct card*)malloc(sizeof(struct card));
				dbg_printf("Card pointer: %p\n", to_add);
				if (to_add == NULL) {
					// Clean up allocated cards and deck on failure
					for (int k = 0; k < 4 * i + j; k++) {
						free(deck[k]);
					}
					free(deck);
					gfx_End();
					dbg_printf("No space for card, bailing out!\n");
					exit(1);
				}

				// Set card properties
				to_add->suit = j;	   // Assign suit
				to_add->number = i;	 // Assign value (0-based index for 2 through Ace)
				to_add->hidden = false;
				strcpy(to_add->indicated, "?");

				// Assign value and indicated string based on card number
				if (i >= 0 && i < 9) { // 2 through 10
					to_add->value = i + 2;
					sprintf(to_add->indicated, "%d", i + 2);
				} else if (i >= 9 && i < 12) { // J, Q, K
					to_add->value = 10;
					if (i == 9) strcpy(to_add->indicated, "J");
					else if (i == 10) strcpy(to_add->indicated, "Q");
					else if (i == 11) strcpy(to_add->indicated, "K");
				} else { // Ace
					to_add->value = 11;
					strcpy(to_add->indicated, "A");
				}

				// Store the pointer in the deck
				deck[4 * i + j] = to_add;
			}
		}
	}

	return deck;
}
void draw(struct card** from, struct card** to) {
	int i = 0;
	while (from[i] == NULL) i++;
	if (i >= 52) {
		dbg_printf("Could not find card to draw from!");
		return;
	}
	struct card* target = from[i];
	from[i] = NULL;
	i = 0;
	while (to[i] != NULL) i++;
	if (i >= 52) {
		dbg_printf("Could not find card to draw from!");
		return;
	}
	dbg_printf("Drew card suit %d number %s\n", target->suit, target->indicated);
	to[i] = target;
}
void draw_item(struct card* item, int x, int y) {
	gfx_SetColor(255);
	gfx_FillRectangle(x, y, 39, 25);
	gfx_SetColor(0);
	gfx_Rectangle(x, y, 39, 25);
	if (item->suit < 2) gfx_SetTextFGColor(224);
	else gfx_SetTextFGColor(0);
	char string[11];
	char string2[7];
	char suit[2];
	if (item->suit == 0) strcpy(suit, "D");
	if (item->suit == 1) strcpy(suit, "H");
	if (item->suit == 2) strcpy(suit, "C");
	if (item->suit == 3) strcpy(suit, "S");
	if (strcmp(item->indicated, "10") == 0) 
	     strcpy(string, "10  10");
	else sprintf(string,  "%s      %s", item->indicated, item->indicated);
	sprintf(string2, "%s +%d ", suit, item->value);
	if (item->hidden) {
		strcpy(string, "FACE");
		strcpy(string2, "DOWN");
		gfx_SetTextFGColor(0);
	}
	gfx_PrintStringXY(string, x + 3, y + 3);
	gfx_PrintStringXY(string2, x + 3, y + 15);
}
void draw_deck(struct card** deck, int x, int y) {
	int index = 0;
	for (int i = 0; i < 52; i++) {
		if (deck[i] == NULL) continue;
		dbg_printf("Deck pointer: %p\n", deck[i]);
		//if (deck[i]->suit < 0 || deck[i]->suit > 3 || deck[i]->number < 0 || deck[i]->number > 12) continue;
		dbg_printf("Draw card suit %d num %s number %d index %d\n", deck[i]->suit, deck[i]->indicated, deck[i]->number, index);
		int item_x = x + 39 * (index % 4);
		int item_y = y - (index - index % 4) / 4 * 25 - 25;
		draw_item(deck[i], item_x, item_y);
		index += 1;
	}
	dbg_printf("%d items drawn\n", index);
}
int calculate_total(struct card** deck) {
	int total = 0;
	int ace_indexes[5] = {-1, -1, -1, -1, -1}; // actually 4 aces but who cares
	for (int i = 0; i < 52; i++) {
		if (deck[i] == NULL) continue;
		// reset all aces for now to get as much value as possible
		if (deck[i]->value == 1) deck[i]->value = 11;
		if (deck[i]->value == 11) {
			int j = 0;
			while (ace_indexes[j] != -1) j += 1;
			if (j >= 5) {
				dbg_printf("Whaaa... how many aces in here???\n");
			} else ace_indexes[j] = i;
		}
		total += deck[i]->value;
		if (deck[i]->hidden) return -1;
	}
	if (total > 21) {
		for (int i = 0; i < 5; i++) {
			int index = ace_indexes[i];
			if (index == -1) continue;
			deck[index]->value -= 10;
			total -= 10;
			if (total <= 21) break;
		}
	}
	return total;
}
int draw_stats(const char* name, struct card** cards, int x, int y) {
	int total = calculate_total(cards);
	gfx_SetColor(255);
	gfx_FillRectangle(x + 5, y + 5, 149, 25);
	gfx_SetColor(0);
	gfx_Rectangle(x + 5, y + 5, 149, 25);
	char status[30];
	char out[6];
	if (total > 21) strcpy(out, ", OUT");
	else strcpy(out, "");
	if (total != -1) sprintf(status, "%s, score %d%s", name, total, out);
	else sprintf(status, "%s, score hidden", name);
	gfx_SetTextFGColor(0);
	if (total > 21) gfx_SetTextFGColor(224);
	gfx_PrintStringXY(status, x + 10, y + 10);
	return total > 21;
}
int main() {
	struct card** deck = new_deck(true);
	struct card** dealer = new_deck(false);
	struct card** p1 = new_deck(false);
	struct card** p2 = new_deck(false);
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Play blackjack here. del or 9 bails out.");
	os_SetCursorPos(2, 0);
	os_PutStrFull("Press any key...");
	while (!os_GetCSC());

	char rounds[4];
	char seed_char[10];
	char* end;
	int roundCount;
	int seed;
	os_SetCursorPos(3, 0);
	os_PutStrFull("Number of rounds (0 quits):");
	os_SetCursorPos(4, 0);
	os_GetStringInput("> ", rounds, 3);
	roundCount = strtol(rounds, &end, 10);
	if (roundCount == 0) return 1;
	int total_rounds = roundCount;
	os_SetCursorPos(5, 0);
	os_PutStrFull("Seed RNG:");
	os_SetCursorPos(6, 0);
	os_GetStringInput("> ", seed_char, 9);
	seed = strtol(seed_char, &end, 10);
	srand(seed);
	shuffle(deck, 52);
	os_SetCursorPos(7, 0);
	os_PutStrFull("Press 1 or 2 to set number of players...");
	int players = 1;
	uint8_t key;
	while (true) {
		while (true) {
			key = os_GetCSC();
			if (key) break;
		}
		if (key == sk_9 || key == sk_Del) return 1;
		if (key == sk_1) break;
		if (key == sk_2) {
			players = 2;
			break;
		}
	}
	os_ClrHome();

	gfx_Begin();
	gfx_SetDrawBuffer();

	int turn = 0;
	int p1_wins = 0;
	int p2_wins = 0;
	bool score_processed = false;
	while (true) {
		gfx_SetColor(255);
		gfx_FillRectangle(0, 0, 329, 239);
		gfx_SetColor(31);
		gfx_FillRectangle(0, 0, 159, 239);
		if (players == 2) {
			gfx_FillRectangle(160, 120, 159, 119);
		}
		gfx_SetColor(0);
		gfx_HorizLine(0, 120, 329);
		gfx_VertLine(160, 0, 239);
		dbg_printf("Drew rectangles!\n");
		gfx_SetTextFGColor(0);
		if (turn == 3) dealer[1]->hidden = false;
		if (turn == 0) {
			dbg_printf("Drawing first...\n");
			draw(deck, dealer);
			draw(deck, p1);
			draw(deck, p2);
			dbg_printf("Drawing second...\n");
			draw(deck, dealer);
			draw(deck, p1);
			draw(deck, p2);
			dealer[1]->hidden = true;
			dbg_printf("Drew both!\n");
			turn = 1;
		} else dbg_printf("Dealer draw skipped\n");

		int player_score;
		int player_1_score = calculate_total(p1);
		int player_2_score = calculate_total(p2);
		int dealer_score = calculate_total(dealer);
		if (turn == 1) player_score = player_1_score;
		else if (turn == 2) player_score = player_2_score;
		else player_score = dealer_score;

		if (turn == 3) {
			while (player_score < 17) {
				draw(deck, dealer);
				player_score = calculate_total(dealer);
				dealer_score = player_score;
			}
		}

		dbg_printf("Drawing cards on screen...\n");
		draw_stats("Dealer", dealer, 0, 0);
		draw_stats("P1", p1, 0, 120);
		if (players == 2) draw_stats("P2", p2, 160, 120);
		dbg_printf("Drawing cards...\n");
		draw_deck(dealer, 0, 119);
		draw_deck(p1, 0, 239);
		if (players == 2) draw_deck(p2, 160, 239);

		char line[27];
		gfx_SetTextFGColor(0);
		if (turn != 3) {
			sprintf(line, "P%d's turn, %d rounds left", turn, roundCount);
			gfx_PrintStringXY(line, 162, 2);
			sprintf(line, "Your total is %d", player_score);
			gfx_PrintStringXY(line, 162, 15);
			if (player_score > 21) strcpy(line, "You lost!");
			else strcpy(line, "");
			gfx_PrintStringXY(line, 162, 28);
			if (player_score > 21) strcpy(line, "Enter to continue");
			else strcpy(line, "[Enter] to draw a card");
			gfx_PrintStringXY(line, 162, 41);
			if (player_score > 21) strcpy(line, "");
			else strcpy(line, "[+] to end turn");
			gfx_PrintStringXY(line, 162, 54);
		} else {
			if (!score_processed) roundCount -= 1;

			sprintf(line, "Dealer, %d rounds left", roundCount);
			gfx_PrintStringXY(line, 162, 2);
			sprintf(line, "Dealer's total is %d", player_score);
			gfx_PrintStringXY(line, 162, 15);
			if (player_score > 21) strcpy(line, "Dealer lost!");
			else strcpy(line, "Round ended!");
			gfx_PrintStringXY(line, 162, 28);

			int p1_won;
			int p2_won;
			if (dealer_score > 21) {
				// anyone who didnt go over 21 wins
				p1_won = player_1_score <= 21;
				p2_won = player_2_score <= 21;
			} else {
				// anyone who got higher than the dealer
				// but not higher than 21 wins
				p1_won = (dealer_score < player_1_score) && (player_1_score <= 21);
				p2_won = (dealer_score < player_2_score) && (player_2_score <= 21);
			}
			if (!score_processed) {
				p1_wins += p1_won;
				p2_wins += p2_won;
				score_processed = true;
			}

			if (p1_won && !p2_won) strcpy(line, "P1 wins!");
			else if (p1_won && p2_won) strcpy(line, "P1 and P2 win!");
			else if (!p1_won && p2_won) strcpy(line, "P2 wins!");
			else strcpy(line, "No one wins!");
			
			if (players == 1 && p1_won) strcpy(line, "You win!");
			if (players == 1 && !p1_won) strcpy(line, "You lose!");
			gfx_PrintStringXY(line, 162, 43);
			strcpy(line, "[2nd] to continue");
			gfx_PrintStringXY(line, 162, 54);
			gfx_PrintStringXY("(PLEASE BE PATIENT!!!)", 162, 69);
		}

		gfx_PrintStringXY("[del] or [9] to bail out", 162, 82);
		char wins[30];
		char p1_wins_string[15];
		char p2_wins_string[15];
		sprintf(p1_wins_string, "P1 %d", p1_wins);
		if (players == 2) sprintf(p2_wins_string, ", P2 %d", p2_wins);
		else strcpy(p2_wins_string, "");
		sprintf(wins, "Wins: %s %s", p1_wins_string, p2_wins_string);
		gfx_PrintStringXY(wins, 162, 95);

		dbg_printf("Buffer swapping...\n");
		gfx_SwapDraw();
		dbg_printf("Buffer swapped\n");
		while (true) {
			key = os_GetCSC();
			if (key) {
				dbg_printf("%d\n", key);
				if (key == sk_9 || key == sk_Del) {
					gfx_End();
					return 1;
				} else if ((turn == 1 || turn == 2) && key == sk_Enter) {
					if (player_score > 21) {
						turn += 1;
						if (players == 1) turn += 1;
					} else {
						// draw a card
						if (turn == 1) draw(deck, p1);
						else draw(deck, p2);
					}
					break;
				} else if ((turn == 1 || turn == 2) && key == sk_Add) {
					turn += 1;
					if (players == 1) turn += 1;
					break;
				} else if (turn == 3 && key == sk_2nd) {
					// return all cards
					for (int i = 0; i < 52; i++) {
						draw(p1, deck);
						draw(p2, deck);
						draw(dealer, deck);
					}
					shuffle(deck, 52);
					turn = 0;
					score_processed = false;
					break;
				}
			}
		}
		if (roundCount <= 0) break;
	}
	gfx_End();
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Deleting cards from memory...");
	for (int i = 0; i < 52; i++) {
		if (deck[i] == NULL) continue;
		free(deck[i]);
	}
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("GAME ENDED");
	os_SetCursorPos(2, 0);
	char summary_line[50];
	char p2_wins_summary[17];
	sprintf(p2_wins_summary, ", and P2 won %d", p2_wins);
	if (players == 1) strcpy(p2_wins_summary, "");
	sprintf(summary_line, "Out of %d rounds, P1 won %d%s.", total_rounds, p1_wins, p2_wins_summary);
	os_PutStrFull(summary_line);
	os_SetCursorPos(7, 0);
	os_PutStrFull("github.com/weeklyd3/ti84");
	os_SetCursorPos(8, 0);
	os_PutStrFull("Press any key to exit...");
	while (!os_GetCSC());
}