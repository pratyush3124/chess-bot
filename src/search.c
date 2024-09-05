

#include "stdio.h"
#include <stdlib.h>
#include <math.h>
#include "string.h"
#include "defs.h"

static void CheckUp(S_SEARCHINFO *info) {
	
	if( (info->timeset == TRUE) && (GetTimeMs() > info->stoptime) ) {
		info->stopped = TRUE;
	}
}


static void sort_move_list(S_MOVELIST *list, int move_count) {

	
	if (list == NULL || move_count <= 0) {
        return;
    }
	
	S_MOVE temp;

	
	for (int i = 1; i < move_count; ++i) {
        temp = list->moves[i];
        
        
        
		int j = i - 1;
        while ( (j >= 0) && (list->moves[j].score < temp.score) ) {
			list->moves[j + 1] = list->moves[j];
            j = j - 1;
        }
        list->moves[j + 1] = temp;
    }

}

/*

static void PickNextMove(int moveNum, S_MOVELIST *list) {

	S_MOVE temp;
	int index = 0;
	int bestScore = -INF_BOUND;
	int bestNum = moveNum;

	for (index = moveNum; index < list->count; ++index) {
		if (list->moves[index].score > bestScore) {
			bestScore = list->moves[index].score;
			bestNum = index;
		}
	}

	ASSERT(moveNum >= 0 && moveNum < list->count);
	ASSERT(bestNum >= 0 && bestNum < list->count);
	ASSERT(bestNum >= moveNum);

	temp = list->moves[moveNum];
	list->moves[moveNum] = list->moves[bestNum];
	list->moves[bestNum] = temp;
}
*/

static int IsRepetition(const S_BOARD *pos) {

	int index = 0;

	for(index = pos->hisPly - pos->fiftyMove; index < pos->hisPly-1; ++index) {
		ASSERT(index >= 0 && index < MAXGAMEMOVES);
		if(pos->posKey == pos->history[index].posKey) {
			return TRUE;
		}
	}
	return FALSE;
}

static void ClearForSearch(S_BOARD *pos, S_HASHTABLE *table, S_SEARCHINFO *info) {

	for(int index = 0; index < 13; ++index) {
		for(int index2 = 0; index2 < BRD_SQ_NUM; ++index2) {
			pos->searchHistory[index][index2] = 40000;
		}
	}

	for(int index = 0; index < 2; ++index) {
		for(int index2 = 0; index2 < MAX_DEPTH; ++index2) {
			pos->searchKillers[index][index2] = 40000;
		}
	}

	table->overWrite = 0;
	table->hit = 0;
	table->cut = 0;
	table->currentAge++;
	pos->ply = 0;

	info->stopped = FALSE;
	info->nodes = 0;
	info->fh = 0;
	info->fhf = 0;
}

static inline int Quiescence(int alpha, int beta, S_BOARD *pos, S_SEARCHINFO *info) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);
	if(( info->nodes & 2047 ) == 0) {
		CheckUp(info);
	}

	if(IsRepetition(pos) || pos->fiftyMove >= 100) {
		return 0;
	}

	if(pos->ply >= MAX_DEPTH) {
		return EvalPosition(pos);
	}

	int32_t Score = EvalPosition(pos); 

	ASSERT(Score > -INF_BOUND && Score < INF_BOUND);

	
	if(Score >= beta) {
		return beta;
	}
	
	
	uint16_t big_delta = 936; 
	
	if (Score < alpha - big_delta) {
		return alpha;
	}

	if(Score > alpha) {
		alpha = Score;
	}

	
	S_MOVELIST list[1];
    GenerateAllCaps(pos, list);

    int MoveNum = 0;
	int Legal = 0;
	
	
	#define DELTA_BUFFER 180

	sort_move_list(list, list->count);

	for(MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		

		
		
		
		int capturedPiece = CAPTURED(list->moves[MoveNum].move);
		int delta = PieceValMg[capturedPiece]; 
		
		
		if (delta + DELTA_BUFFER <= alpha) {
			return alpha;
		}
		 
		
        if ( !MakeMove(pos, list->moves[MoveNum].move) )  {
            continue;
        }
		info->nodes++;
		Legal++;

		Score = -Quiescence(-beta, -alpha, pos, info);
        TakeMove(pos);

		if(info->stopped == TRUE) {
			return 0;
		}

		if(Score > alpha) {
			if(Score >= beta) {
				if(Legal==1) {
					info->fhf++;
				}
				info->fh++;
				return beta;
			}
			alpha = Score;
		}
    }

	return alpha;
}

