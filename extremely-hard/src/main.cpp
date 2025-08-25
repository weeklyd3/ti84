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
#include <fileioc.h>
#include <keypadc.h>
#define LIST_SIZE 14

bool rect_rect_collision(float x1, float y1, float w1, float h1, 
	       float x2, float y2, float w2, float h2) {
	if (x1 + w1 < x2 || x2 + w2 < x1) return false;
	if (y1 + h1 < y2 || y2 + h2 < y1) return false;
	return true;
}
bool rect_point_collision(float x1, float y1, float w1, float h1,
			  float x2, float y2) {
	return (x1 <= x2 && (x2 <= (x1 + w1))) && (y1 <= y2 && (y2 <= (y1 + h1)));
}
bool circle_circle_collision(float x1, float y1, float r1,
			     float x2, float y2, float r2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float distanceSquared = dx * dx + dy * dy;

	return distanceSquared <= (r1 + r2) * (r1 + r2);
}
bool rect_circle_collision(float x1, float y1, float w1, float h1,
			   float x2, float y2, float r) {
	// not perfect for corners but the screen doesn't
	// have enough pixels for me to care
	return rect_point_collision(x1 - r, y1 - r, w1 + 2 * r, h1 + 2 * r, x2, y2);
}
float max(float one, float two) {
	return one > two ? one : two;
}
float min(float one, float two) {
	return one < two ? one : two;
}

class player {
	public:
	float x;
	float y;
	float previous_x;
	float previous_y;
	int radius;
	int display_radius;
	int deaths;
	float speed;
	float win_area[4];
	float gravity_center[2];
	float gravity_magnitude;
	void reset() {
		this->x = 10;
		this->y = 10;
	}
	void die() {
		this->reset();
		this->deaths += 1;
	}
	player() {
		this->gravity_center[0] = 0;
		this->gravity_center[1] = 0;
		this->gravity_magnitude = 0;
	}
};

void resolveCollisionStatic(player* user, float x, float y, float w, float h) {
	// Find the closest point on the rectangle to the circle's center
	float px = max(x, min(user->x, x + w));
	float py = max(y, min(user->y, y + h));

	// Displacement vector from closest point to circle's center
	float dx = user->x - px;
	float dy = user->y - py;
	real_t distance_squared = os_FloatToReal(dx * dx + dy * dy);
	distance_squared = os_RealSqrt(&distance_squared);
	float distance = os_RealToFloat(&distance_squared);

	// If intersecting (distance < radius), move the circle out
	if (distance < user->radius && distance > 0.0001) {
		// Avoid division by zero
		float scale = (user->radius - distance) / distance;
		// Move the circle along the displacement vector
		user->x += dx * scale;
		user->y += dy * scale;
	}
}

