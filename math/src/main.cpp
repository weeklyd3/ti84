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
int compare(int a, int b) {
	return (a < b) ? -1 : (a > b);
}

int main(void) {
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Here, you will be shown a bunch of calculations. Please do them!");
	os_SetCursorPos(3, 0);
	os_PutStrFull("No goofing when you're not supposed to, ok? Press any key");
	while (!os_GetCSC());

	char rounds[5];
	char* end;
	int roundCount;
	os_SetCursorPos(7, 0);
	os_PutStrFull("Number of rounds (0 quits):");
	os_SetCursorPos(8, 0);
	os_GetStringInput("> ", rounds, 4);
	roundCount = strtol(rounds, &end, 10);
	if (roundCount == 0) {
		os_ClrHome();
		os_SetCursorPos(0, 0);
		os_PutStrFull("Zero or bad value entered. Quitting.");
		while (!os_GetCSC());
		return 1;
	}
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull("Enter RNG seed.");
	char seed[20];
	int seedNum;
	os_SetCursorPos(1, 0);
	os_GetStringInput("> ", seed, 20);
	seedNum = strtol(seed, &end, 10);
	srandom(seedNum);
	clock_t start = clock();
	int num1;
	int num2;
	int operation;
	char operatorChar[2];
	char titleBar[16];
	char question[9];
	char answer[4];
	char correctWrong[30];
	int answerNum;
	int actualAnswer;
	int correct = 0;
	int wrong = 0;
	for (int i = 0; i < roundCount; i++) {
		operation = randInt(1, 4);
		if (operation == 1 || operation == 3) {
			// do addition (1) or multiplication (3)
			num1 = randInt(1, 9);
			num2 = randInt(1, 9);
			if (operation == 1) {
				actualAnswer = num1 + num2;
				strcpy(operatorChar, "+");
			} else {
				actualAnswer = num1 * num2;
				strcpy(operatorChar, "*");
			}
		} else if (operation == 2) {
			// do subtraction
			strcpy(operatorChar, "-");
			num1 = randInt(1, 9);
			num2 = randInt(1, num1);
			actualAnswer = num1 - num2;
		} else {
			// do division
			strcpy(operatorChar, "/");
			num1 = randInt(2, 9);
			// YES, this code sucks. too lazy to think of a better way to do this.
			if (num1 == 2) num2 = randInt(1, 2);
			else if (num1 == 3) {
				if (randInt(1, 2) == 1) num2 = 1;
				else num2 = 3;
			} else if (num1 == 4) {
				num2 = randInt(1, 3); // temporary value, the if statements below assign another value that divides easily.
				if (num2 == 1) num2 = 1;
				else if (num2 == 2) num2 = 2;
				else num2 = 4;
			} else if (num1 == 5) {
				if (randInt(1, 2) == 1) num2 = 1;
				else num2 = 5;
			} else if (num1 == 6) {
				num2 = randInt(1, 4); // temporary value, the if statements below assign another value that divides easily.
				if (num2 == 1) num2 = 1;
				else if (num2 == 2) num2 = 2;
				else if (num2 == 3) num2 = 3;
				else num2 = 6;
			} else if (num1 == 7) {
				if (randInt(1, 2) == 1) num2 = 1;
				else num2 = 7;
			} else if (num1 == 8) {
				num2 = randInt(1, 4); // temporary value, the if statements below assign another value that divides easily.
				if (num2 == 1) num2 = 1;
				else if (num2 == 2) num2 = 2;
				else if (num2 == 3) num2 = 4;
				else num2 = 8;
			} else if (num1 == 9) {
				num2 = randInt(1, 3); // temporary value, the if statements below assign another value that divides easily.
				if (num2 == 1) num2 = 1;
				else if (num2 == 2) num2 = 3;
				else num2 = 9;
			}
			actualAnswer = num1 / num2;
		}

		os_ClrHome();
		os_SetCursorPos(0, 0);
		sprintf(titleBar, "Round %d of %d", i + 1, roundCount);
		os_PutStrFull(titleBar);
		os_SetCursorPos(1, 0);
		os_PutStrFull("Enter Q to quit.");
		os_SetCursorPos(2, 0);
		sprintf(correctWrong, "Correct/Wrong: %d/%d", correct, wrong);
		os_PutStrFull(correctWrong);
		os_SetCursorPos(3, 0);
		sprintf(question, "%d %s %d = ?", num1, operatorChar, num2);
		os_PutStrFull(question);
		os_SetCursorPos(4, 0);
		os_GetStringInput("> ", answer, 4);
		if (answer[0] == 'Q' || answer[0] == 'q') {
			// BAIL OUT!!! BAIL OUT!!!
			return 0;
		}
		answerNum = strtol(answer, &end, 10);
		if (answerNum == actualAnswer) correct += 1;
		else wrong += 1;
	}
	clock_t endTime = clock();
	float seconds = (float) (endTime - start) / CLOCKS_PER_SEC;
	dbg_printf("TIME: %.6f\n", seconds);
	char secondsToString[9];
	float2str(seconds, secondsToString);

	float score = 10 * seconds / (correct + wrong) + wrong;
	char scoreToString[5];
	float2str(score, scoreToString);
	char status[150];
	//sprintf(status, "Correct/Wrong: %d/%d. You took %s seconds and scored %s points, using seed %d. Press any key to end...", correct, wrong, secondsToString, scoreToString, seedNum);
	sprintf(status, "Correct/Wrong: %d/%d. You scored %s points using seed %d. Press any key to end...", correct, wrong, scoreToString, seedNum);
	os_ClrHome();
	os_SetCursorPos(0, 0);
	os_PutStrFull(status);
	os_SetCursorPos(6, 0);
	os_PutStrFull("Lower is better!");
	os_SetCursorPos(7, 0);
	os_PutStrFull("github.com/weeklyd3/ti84");
	while (!os_GetCSC());
	return 0;
}