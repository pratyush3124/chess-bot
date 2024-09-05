

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "defs.h"
#include "evaluate.h"















#define openingPhase 64

int16_t evaluate_pawn_structure(const S_BOARD *pos, uint8_t pawn_sq, uint8_t col) {

	int16_t pawn_score = 0;

	
	if( (IsolatedMask[SQ64(pawn_sq)] & pos->pawns[col]) == 0) {
		
		pawn_score += PawnIsolated;
	}

	
	U64 mask = FileBBMask[FilesBrd[pawn_sq]] & pos->pawns[col];
	uint8_t stacked_count = CountBits(mask);
	if(stacked_count > 1) {
		
		pawn_score += PawnDoubled * (stacked_count - 1);
	}

	
	if (col == WHITE) {
		if( (WhitePassedMask[SQ64(pawn_sq)] & pos->pawns[BLACK]) == 0) {
			
			pawn_score += PawnPassed[RanksBrd[pawn_sq]];
		}
	} else {
		if( (BlackPassedMask[SQ64(pawn_sq)] & pos->pawns[WHITE]) == 0) {
			
			pawn_score += PawnPassed[7 - RanksBrd[pawn_sq]];
		}
	}
	
	return pawn_score;
}


inline double evalWeight(const S_BOARD *pos) {
	
	

	ASSERT(CheckBoard(pos));

	
	uint8_t gamePhase = 3 * ( pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB] );
	gamePhase += 5 * ( pos->pceNum[wR] + pos->pceNum[bR] );
	gamePhase += 10 * ( pos->pceNum[wQ] + pos->pceNum[bQ] );
	gamePhase = fmin(gamePhase, openingPhase); 

	
	
	return gamePhase / (double)openingPhase;

}


static inline double king_tropism_for_knight(const S_BOARD *pos, int opp_king_sq, uint8_t pce, uint8_t factor) {

	double tropism = 0;

	for (int i = 0; i < pos->pceNum[pce]; ++i) {
		uint8_t sq = pos->pList[pce][i];
		tropism += factor * ( 15 - (dist_between_squares(opp_king_sq, sq) - 3)); 
	}

	return tropism;
}

static inline double king_tropism_for_bishop(const S_BOARD *pos, int opp_king_sq, uint8_t pce) {

	const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 }; 
	const uint8_t bishop_king_sq_bonus = 4;
	double tropism = 0;

	for (int i = 0; i < pos->pceNum[pce]; ++i) {
		uint8_t sq = pos->pList[pce][i];
		for (int j = 0; j < 8; ++j) {
			uint8_t sq_near_king = sq + KiDir[j];
			if ( (SQ64(sq_near_king) != 65) && on_same_diagonal(opp_king_sq, sq) ) {
				tropism += bishop_king_sq_bonus;
			}
		}
	}

	return tropism;
}

static inline int16_t king_tropism_for_RQ(const S_BOARD *pos, uint8_t opp_king_sq, uint8_t col) {
	
	
	const uint8_t rook_KingSemiopenFile = 10;
	const uint8_t rook_KingOpenFile[3] = { 25, 55, 25 };
	const uint8_t queen_KingSemiopenFile = 10;
	const uint8_t queen_KingOpenFile[3] = { 40, 70, 40 };

	uint8_t king_file = FilesBrd[opp_king_sq];
	uint8_t col_offset = (col == WHITE) ? 0 : 6;
	uint16_t tropism = 0;

	uint8_t pce = wR + col_offset;
	for (int i = 0; i < pos->pceNum[pce]; ++i) {
		uint8_t sq = pos->pList[pce][i];
		int8_t relative_file = FilesBrd[sq] - king_file;

		if (abs(relative_file) < 1) {
			if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
				tropism -= rook_KingSemiopenFile;
			} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
				uint8_t relative_file = FilesBrd[sq] - king_file;
				tropism += rook_KingOpenFile[relative_file];
			}
		}
	}

	pce = wQ + col_offset;
	for (int i = 0; i < pos->pceNum[pce]; ++i) {
		uint8_t sq = pos->pList[pce][i];
		int8_t relative_file = FilesBrd[sq] - king_file;

		if (abs(relative_file) < 1) {
			if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
				tropism += queen_KingSemiopenFile;
			} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
				uint8_t relative_file = FilesBrd[sq] - king_file;
				tropism += queen_KingOpenFile[relative_file];
			}
		}
	}


	return tropism;
}

