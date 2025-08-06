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
// keeps crashing and idk why
// try seed 5 and press enter to make the dealer draw 14 cards
struct card {
	int suit;
	int number;
	int value;
	char indicated[2];
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
	bool valid;
};
void shuffle(struct card arr[52], int size) {
	// Seed the random number generator with current time
	//srand(time(NULL));
	
	// Fisher-Yates shuffle
	for (int i = size - 1; i > 0; i--) {
		// Generate random index between 0 and i
		int j = rand() % (i + 1);
		
		// Swap elements at i and j
		struct card temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
}
void draw(struct card from[52], struct card receiver[52], struct card null_card) {
	int i = 0;
	while (!from[i].valid) i += 1;
	struct card target = from[i];
	int j = 0;
	while (receiver[j].valid) j += 1;
	receiver[j] = target;
	dbg_printf("Took card suit %d num %s\n", target.suit, target.indicated);
	from[i] = null_card;
}
void draw_item(struct card item, int x, int y) {
	if (!item.valid) return;
	gfx_SetColor(255);
	gfx_FillRectangle(x, y, 39, 25);
	gfx_SetColor(0);
	gfx_Rectangle(x, y, 39, 25);
	if (item.suit < 2) gfx_SetTextFGColor(224);
	else gfx_SetTextFGColor(0);
	char string[8];
	char string2[7];
	char suit[2];
	if (item.suit == 0) strcpy(suit, "D");
	if (item.suit == 1) strcpy(suit, "H");
	if (item.suit == 2) strcpy(suit, "C");
	if (item.suit == 3) strcpy(suit, "S");
	sprintf(string,  "%s     %s", item.indicated, item.indicated);
	sprintf(string2, "%s +%d ", suit, item.value);
	gfx_PrintStringXY(string, x + 3, y + 3);
	gfx_PrintStringXY(string2, x + 3, y + 15);
}
void draw_deck(struct card deck[52], int x, int y) {
	int index = 0;
	for (int i = 0; i < 52; i++) {
		if (!deck[i].valid) continue;
		// it keeps hanging after reading garbage data from a (prolly uninitialized) null card so
		if (deck[i].suit > 3 || deck[i].suit < 0 || deck[i].number < 0 || deck[i].number >= 13) continue;
		dbg_printf("Draw card suit %d num %s number %d valid %d index %d\n", deck[i].suit, deck[i].indicated, deck[i].number, deck[i].valid, index);
		int item_x = x + 39 * (index % 4);
		int item_y = y - (index - index % 4) / 4 * 25 - 25;
		draw_item(deck[i], item_x, item_y);
		index += 1;
	}
	dbg_printf("%d items drawn\n", index);
}
int main(void) {
	struct card deck[52];
	struct card all[52];
	struct card dealer[52];
	struct card p1[52];
	struct card p2[52];
	struct card null_card;
	null_card.valid = false;
	null_card.suit = -1;
	null_card.number = -1;
	null_card.hidden = false;
	strcpy(null_card.indicated, "?");
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 13; j++) {
			struct card current;
			current.suit = i;
			current.number = j;
			current.hidden = false;
			current.valid = true;
			strcpy(current.indicated, "?");
			if (j >= 0 && j < 9) {
				current.value = j + 2;
				sprintf(current.indicated, "%d", j + 2);
			} else if (j >= 9 && j < 12) {
				current.value = 10;
				if (j == 9) strcpy(current.indicated, "J");
				if (j == 10) strcpy(current.indicated, "Q");
				if (j == 11) strcpy(current.indicated, "K");
			} else {
				current.value = 11;
				strcpy(current.indicated, "A");
			}
			dbg_printf("Adding card suit %d letter %s number %d\n", current.suit, current.indicated, current.number);
			deck[13 * i + j] = current;
			all[13 * i + j] = current;
			dealer[13 * i + j] = null_card;
			p1[13 * i + j] = null_card;
			p2[13 * i + j] = null_card;
		}
	}
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Play blackjack here. del or 9 bails out.");
	os_SetCursorPos(2, 0);
	os_PutStrFull("Press any key...");
	while (!os_GetCSC());

	char rounds[5];
	char seed_char[10];
	char* end;
	int roundCount;
	int seed;
	os_SetCursorPos(3, 0);
	os_PutStrFull("Number of rounds (0 quits):");
	os_SetCursorPos(4, 0);
	os_GetStringInput("> ", rounds, 4);
	roundCount = strtol(rounds, &end, 10);
	if (roundCount == 0) return 1;
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
	while (true) {
		gfx_SetColor(31);
		gfx_FillRectangle(0, 0, 159, 239);
		if (players == 2) {
			gfx_FillRectangle(160, 120, 159, 119);
		}
		gfx_SetColor(0);
		gfx_HorizLine(0, 120, 329);
		gfx_VertLine(160, 0, 239);
		dbg_printf("Drew rectangles!\n");
		if (turn == 0) {
			dbg_printf("Drawing first...\n");
			draw(deck, dealer, null_card);
			dbg_printf("Drawing second...\n");
			draw(deck, dealer, null_card);
			dbg_printf("Drew both!\n");
		} else dbg_printf("Dealer draw skipped\n");
		dbg_printf("Drawing cards on screen...\n");
		draw_deck(dealer, 0, 119);
		dbg_printf("Buffer swapping...\n");
		gfx_SwapDraw();
		dbg_printf("Buffer swapped\n");
		while (true) {
			key = os_GetCSC();
			if (key) break;
		}
		dbg_printf("%d\n", key);
		if (key == sk_9 || key == sk_Del) {
			gfx_End();
			return 1;
		}
	}

	gfx_End();
}