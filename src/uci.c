

#include <stdio.h>
#include "defs.h"
#include <string.h>
#include <math.h>

#define INPUTBUFFER 400 * 6


void ParseGo(char* line, S_SEARCHINFO *info, S_BOARD *pos, S_HASHTABLE *table) {

	int depth = -1, movestogo = 30, movetime = -1;
	int time = -1, inc = 0; 
    char *ptr = NULL;
	info->timeset = FALSE;

	if ((ptr = strstr(line, "infinite"))) {
		;
	}

	if ((ptr = strstr(line, "binc")) && pos->side == BLACK) {
		inc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line, "winc")) && pos->side == WHITE) {
		inc = atoi(ptr + 5);
	}

	if ((ptr = strstr(line, "wtime")) && pos->side == WHITE) {
		time = atoi(ptr + 6);
	}

	if ((ptr = strstr(line, "btime")) && pos->side == BLACK) {
		time = atoi(ptr + 6);
	}

	if ((ptr = strstr(line, "movestogo"))) {
		movestogo = atoi(ptr + 10);
	}

	if ((ptr = strstr(line, "movetime"))) {
		movetime = atoi(ptr + 9);
	}

	if ((ptr = strstr(line, "depth"))) {
		depth = atoi(ptr + 6);
	}

	if (movetime != -1) {
		time = movetime;
		movestogo = 1;
	}

	info->starttime = GetTimeMs();
	info->depth = depth;
	
	/**********************
	*** Time Management ***
	**********************/
	
	if (time != -1) {

		info->timeset = TRUE;
		double time_allocated = time;
		uint8_t phase_moves = 0;

		
		

		
		if (time < 30000 /* 30s */) {
			time_allocated /= 80;
		} else {
			
			if (pos->hisPly <= 30) {
				
				time_allocated *= 0.1;
				phase_moves = round((30 - pos->hisPly + (pos->side ? 0 : 1)) / 2.0); 
				time_allocated /= phase_moves;
			} else {
				
				if (pos->hisPly <= 70) {
					
					time_allocated *= 0.45;
					phase_moves = round( (70 - pos->hisPly + (pos->side ? 0 : 1)) / 2.0 );
					time_allocated /= phase_moves;
				} else {
					
					
					time_allocated /= 35;
				}
			}
		}
		

		
		time_allocated -= 50; 
		info->stoptime = (int)(info->starttime + time_allocated + inc/2);
	}

	if (depth == -1) {
		info->depth = MAX_DEPTH;
	}

	
	SearchPosition(pos, table, info);
}




void ParsePosition(char* lineIn, S_BOARD *pos) {

	lineIn += 9;
    char *ptrChar = lineIn;

    if(strncmp(lineIn, "startpos", 8) == 0){
        ParseFen(START_FEN, pos);
    } else {
        ptrChar = strstr(lineIn, "fen");
        if(ptrChar == NULL) {
            ParseFen(START_FEN, pos);
        } else {
            ptrChar += 4;
            ParseFen(ptrChar, pos);
        }
    }

	ptrChar = strstr(lineIn, "moves");
	int move;

	if(ptrChar != NULL) {
        ptrChar += 6;
        while(*ptrChar) {
              move = ParseMove(ptrChar,pos);
			  if(move == NOMOVE) break;
			  MakeMove(pos, move);
              pos->ply=0;
              while(*ptrChar && *ptrChar!= ' ') ptrChar++;
              ptrChar++;
        }
    }

	
}

void Uci_Loop(S_BOARD *pos, S_SEARCHINFO *info) {

	setbuf(stdin, NULL);
    setbuf(stdout, NULL);

	char line[INPUTBUFFER];
    printf("id name %s\n", NAME);
    printf("id author Tamplite Siphron Kents\n");
	printf("option name Hash type spin default 64 min 4 max %d\n", MAX_HASH);
	int MB = 64;
	if (OPENBENCH_MODE) {
		printf("option name Threads type spin default 1 min 1 max 1\n"); 
		printf("option name Book type check default false\n"); 
		EngineOptions->UseBook = FALSE;
	} else {
		printf("option name Book type check default true\n");
		EngineOptions->UseBook = TRUE;
	}
	ParseFen(START_FEN, pos); 
    printf("uciok\n");

	while (TRUE) {
		memset(&line[0], 0, sizeof(line));
        fflush(stdout);
        if (!fgets(line, INPUTBUFFER, stdin))
        continue;

        if (line[0] == '\n')
        continue;

        if (!strncmp(line, "isready", 7)) {
            printf("readyok\n");
            continue;
        } else if (!strncmp(line, "position", 8)) {
            ParsePosition(line, pos);
        } else if (!strncmp(line, "ucinewgame", 10)) {
			ClearHashTable(HashTable);
            ParsePosition("position startpos\n", pos);
        } else if (!strncmp(line, "go", 2)) {
            ParseGo(line, info, pos, HashTable);
		} else if (!strncmp(line, "run", 3)) {
            ParseGo("go infinite", info, pos, HashTable);
        } else if (!strncmp(line, "quit", 4)) {
            info->quit = TRUE;
            break;
        } else if (!strncmp(line, "uci", 3)) {
            printf("id name %s\n", NAME);
            printf("id author Tamplite Siphron Kents\n");
            printf("uciok\n");
		} else if (!strncmp(line, "debug", 4)) {
            DebugAnalysisTest(pos, HashTable, info);
            break;
        } else if (!strncmp(line, "setoption name Hash value ", 26)) {			
			sscanf(line,"%*s %*s %*s %*s %d", &MB);
			if (MB < 4) MB = 4;
			if (MB > MAX_HASH) MB = MAX_HASH;
			printf("Set Hash to %d MB\n", MB);
			InitHashTable(HashTable, MB);
		} else if (!strncmp(line, "setoption name Book value ", 26)) {			
			char *ptrTrue = NULL;
			ptrTrue = strstr(line, "true");
			if(ptrTrue != NULL) {
				printf("Set Book to true\n");
				EngineOptions->UseBook = TRUE;
			} else {
				printf("Set Book to false\n");
				EngineOptions->UseBook = FALSE;
			}
		}
		if (info->quit) break;
    }
}













