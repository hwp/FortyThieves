/*
 * Author : hwp
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

#define CARDS_PER_SUIT 13
#define NUM_PER_DECK (CARDS_PER_SUIT * 4)
#define NUM_DECKS 2
#define NUM_CARDS (NUM_PER_DECK * NUM_DECKS)
#define NUM_TABLEAU 10
#define NUM_FOUNDATION 8
#define NUM_PER_TABLEAU 4

#define STOCK 0xFFFF0000
#define FOUNDATION 0x80
#define SHIFT (sizeof(char) * 8)

char face[] = "🂱🂲🂳🂴🂵🂶🂷🂸🂹🂺🂻🂽🂾🃑🃒🃓🃔🃕🃖🃗🃘🃙🃚🃛🃝🃞🃁🃂🃃🃄🃅🃆🃇🃈🃉🃊🃋🃍🃎🂡🂢🂣🂤🂥🂦🂧🂨🂩🂪🂫🂭🂮";
char suitf[] = "♥♣♦♠";
char codef[] = "A234567890JQK";

typedef struct stack{
	int value;
	struct stack* prev;
} stack;

void push(stack** s, int v) {
	stack* p = *s;
	*s = malloc(sizeof(stack));
	(*s)->value = v;
	(*s)->prev = p;
}

int pop(stack** s) {
	int v = (*s)->value;
	stack* h = *s;
	*s = (*s)->prev;
	free(h);
	return v;
}

int size(stack* s) {
	int size = 0;
	while (s != NULL) {
		size++;
		s = s->prev;
	}
	return size;
}

stack* stock;
stack* waste;
stack* tableau[NUM_TABLEAU];
int foundation[NUM_FOUNDATION];
char pick;

stack* history;

void shuffle(int* seq, int n) {
	int r, c;
	for (int i = 0; i < n; i++) {
		r = rand() % (n - i);
		c = seq[i];
		seq[i] = seq[r + i];
		seq[r + i] = c;
	}
}

void print_face (int id) {
	if (id / CARDS_PER_SUIT % 2 == 0) {
		printf("\033[31m");
	}
//	printf("%.4s ", face + id * 4);
	printf("%c%.3s  ",
			codef[id % CARDS_PER_SUIT],
			suitf + id / CARDS_PER_SUIT * 3);
	if (id / CARDS_PER_SUIT % 2 == 0) {
		printf("\033[0m");
	}
}

void print_stack(stack* s) {
	if (s != NULL) {
		print_stack(s->prev);
		print_face(s->value);
	}
}

int move(char from, char to) {
	stack** fs;
	if (from == 'w') {
		fs = &waste;
	}
	else {
		fs = &tableau[from - '0'];
	}
	int id = (*fs)->value;
	int suit = id / CARDS_PER_SUIT;
	int code = id % CARDS_PER_SUIT;

	if (to == 'f') {
		for (int i = suit * NUM_DECKS; i < suit * NUM_DECKS + 2; i++) {
			if (foundation[i] == code) {
				foundation[i]++;
				pop(fs);
				push(&history, ((int)from << SHIFT) + (FOUNDATION + i));
				return 0;
			}
		}
	}
	else if (to == 'w') {
		//do nothing	
	}
	else {
		stack** ts = &tableau[to - '0'];
		if (*ts == NULL) {
			push(ts, pop(fs));
			push(&history, ((int)from << SHIFT) + to);
			return 0;
		}
		else {
			int tid = (*ts)->value;
			int tsuit = tid / CARDS_PER_SUIT;
			int tcode = tid % CARDS_PER_SUIT;
			printf("%d->%d", id, tid);
			if ((suit == tsuit) && (code == tcode - 1)) {
				push(ts, pop(fs));
				push(&history, ((int)from << SHIFT) + to);
				return 0;
			}
		}
	}

	return 1;
}

void undo() {
	if (history != NULL) {
		int c = pop(&history);
		if (c == STOCK) {
			push(&stock, pop(&waste));
		}
		else {
			int from = c >> SHIFT;
			int to = c & ((1 << SHIFT) - 1);

			stack** fs;
			if (from == 'w') {
				fs = &waste;
			}
			else {
				fs = &tableau[from - '0'];
			}

			if (to >= FOUNDATION && to < FOUNDATION + NUM_FOUNDATION) {
				int i = to - FOUNDATION;
				foundation[i]--;
				int d = (i / 2) * CARDS_PER_SUIT + foundation[i];
				push(fs, d);
			}
			else {
				stack** ts = &tableau[to - '0'];
				push(fs, pop(ts));
			}
		}
	}
}

void print() {
	printf("\033[2J\033[1;1H");

	printf("[f] : ");
	for(int i = 0; i < NUM_FOUNDATION; i++) {
		if (foundation[i] > 0) {
			int v = (i / 2) * CARDS_PER_SUIT + foundation[i] - 1;
			print_face(v);
		}
	}
	printf("\n\n");
	printf("[s] : %u\n", size(stock));
	printf("[w] : ");
	print_stack(waste);
	if (pick == 'w') {
		printf("<-");
	}
	printf("\n\n");
	for (int i = 0; i < NUM_TABLEAU; i++) {
		printf("[%u] : ", i);
		print_stack(tableau[i]);
		if ((pick - '0') == i) {
			printf("<-");
		}
		printf("\n");
	}
	printf("\n");
}

struct termios pterm;

void tcexit() {
	tcsetattr(STDIN_FILENO, TCSANOW, &pterm);
}

void tcinit() {
	struct termios term;
	tcgetattr(STDIN_FILENO, &term);
	pterm = term;
	term.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	atexit(tcexit);
}

int main() {
	srand(time(NULL));

	stock = NULL;
	waste = NULL;
	for (int i = 0; i < NUM_TABLEAU; i++) {
		tableau[i] = NULL;
	}
	for (int i = 0; i < NUM_FOUNDATION; i++) {
		foundation[i] = 0;
	}
	pick = '\0';

	history = NULL;

	int seq[NUM_CARDS];
	for (int i = 0; i < NUM_CARDS; i++) {
		seq[i] = i % NUM_PER_DECK;
		printf("%d\n", seq[i]);
	}
	shuffle(seq, NUM_CARDS);

	for (int i = 0; i < NUM_CARDS; i++) {
		printf("%d\n", seq[i]);
	}

	int counter = 0;
	for (int i = 0; i < NUM_TABLEAU; i++) {
		for (int j = 0; j < NUM_PER_TABLEAU; j++) {
			push(&(tableau[i]), seq[counter]);
			counter++;
		}
	}

	for ( ; counter < NUM_CARDS; counter++) {
		push(&stock, seq[counter]);
	}
	
	print();

	tcinit();
	char c;
	while ((c = getchar()) != 'q') {
		if (c == 's') {
			if (stock != NULL) {
				push(&waste, pop(&stock));
				push(&history, STOCK);
			}
			pick = '\0';
		}
		else if ((c >= '0' && c <= '9') || c == 'w' || c == 'f') {
			if (pick == '\0') {
				if (c != 'f') {
					stack* fs;
					if (c == 'w') {
						fs = waste;
					}
					else {
						fs = tableau[c - '0'];
					}
					
					if (fs != NULL) {
						pick = c;
					}
				}
			}
			else {
				move(pick, c);
				pick = '\0';
			}
		}
		else if (c == 'u') {
			undo();
			pick = '\0';
		}
		else {
			continue;
		}

		print();
	}
}