static inline double king_tropism(const S_BOARD *pos, uint8_t col) {
	
	
	

	double tropism = 0;
	int opp_king_sq = 0;
	uint8_t colour_offset = (col == BLACK) ? 6 : 0;

	
	opp_king_sq = pos->KingSq[!col];
	
	
	uint8_t pce = wN + colour_offset;
	tropism += king_tropism_for_knight(pos, opp_king_sq, pce, 1); 

	pce = wB + colour_offset;
	tropism += king_tropism_for_bishop(pos, opp_king_sq, pce); 

	
	tropism += king_tropism_for_RQ(pos, opp_king_sq, col) * 0.5; 

	return tropism;
}

/*
	Attack Units (modified)
*/

U64 generate_king_zone(uint8_t kingSq) {

	/*
	--------
	--------
	--------
	--------
	---xxxxx
	---xxxxx
	---xxxxx
	---xxxKx
	*/

	U64 king_zone = 0ULL;
	uint8_t king_file = FilesBrd[kingSq];
	uint8_t king_rank = RanksBrd[kingSq];

	for (int rank = king_rank - 3; rank <= king_rank + 3; ++rank) {
		for (int file = king_file - 3; file <= king_file + 3; ++file) {
			if (rank >= RANK_1 && rank <= RANK_8 && file >= FILE_A && file <= FILE_H) {
				uint8_t sq = SQ64(FR2SQ(file, rank));
				king_zone |= 1ULL << sq;
			}
		}
	}

	return king_zone;
}


int16_t attack_units(const S_BOARD *pos, uint8_t col) {
	
	uint8_t opp_king_sq = pos->KingSq[!col];
	U64 king_zone = generate_king_zone(opp_king_sq);
	uint8_t attack_units = 0, defense_units = 0;
	
	while (king_zone) {
		uint8_t sq = SQ120(PopBit(&king_zone));
		if (SqAttackedS(sq, col, pos)) {
			attack_units++;
		}
		if (SqAttackedS(sq, !col, pos)) {
			defense_units++;
		}
	}

	int16_t attack_potency = (attack_units - defense_units) * 3;
	return SafetyTable[clamp(0, attack_potency, 99)];
}


/*
	Pawn Shield
*/

static inline U64 generate_shield_zone(uint8_t kingSq, uint8_t col) {

	/*
	--------
	--------
	--------
	--------
	-----xxx
	-----xxx
	-----xxx
	------K-
	*/

	U64 shield_zone = 0ULL;
	uint8_t king_file = FilesBrd[kingSq];
	uint8_t king_rank = RanksBrd[kingSq];

	int8_t delta = 0;
	uint8_t start_rank = 0, end_rank = 0;
	if (col == WHITE) {
		delta = 1;
		start_rank = king_rank + 1;
		end_rank = king_rank + 3;
	} else {
		delta = -1;
		start_rank = king_rank - 1;
		end_rank = king_rank - 3;
	}

	for (int rank = start_rank; (col == WHITE) ? rank <= end_rank : rank >= end_rank; rank += delta) {
		for (int file = king_file - 1; file <= king_file + 1; ++file) {
			uint8_t sq = SQ64(FR2SQ(file, rank));
			if (sq != 65) {
				shield_zone |= 1ULL << sq;
			}
		}
	}

	return shield_zone;
}

static inline int16_t pawn_shield(const S_BOARD *pos, uint8_t kingSq, uint8_t col) {

	
	
	
	
	const int PawnShield[4] = { 0, -10, -18, -125 }; 
	U64 castled_king = 0ULL;
	int16_t shield = 0;

	if (col == WHITE) {
		
		castled_king = RankBBMask[RANK_1];
		castled_king ^= (1ULL << SQ64(D1)) | (1ULL << SQ64(E1)) | (1ULL << SQ64(F1)); 
		if (castled_king & (1ULL << SQ64(kingSq))) {
			U64 king_zone = generate_shield_zone(kingSq, WHITE);
			U64 pawns_in_zone = pos->pawns[WHITE] & king_zone;

			uint8_t bits = CountBits(pawns_in_zone);
			if (bits < 3) {
				
				shield += PawnShield[3] * (3 - bits);
			}

			while (pawns_in_zone) {
				uint8_t pawn_sq = PopBit(&pawns_in_zone);
				uint8_t rank = pawn_sq / 8;
				shield += PawnShield[rank - 2];
			}
		}
	} 
	else {
		castled_king = RankBBMask[RANK_8];
		castled_king ^= (1ULL << SQ64(D8)) | (1ULL << SQ64(E8)) | (1ULL << SQ64(F8)); 
		if (castled_king & (1ULL << SQ64(kingSq))) {
			U64 king_zone = generate_shield_zone(kingSq, BLACK);
			U64 pawns_in_zone = pos->pawns[BLACK] & king_zone;

			uint8_t bits = CountBits(pawns_in_zone);
			if (bits < 3) {
				
				
				shield += PawnShield[3] * (3 - bits);
			}

			while (pawns_in_zone) {
				uint8_t pawn_sq = PopBit(&pawns_in_zone);
				uint8_t rank = pawn_sq / 8;
				shield += PawnShield[7 - rank];
			}
		}
	}

	return shield;

}