static inline int AlphaBeta(int alpha, int beta, int depth, S_BOARD *pos, S_HASHTABLE *table, S_SEARCHINFO *info, int DoNull) {

	ASSERT(CheckBoard(pos));
	ASSERT(beta>alpha);
	ASSERT(depth>=0);

	if(depth <= 0) {
		return Quiescence(alpha, beta, pos, info);
		
	}

	if(( info->nodes & 2047 ) == 0) {
		CheckUp(info);
	}

	if((IsRepetition(pos) || pos->fiftyMove >= 100) && pos->ply) {
		return 0;
	}

	
	if(pos->ply >= MAX_DEPTH) {
		return EvalPosition(pos);
	}

	
	uint8_t InCheck = SqAttacked(pos->KingSq[pos->side],!pos->side,pos);

	
	if(InCheck == TRUE) {
		depth++;
	}

	int Score = -INF_BOUND;
	int PvMove = NOMOVE;

	if( ProbeHashEntry(pos, table, &PvMove, &Score, alpha, beta, depth) == TRUE ) {
		table->cut++;
		return Score;
	}

	/*
		Null-move Pruning
	*/
	
	if( DoNull && !InCheck && pos->ply && (pos->bigPce[pos->side] > 1) && depth >= 4) {
		MakeNullMove(pos);
		Score = -AlphaBeta( -beta, -beta + 1, depth-4, pos, table, info, FALSE);
		TakeNullMove(pos);
		if(info->stopped == TRUE) {
			return 0;
		}

		if (Score >= beta && abs(Score) < ISMATE) {
			info->nullCut++;
			return beta;
		}
	}

	S_MOVELIST list[1];
    GenerateAllMoves(pos, list); 

	int Legal = 0;
	int OldAlpha = alpha;
	int BestMove = NOMOVE;

	int BestScore = -INF_BOUND;
	Score = -INF_BOUND;

	
	if (PvMove != NOMOVE) {
		for(int MoveNum = 0; MoveNum < list->count; ++MoveNum) {
			if (list->moves[MoveNum].move == PvMove) {
				list->moves[MoveNum].score = 2000000;
				break;
			}
		}
	}

	sort_move_list(list, list->count);

	for(int MoveNum = 0; MoveNum < list->count; ++MoveNum) {

		

		int curr_move = list->moves[MoveNum].move;
		
		/*
			(Extended) Futility Pruning
		*/
		

		
		#define FUTILITY_MARGIN 325
		
		#define EXTENDED_FUTILITY_MARGIN 475

		
		int IsCapture = CAPTURED(curr_move);

		
		
		if ( ( (depth == 1) || (depth == 2) ) && (abs(Score) < ISMATE) ) {
			int static_score = EvalPosition(pos);

			
			if (!InCheck && !IsCapture) {
				if ( (depth == 2) && (static_score + EXTENDED_FUTILITY_MARGIN <= alpha) ) {
					continue; 
				} else if ( (depth == 1) && (static_score + FUTILITY_MARGIN <= alpha) ) {
					continue;
				}
			}
		} 
		
		
		
        if ( !MakeMove(pos, curr_move) )  {
            continue;
        }
		Legal++;
		info->nodes++;

		/*
			Late Move Reductions
		*/ 
        

        int reduced_depth = depth - 1; 
        
        if (abs(Score) < ISMATE) {

            
            if (MoveNum > 3 && depth > 4) {

                uint8_t self_king_sq = pos->KingSq[pos->side];
                uint8_t moving_pce = pos->pieces[FROMSQ(curr_move)];
                uint8_t target_sq = TOSQ(curr_move);

                int IsPromotion = PROMOTED(curr_move);
                
                
                uint8_t target_sq_within_king_zone = dist_between_squares(self_king_sq, target_sq) <= 3; 

                if (!IsCapture && !IsPromotion && !InCheck && !IsPawn(moving_pce) && !target_sq_within_king_zone) {
                    reduced_depth = (int)( log(depth) * log(MoveNum) / 2.25 );
					
					reduced_depth = max(reduced_depth, max(4, depth - 4));
                }
            }
        }

		Score = -AlphaBeta(-beta, -alpha, reduced_depth, pos, table, info, TRUE);
		TakeMove(pos);

		if(info->stopped == TRUE) {
			return 0;
		}

		
		if(Score > BestScore) {
			BestScore = Score;
			BestMove = curr_move;
			if(Score > alpha) {
				if(Score >= beta) {
					if(Legal == 1) {
						info->fhf++;
					}
					info->fh++;

					if(!(curr_move & MFLAGCAP)) {
						pos->searchKillers[1][pos->ply] = pos->searchKillers[0][pos->ply];
						pos->searchKillers[0][pos->ply] = curr_move;
					}

					StoreHashEntry(pos, table, BestMove, beta, HFBETA, depth);

					return beta;
				}
				alpha = Score;

				if(!(curr_move & MFLAGCAP)) {
					pos->searchHistory[pos->pieces[FROMSQ(BestMove)]][TOSQ(BestMove)] += depth;
				}
			}
		}
    }

	
	if (Legal == 0) {
		if(InCheck) {
			return -INF_BOUND + pos->ply;
		} else {
			return 0;
		}
	}

	ASSERT(alpha >= OldAlpha);

	if(alpha != OldAlpha) {
		StoreHashEntry(pos, table, BestMove, BestScore, HFEXACT, depth);
	} else {
		StoreHashEntry(pos, table, BestMove, alpha, HFALPHA, depth);
	}

	return alpha;
}