class object {
	public:
	float x;
	float y;
	float vx;
	float vy;
	float min_x;
	float max_x;
	float min_y;
	float max_y;
	bool circular;
	float size_x;
	float size_y;
	int color;
	void physics(player* user) {
		this->x += this->vx;
		this->y += this->vy;
		if (this->x <= this->min_x || this->x >= this->max_x) this->vx = -this->vx;
		if (this->y <= this->min_y || this->y >= this->max_y) this->vy = -this->vy;
	}
	bool collides_with_player(player* user) {
		if (this->circular) return circle_circle_collision(
			this->x, this->y, this->size_x,
			user->x, user->y, user->radius
		);
		return rect_circle_collision(
			this->x, this->y, this->size_x, this->size_y, 
			user->x, user->y, user->radius
		);
	}
	object(float x, float y, float vx, float vy, bool circular, float size_x, float size_y = 0) {
		this->x = x;
		this->y = y;
		this->vx = vx;
		this->vy = vy;
		this->min_x = 0;
		this->min_y = 0;
		this->max_x = 319 - size_x;
		this->max_y = 209 - size_y;
		if (circular) this->max_y = 209 - size_x;
		this->circular = circular;
		// when circular, size_x refers to radius
		// otherwise width
		this->size_x = size_x;
		this->size_y = size_y;
		this->color = 0;
	}
	void draw() {
		gfx_SetColor(this->color);
		if (this->circular) {
			gfx_FillCircle(this->x, this->y, this->size_x);
		} else {
			gfx_FillRectangle(this->x, this->y, this->size_x, this->size_y);
		}
	}
};
class lava: public object {
	public:
	void physics(player* user) {
		object::physics(user);
		if (this->collides_with_player(user)) user->die();
	}
	lava(float x, float y, float vx, float vy, bool circular, float size_x, float size_y = 0): object(x, y, vx, vy, circular, size_x, size_y) {
		this->color = 224;
		// do i really have to copy paste these arguments every time i make one of these classes???
		// but yea i have nothing else to initialize
	}
};
class accelerator: public object {
	public:
	float induced_vx;
	float induced_vy;
	void physics(player* user) {
		object::physics(user);
		if (this->collides_with_player(user)) {
			user->x += this->induced_vx;
			user->y += this->induced_vy;
		}
	}
	accelerator(float x, float y, float vx, float vy, bool circular, float induced_vx, float induced_vy, float size_x, float size_y = 0): object(x, y, vx, vy, circular, size_x, size_y) {
		this->color = 24;
		// do i really have to copy paste these arguments every time i make one of these classes???
		// but yea i have nothing else to initialize
		this->induced_vx = induced_vx;
		this->induced_vy = induced_vy;
	}
};
class wall: public object {
	public:
	float induced_vx;
	float induced_vy;
	void physics(player* user) {
		object::physics(user);
		if (this->collides_with_player(user)) {
			resolveCollisionStatic(user, this->x, this->y, this->size_x, this->size_y);
		}
	}
	wall(float x, float y, float vx, float vy, float size_x, float size_y = 0): object(x, y, vx, vy, false, size_x, size_y) {
		this->color = 32;
		// do i really have to copy paste these arguments every time i make one of these classes???
		// but yea i have nothing else to initialize
	}
};
template <typename T>
void fill_array(T** array, int size, bool delete_entries = false) {
	for (int i = 0; i < size; i++) {
		if (delete_entries && array[i] != nullptr) {
			delete array[i];
		}
		array[i] = nullptr;
	}
}
template <typename T>
void draw_list(T** array, int size, player* user) {
	for (int i = 0; i < size; i++) {
		if (array[i] != nullptr) {
			array[i]->physics(user);
			array[i]->draw();
		}
	}
}
bool load_level(int level, player* user, char* help, lava** lava_list, accelerator** accelerator_list, wall** wall_list) {
	user->reset();
	fill_array(lava_list, LIST_SIZE, true);
	fill_array(accelerator_list, LIST_SIZE, true);
	fill_array(wall_list, LIST_SIZE, true);
	switch (level) {
		case -1:
		lava_list[0] = new lava(50, 50, 0, 0, false, 50, 50);
		accelerator_list[0] = new accelerator(50, 0, 0, 0, false, 5, 0, 100, 40);
		wall_list[0] = new wall(200, 0, 0, 0, 50, 100);
		return true;
		case 1:
		strcpy(help, "Go to the green area. Try not to die!");
		lava_list[0] = new lava(150, 111, 0, 0, false, 25, 25);
		lava_list[1] = new lava(37, 0, 0, 0, false, 37, 116);
		lava_list[2] = new lava(101, 25, 0, 0, false, 37, 111);
		accelerator_list[0] = new accelerator(110, 185, 0, 0, false, 5, 0, 175, 25);
		accelerator_list[1] = new accelerator(110, 148, 0, 0, false, -5, 0, 175, 25);
		accelerator_list[2] = new accelerator(175, 111, 0, 0, false, -0.5, -1, 40, 37);
		wall_list[0] = new wall(25, 0, 0, 0, 12, 175);
		wall_list[1] = new wall(25, 173, 0, 0, 50, 12);
		wall_list[2] = new wall(63, 136, 0, 0, 12, 37);
		wall_list[3] = new wall(75, 136, 0, 0, 100, 12);
		wall_list[4] = new wall(138, 111, 0, 0, 12, 25);
		wall_list[5] = new wall(138, 99, 0, 0, 157, 12);
		wall_list[6] = new wall(215, 136, 0, 0, 105, 12);
		wall_list[7] = new wall(100, 173, 0, 0, 195, 12);
		wall_list[8] = new wall(138, 37, 0, 0, 12, 62);
		wall_list[9] = new wall(175, 62, 0, 0, 145, 12);
		wall_list[10] = new wall(138, 25, 0, 0, 157, 12);
		user->win_area[0] = 37;
		user->win_area[1] = 136;
		user->win_area[2] = 26;
		user->win_area[3] = 37;
		return true;
		case 2:
		strcpy(help, "Just make your way through...");
		for (int i = 40; i < 279; i += 40) {
			lava_list[i / 40] = new lava(i, 169, 0, -i * 0.03 - 1.2, false, 40, 40);
		}
		user->win_area[0] = 280;
		user->win_area[1] = 0;
		user->win_area[2] = 40;
		user->win_area[3] = 210;
		return true;
		case 3:
		strcpy(help, "I hated this one in the original.");
		user->win_area[0] = 140;
		user->win_area[1] = 90;
		user->win_area[2] = 40;
		user->win_area[3] = 30;
		accelerator_list[0] = new accelerator(50, 0, 0, 0, false, -12, 0, 245, 25);
		accelerator_list[1] = new accelerator(25, 185, 0, 0, false, 12, 0, 245, 25);
		accelerator_list[2] = new accelerator(25, 0, 0, 0, false, 0, 12, 25, 185);
		accelerator_list[3] = new accelerator(270, 25, 0, 0, false, 0, -12, 25, 185);

		accelerator_list[4] = new accelerator(100, 135, 0, 0, false, -8, 0, 145, 25);
		accelerator_list[5] = new accelerator(75, 50, 0, 0, false, 8, 0, 145, 25);
		accelerator_list[6] = new accelerator(220, 50, 0, 0, false, 0, 8, 25, 85);
		accelerator_list[7] = new accelerator(75, 75, 0, 0, false, 0, -8, 25, 85);

		lava_list[0] = new lava(87.5, 62.5, 0, 0, true, 25);
		lava_list[1] = new lava(87.5, 147.5, 0, 0, true, 25);
		lava_list[2] = new lava(232.5, 62.5, 0, 0, true, 25);
		lava_list[3] = new lava(232.5, 147.5, 0, 0, true, 25);
		return true;
		case 4:
		strcpy(help, "It can't be THAT easy, can it?");
		accelerator_list[4] = new accelerator(100, 135, 0, 0, false, -12, 0, 145, 25);
		accelerator_list[5] = new accelerator(75, 50, 0, 0, false, 12, 0, 145, 25);
		accelerator_list[6] = new accelerator(220, 50, 0, 0, false, 0, 12, 25, 85);
		accelerator_list[7] = new accelerator(75, 75, 0, 0, false, 0, -12, 25, 85);
		lava_list[0] = new lava(210, 75, 0, 0, false, 10, 60);
		lava_list[1] = new lava(100, 75, 0, 0, false, 110, 10);
		return true;
		case 5:
		strcpy(help, "This looks TOO easy. Hmm...");
		user->gravity_center[0] = 160;
		user->gravity_center[1] = 110;
		user->gravity_magnitude = -65;
		return true;
		case 6:
		strcpy(help, "Do you like playing Flappy Bird?");
		accelerator_list[0] = new accelerator(25, 0, 0, 0, false, 0, 1, 285, 210);
		user->win_area[0] = 310;
		user->win_area[1] = 0;
		user->win_area[2] = 10;
		user->win_area[3] = 210;
		srandom(42);
		for (int i = 0; i < 7; i++) {
			int top_height = randInt(45, 85);
			wall_list[2 * i] = new wall(25 + i * 45, 0, 0, 0/*, false*/, 15, top_height);
			wall_list[2 * i + 1] = new wall(25 + i * 45, top_height + 40, 0, 0/*, false*/, 15, 210 - top_height - 40);
			user->gravity_magnitude = 0;
		}
		return true;
		case 7:
		strcpy(help, "Why not try it with lava?");
		accelerator_list[0] = new accelerator(25, 0, 0, 0, false, 0, 1, 280, 210);
		user->win_area[0] = 305;
		user->win_area[1] = 0;
		user->win_area[2] = 15;
		user->win_area[3] = 210;
		for (int i = 0; i < 7; i++) {
			int top_height = randInt(45, 85);
			lava_list[2 * i] = new lava(25 + i * 45, 0, 0, 0, false, 10, top_height);
			lava_list[2 * i + 1] = new lava(25 + i * 45, top_height + 40, 0, 0, false, 10, 210 - top_height - 40);
		}
		return true;
		case 8:
		strcpy(help, "Isn't it more fun when things move?");
		accelerator_list[0] = new accelerator(25, 0, 0, 0, false, 0, 3.5, 280, 210);
		for (int i = 0; i < 7; i++) {
			int top_height = randInt(0, 50);
			lava_list[2 * i] = new lava(25 + i * 45, top_height + 150, 0, 0, false, 10, 210 - top_height - 80);
			lava_list[2 * i + 1] = new lava(25 + i * 45, top_height + 40, 0, randInt(5, 10), false, 10, 40);
			lava_list[2 * i + 1]->min_y = lava_list[2 * i + 1]->y;
			lava_list[2 * i + 1]->max_y = lava_list[2 * i + 1]->y + 110;
		}
		return true;
		case 9:
		strcpy(help, "Might take a BIT of trial and error.");
		accelerator_list[0] = new accelerator(25, 0, 0, 0, false, 0, 3.5, 280, 210);
		for (int i = 0; i < 7; i++) {
			lava_list[2 * i] = new lava(25 + i * 45, 0, 0, 0, false, 45, 80);
			lava_list[2 * i + 1] = new lava(25 + i * 45, 130, 0, 0, false, 45, 80);
			int vy = randInt(-1, 1);
			dbg_printf("%d\n", vy);
			accelerator_list[i] = new accelerator(25 + i * 45, 80, 0, 0, false, 0, vy / 1.2, 45, 50);
		}
		return true;
		case 10:
		strcpy(help, "That lava sure looks scary. Good luck!");
		user->win_area[0] = 280;
		user->win_area[1] = 180;
		user->win_area[2] = 40;
		user->win_area[3] = 30;
		wall_list[0] = new wall(20, 0, 0, 0, 58, 185);
		wall_list[1] = new wall(268, 25, 0, 0, 12, 185);
		lava_list[0] = new lava(238, 25, 0, 0, false, 30, 185);
		lava_list[1] = new lava(280, 168, -3, 0, false, 40, 12);
		lava_list[1]->min_x = 240;
		for (int i = 0; i < 8; i++) {
			lava_list[i + 2] = new lava(78 + 20 * i, 185, 0, -i * 0.2 - 0.2, false, 20, 15);
		}
		return true;
	}
	return false;
}
int main() {
	player* user = new player;
	user->x = 10;
	user->y = 10;
	user->radius = 8;
	user->display_radius = 9;
	user->deaths = 0;
	user->speed = 3;
	dbg_printf("Initializing graphx...\n");
	gfx_Begin();
	gfx_SetDrawBuffer();
	gfx_FillScreen(159);
	gfx_PrintStringXY("Extremely Hard ported from and inspired by:", 2, 2);
	gfx_PrintStringXY("github.com/idkwutocalmself/extremely-hard", 2, 17);
	gfx_PrintStringXY("Arrow keys to move, DEL to bail out", 2, 47);
	gfx_PrintStringXY("black sections are walls, red sections are lava", 2, 62);
	gfx_PrintStringXY("blue sections are accelerators", 2, 77);
	gfx_PrintStringXY("Press any key to start...", 2, 107);
	gfx_SwapDraw();
	while (!os_GetCSC());
	int fps = 24;

	lava* lava_list[LIST_SIZE];
	fill_array(lava_list, LIST_SIZE);
	accelerator* accelerator_list[LIST_SIZE];
	fill_array(accelerator_list, LIST_SIZE);
	wall* wall_list[LIST_SIZE];
	fill_array(wall_list, LIST_SIZE);

	int level = 1;
	char help[40];
	load_level(level, user, help, lava_list, accelerator_list, wall_list);
	dbg_printf("Starting gameloop...\n");
	bool win = false;
	bool quit = true;
	bool result = true;
	char status[18];
	do {
		clock_t begin = clock();
		kb_Scan();
		if (win) {
			win = false;
			level += 1;
			result = load_level(level, user, help, lava_list, accelerator_list, wall_list);
			if (!result) {
				dbg_printf("Result is false, no more levels!");
				quit = false;
				gfx_SetColor(0);
				gfx_FillScreen(159);
				gfx_PrintStringXY("You won! You went through all the levels", 2, 2);
				char line2[40];
				sprintf(line2, "with %d deaths. Press DEL to exit...", user->deaths);
				gfx_PrintStringXY(line2, 2, 17);
				gfx_PrintStringXY("github.com/weeklyd3/ti84", 2, 47);
				gfx_SwapDraw();
				while (os_GetCSC() != sk_Del) quit = true;
				break;
			}
		}

		user->previous_x = user->x;
		user->previous_y = user->y;
		if (kb_Data[7] & kb_Down) user->y += user->speed;
		if (kb_Data[7] & kb_Up) user->y -= user->speed;
		if (kb_Data[7] & kb_Left) user->x -= user->speed;
		if (kb_Data[7] & kb_Right) user->x += user->speed;
		if (kb_Data[6] & kb_Add) win = true;
		if (user->gravity_magnitude != 0) {
			float dx = user->x - user->gravity_center[0];
			float dy = user->y - user->gravity_center[1];
			real_t distance_squared = os_FloatToReal(dx * dx + dy * dy);
			distance_squared = os_RealSqrt(&distance_squared);
			float dist = os_RealToFloat(&distance_squared);
			user->x += user->gravity_magnitude / (dist) * (dx > 0 ? -1 : 1);
			user->y += user->gravity_magnitude / (dist) * (dy > 0 ? -1 : 1);
		}
		if ((user->x + user->display_radius) > 319) user->x = 319 - user->display_radius;
		if ((user->x - user->display_radius) < 0) user->x = user->display_radius;
		if ((user->y + user->display_radius) > 209) user->y = 209 - user->display_radius;
		if ((user->y - user->display_radius) < 0) user->y = user->display_radius;
		gfx_FillScreen(159);

		draw_list(accelerator_list, LIST_SIZE, user);
		draw_list(lava_list, LIST_SIZE, user);
		draw_list(wall_list, LIST_SIZE, user);

		gfx_SetColor(4);
		gfx_FillRectangle(user->win_area[0], user->win_area[1], user->win_area[2], user->win_area[3]);

		gfx_SetColor(39);
		gfx_FillCircle(user->x, user->y, user->display_radius);

		gfx_SetColor(255);
		gfx_FillRectangle(0, 209, 320, 30);
		sprintf(status, "L%d, %d deaths", level, user->deaths);
		gfx_PrintStringXY(status, 2, 211);
		gfx_PrintStringXY(help, 2, 221);

		if (rect_point_collision(user->win_area[0], user->win_area[1], user->win_area[2], user->win_area[3], user->x, user->y)) {
			win = true;
		}
		gfx_SwapDraw();
		double time = ((double) (clock() - begin)) / CLOCKS_PER_SEC;
		if (time > (1.0 / fps)) {
			dbg_printf("OVERRUN! %d ms > %d ms\n", (int) (time * 1000), (int) (1000 / fps));
		} else {
			usleep((1.0 / 24 - time) * 1000000);
		}
	} while (!(kb_Data[1] & kb_Del));
	gfx_End();
}