/*
	Other components
*/



static inline int16_t punish_center_kings(const S_BOARD *pos, uint8_t king_sq, uint8_t col) {
	
	const int file_punishment[8] = { -10, -30, -50, -70, -70, -50, -30, -10 };
	const int rank_punishment[8] = { 0, -25, -75, -125, -150, -175, -200, -225 };
	uint8_t no_castling = 0;
	uint8_t relative_rank = 0;
	if (col == WHITE) {
		no_castling = (pos->castlePerm & 0b0011) == 0;
		relative_rank = RanksBrd[king_sq];
	} else {
		no_castling = (pos->castlePerm & 0b1100) == 0;
		relative_rank = 8 - RanksBrd[king_sq];
	}

	if (no_castling) {
		
		return -50 + file_punishment[FilesBrd[king_sq]] + rank_punishment[relative_rank];
	} else {
		return 0;
	}

	/*
	int8_t no_castle_penalty = -23;
	uint8_t no_castling = 0;

	if (col == WHITE) {
		no_castling = (pos->castlePerm & 0b0011) == 0;
	} else {
		no_castling = (pos->castlePerm & 0b1100) == 0;
	}

	if (no_castling) {
		return no_castle_penalty;
	} else {
		return 0;
	}
	*/

}

static inline int16_t punish_king_open_files(const S_BOARD *pos, uint8_t col) {

	uint8_t opp_king_sq = pos->KingSq[col];
	uint8_t king_file = FilesBrd[opp_king_sq];
	const uint8_t KingOpenFile[3] = { 60, 70, 60 };
	int16_t open_lines = 0;

	for (int file = king_file - 1; file <= king_file + 1; ++file) {
		if (file >= FILE_A && file <= FILE_H) {
			
			if (!(pos->pawns[BOTH] & FileBBMask[file])) {
					open_lines -= KingOpenFile[file - king_file + 1];
			} 
		}
			
	}

	return open_lines;

}

/*
int8_t pawn_storm(const S_BOARD *pos, uint8_t kingSq, uint8_t col) {

	uint8_t king_file = FilesBrd[kingSq];
	uint8_t relevant_base_rank = (col == WHITE) ? RANK_5 : RANK_4; 
	uint8_t relative_rank = 0;
	int8_t pawn_storm = 0;

	
	if (king_file <= FILE_C || king_file >= FILE_G) {
		for (int file = king_file - 1; file <= king_file + 1; ++file) {
			if (file >= FILE_A && file <= FILE_H) {
				U64 pawn_storm_mask = FileBBMask[file] & pos->pawns[!col];
				while (pawn_storm_mask) {
					uint8_t pawn_sq = PopBit(&pawn_storm_mask);
					uint8_t pawn_rank = RanksBrd[SQ120(pawn_sq)];
					
					if ( ( (col == WHITE) && (pawn_rank <= relevant_base_rank) ) || 
						( (col == BLACK) && (pawn_rank >= relevant_base_rank) ) ) {
						relative_rank = abs(relevant_base_rank - pawn_rank);
						pawn_storm += PawnStormPenalty - relative_rank;
					}
				}
			}
		}
	}
	

	return pawn_storm;
}
*/