/*******************************
*** Iterative Deepening Loop ***
*******************************/

void SearchPosition(S_BOARD *pos, S_HASHTABLE *table, S_SEARCHINFO *info) {

	int bestMove = NOMOVE;
	int bestScore = -INF_BOUND;
	int pvMoves = 0;
	int pvNum = 0;

	
	uint8_t window_size = 50; 
	int guess = -INF_BOUND;
	int alpha = -INF_BOUND;
	int beta = INF_BOUND;

	ClearForSearch(pos, table, info); 
	
	
	if (EngineOptions->UseBook == TRUE) {
		bestMove = GetBookMove(pos);
	}

	if (bestMove == NOMOVE) {

		for (int currentDepth = 1; currentDepth <= info->depth; ++currentDepth) {

			
			if (currentDepth == 1) {
				bestScore = AlphaBeta(-INF_BOUND, INF_BOUND, currentDepth, pos, table, info, TRUE);
			} 
			else {
				
				
				if (currentDepth > 6) {
					
					window_size = max(-2.5 * currentDepth + 65, 25);
				}
				alpha = guess - window_size;
				beta = guess + window_size;

				uint8_t reSearch = TRUE;
				while (reSearch) {
					bestScore = AlphaBeta(alpha, beta, currentDepth, pos, table, info, TRUE);

					
					if (bestScore <= alpha || bestScore >= beta) {
						alpha = -INF_BOUND;
						beta = INF_BOUND;
					} else {
						
						reSearch = FALSE;
					}
            	}
			}
            
			guess = bestScore;

			if(info->stopped == TRUE) {
				break;
			}

			pvMoves = GetPvLine(currentDepth, pos, table);
			bestMove = pos->PvArray[0];

			
			unsigned long long time = GetTimeMs() - info->starttime;
			uint8_t mate_found = FALSE; 
			int8_t mate_moves = 0;
			if (abs(bestScore) >= ISMATE) {
				mate_found = TRUE;
				
				
				mate_moves = round( (INF_BOUND - abs(bestScore) - 1) / 2 + 1) * copysign(1.0, bestScore);
				printf("info score mate %d depth %d nodes %ld hashfull %d time %llu pv",
					mate_moves, currentDepth, info->nodes, (int)(table->numEntries / (double)table->maxEntries * 1000), time);
			} else {
				printf("info score cp %d depth %d nodes %ld hashfull %d time %llu pv",
					bestScore, currentDepth, info->nodes, (int)(table->numEntries / (double)table->maxEntries * 1000), time);
			}
			

			
			for(pvNum = 0; pvNum < pvMoves; ++pvNum) {
				printf(" %s",PrMove(pos->PvArray[pvNum]));
			}
			printf("\n");

			
			if ( mate_found && ( currentDepth >= (abs(mate_moves) + 1) ) ) {
				break;
				
			}
		}
	}

	printf("bestmove %s\n", PrMove(bestMove));

}