static inline double king_safety_score(const S_BOARD *pos, uint8_t kingSq, uint8_t col, uint16_t mat) {
	
	
	

	double king_safety = 0;
	king_safety += punish_king_open_files(pos, col) * 0.7;
	
	king_safety += king_tropism(pos, col) * 0.6;
	king_safety += pawn_shield(pos, kingSq, col) * 0.35;
	king_safety += punish_center_kings(pos, kingSq, col) * 0.15;
	

	
	
	uint8_t phase = 3 * (pos->pceNum[wN] + pos->pceNum[bN] + pos->pceNum[wB] + pos->pceNum[bB]) +
                	5 * (pos->pceNum[wR] + pos->pceNum[bR]) + 
					10 * (pos->pceNum[wQ] + pos->pceNum[bQ]);
	return king_safety * phase / 64; 

}

/*************************
*** Endgame Adjustment ***
*************************/





static inline int16_t king_mobility(const S_BOARD *pos, uint8_t king_sq, uint8_t col) {

	const int KiDir[8] = { -1, -10,	1, 10, -9, -11, 11, 9 }; 
	
	const int mobility_bonus[9] = { -75, -50, -33, -25, 0, 5, 10, 11, 12};

	uint8_t mobile_squares = 0;
	for (int i = 0; i < 8; ++i) {
		if (SQ64(king_sq + KiDir[i]) != 65) {
			if (!SqAttacked(king_sq + KiDir[i], !col, pos)) {
				mobile_squares++;
			}
		}
		
	}

	return mobility_bonus[mobile_squares];
}


static inline double CountMaterial(const S_BOARD *pos, double weight, double *whiteMat, double *blackMat) {
	
	*whiteMat = 0;
	*blackMat = 0;

	for(int index = 0; index < BRD_SQ_NUM; ++index) {
		int piece = pos->pieces[index];
		ASSERT(PceValidEmptyOffbrd(piece));
		
		if (piece != OFFBOARD && piece != EMPTY) {
			uint8_t col = PieceCol[piece];
			ASSERT(SideValid(col));
			if (col == WHITE)
				*whiteMat += PieceValMg[piece] * weight + PieceValEg[piece] * (1 - weight);
			else 
				*blackMat += PieceValMg[piece] * weight + PieceValEg[piece] * (1 - weight);
		}
	}

	return *whiteMat - *blackMat;

}

/********************************
*** Main Evaluation Function ****
********************************/


inline int16_t EvalPosition(const S_BOARD *pos) {

	ASSERT(CheckBoard(pos));

	uint8_t pce;
	int pceNum;
	uint8_t sq;

	double score = 0;
	
	double weight = evalWeight(pos);

	/*
		Material
	*/

	double whiteMaterial = 0;
	double blackMaterial = 0;
	double net_material = CountMaterial(pos, weight, &whiteMaterial, &blackMaterial);
	score += net_material;

	
	
	
	uint8_t piece_count = CountBits(pos->occupancy[BOTH]) - CountBits(pos->pawns[BOTH]);
	uint8_t isEndgame = piece_count < 8;
	if (isEndgame) {
		if ( !pos->pceNum[wP] && !pos->pceNum[bP] && is_material_draw(pos, (int)fabs(net_material)) ) {
			return 0;
		}
	}

	/*
		Evaluate pawns
	*/

	pce = wP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += PawnMgTable[SQ64(sq)] * weight + PawnEgTable[SQ64(sq)] * (1 - weight);
		score += evaluate_pawn_structure(pos, sq, WHITE);
	}

	pce = bP;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= PawnMgTable[MIRROR64(SQ64(sq))] * weight + PawnEgTable[MIRROR64(SQ64(sq))] * (1 - weight);	
		score -= evaluate_pawn_structure(pos, sq, BLACK);
	}

	/*
		Evaluate knights
	*/

	pce = wN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += KnightMgTable[SQ64(sq)] * weight + KnightEgTable[SQ64(sq)] * (1 - weight);

		
		U64 mask = pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]];
		int pawnSq = PopBit(&mask);
		if ( (SQ64(sq) - pawnSq == 8) && (FilesBrd[sq] == FILE_C) ) {
			score += KnightBlocksPawn;
		}
	}

	pce = bN;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= KnightMgTable[MIRROR64(SQ64(sq))] * weight + KnightEgTable[MIRROR64(SQ64(sq))] * (1 - weight);

		
		U64 mask = pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]];
		int pawnSq = PopBit(&mask);
		if ( (pawnSq - SQ64(sq) == 8) && (FilesBrd[sq] == FILE_C) ) {
			score -= KnightBlocksPawn;
		}
	}

	/*
		Evaluate bishops
	*/

	pce = wB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {

		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += BishopMgTable[SQ64(sq)] * weight + BishopEgTable[SQ64(sq)] * (1 - weight);

		
		U64 mask = pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]];
		int pawnSq = PopBit(&mask);
		if ( (SQ64(sq) - pawnSq == 8) && (FilesBrd[sq] == FILE_D)) {
			score += BishopBlocksPawn;
		} else {
			if ( (SQ64(sq) - pawnSq == 8) && (FilesBrd[sq] == FILE_E) ) {
				score += BishopBlocksPawn;
			}
		}
	}

	pce = bB;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {

		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= BishopMgTable[MIRROR64(SQ64(sq))] * weight + BishopEgTable[MIRROR64(SQ64(sq))] * (1 - weight);
		
		
		U64 mask = pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]];
		int pawnSq = PopBit(&mask);
		if ( (pawnSq - SQ64(sq) == 8) && (FilesBrd[sq] == FILE_D)) {
			score -= BishopBlocksPawn;
		} else {
			if ( (pawnSq - SQ64(sq) == 8) && (FilesBrd[sq] == FILE_E) ) {
				score -= BishopBlocksPawn;
			}
		}
	}
	
	/*
		Evaluate rooks
	*/

	pce = wR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += RookMgTable[SQ64(sq)] * weight + RookEgTable[SQ64(sq)] * (1 - weight);

		ASSERT(FileRankValid(FilesBrd[sq]));

		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += RookOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += RookSemiOpenFile;
		}
	}

	pce = bR;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= RookMgTable[MIRROR64(SQ64(sq))] * weight + RookEgTable[MIRROR64(SQ64(sq))] * (1 - weight);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= RookOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= RookSemiOpenFile;
		}
	}

	/*
		Evaluate queens
	*/

	pce = wQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		score += QueenMgTable[SQ64(sq)] * weight + QueenEgTable[SQ64(sq)] * (1 - weight);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score += QueenOpenFile;
		} else if(!(pos->pawns[WHITE] & FileBBMask[FilesBrd[sq]])) {
			score += QueenSemiOpenFile;
		}
	}

	pce = bQ;
	for(pceNum = 0; pceNum < pos->pceNum[pce]; ++pceNum) {
		sq = pos->pList[pce][pceNum];
		ASSERT(SqOnBoard(sq));
		ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
		ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
		score -= QueenMgTable[MIRROR64(SQ64(sq))] * weight + QueenEgTable[MIRROR64(SQ64(sq))] * (1 - weight);
		ASSERT(FileRankValid(FilesBrd[sq]));
		if(!(pos->pawns[BOTH] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenOpenFile;
		} else if(!(pos->pawns[BLACK] & FileBBMask[FilesBrd[sq]])) {
			score -= QueenSemiOpenFile;
		}
	}

	/*
		Evaluate kings
	*/

	
	pce = wK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(SQ64(sq)>=0 && SQ64(sq)<=63);
	score += KingMgTable[SQ64(sq)] * weight + KingEgTable[SQ64(sq)] * (1 - weight);
	if (isEndgame) {
		score += king_mobility(pos, sq, WHITE);
	} else {
		score += king_safety_score(pos, sq, WHITE, blackMaterial - 50000);
	}

	pce = bK;
	sq = pos->pList[pce][0];
	ASSERT(SqOnBoard(sq));
	ASSERT(MIRROR64(SQ64(sq))>=0 && MIRROR64(SQ64(sq))<=63);
	score -= KingMgTable[MIRROR64(SQ64(sq))] * weight + KingEgTable[MIRROR64(SQ64(sq))] * (1 - weight);
	if (isEndgame) {
		score -= king_mobility(pos, sq, BLACK);
	} else {
		score -= king_safety_score(pos, sq, BLACK, whiteMaterial - 50000);
	}

	/*
		Bonuses and Adjustments
	*/

	
	if(pos->pceNum[wB] >= 2) score += BishopPair;
	if(pos->pceNum[bB] >= 2) score -= BishopPair;

	
	if (score < ISMATE) {
		score = (int)( score * ( (100 - pos->fiftyMove) / 100.0) );
	}

	

	
	if(pos->side == WHITE) {
		return score;
	} else {
		return -score;
	}
}


















