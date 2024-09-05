## Sicilian Search 
Sicilian Search is a chess engine made in C language. It uses a variation of minimax algorithm called the negamax algorithm with alpha beta pruning along with other optimizations for quicker and more accurate searching. It uses an evalution function to evalute the score of any postion and does it for every root node of the game tree. It is compatible with UCI format.

## How to run the chess engine
Run the command `make CC=gcc` in the project directory to generate .exe file. This file can be added to any UCI compatable chess playing software.

## How does the search work?
The search algorithm starts with iterative deepening, which increases the search depth and uses alpha-beta pruning to explore moves. The algorithm adjusts the search window as depth increases to reduce the search space. It applies null move pruning to skip moves that don't improve the position and quiescence search to handle tactical situations by focusing on capture sequences. Transposition tables store previously evaluated positions, and negamax simplifies score calculations by flipping values during recursion. The search stops when it finds checkmate, exceeds the time limit, or encounters a beta cutoff.
The working on the functions in the search.c file are explained below
- Initialize search by setting best move, best score, table, pos, info to 0 or initial value
- Get moves from opening book on Polybook
- using iterative deepening on a modified version of alpha beta pruning is called recursively
	- do full search for 1st depth
	- use window size of 50 by default for the rest
		- if depth > 6, decrement window size as depth increase to decrease the search space
		- if the the score falls outside the range of guess + or - , then retry full search without window
	- check stop flag and break if required
	- get principal variation (the most competitive line where both sides keep playing the best moves) and store it
	- display mate if there was forced mate
	- exit search at current depth is mate is found
- return the best move
Note that the search function breaks only if mate is found or time limit is exceeded.

The modified version of alpha beta pruning function is explained below
- if depth <=0, then Quiescence is called
- every 2048 nodes visited, the algorithm check if the engine has been stopped due to time limit
- if position is repeated or 50 move rule applies then the function returns 0
- if the max depth is reach then the board is evaluated using a static evaluation function
- if the player is in check, the search depth is increased to find a way out
- check transposition table if its present there
- null move pruning, checks if the player can skip a move and still have a position that is better than beta then the current position is not worth exploring. This is not used when the player is in check or has insufficient 
- Generate all possible legal moves
- Sorts the moves which are more likely to cutoff earlier(captures or previously proven good moves)
- Check if the move doesn't open the king up
- Call the alpha beta pruning recursively. This is done use Negamax instead of plain old minimax as it just flips the sign instead of changing from from finding max to min for each move.
- if the depth is 1 or 2 (shallow), and the static evaluation is significantly worse than alpha and the move is not a capture or a check, then it is unlike to improve the position so it is pruned
- if depth > 4, and the move is a non capture and non promotion, the search depth is reduced to focus on stronger moves
- update the best score with alpha, but if a score produces a score greater than beta, the beta is returned(cutoff)
- Finally the transposition table or hash table stored all the moves that were stored as best score in the table for quick calculation from that point

The Quiescene function is explain below
This search is done mainly to calculate tactics which are a long sequence of captures
- check conditions to stop search like max depth and time
- find static evaluation and use that as a baseline to compare any tactical sequence with
- delta pruning, if the current position(after some capture) is highly losing compared to alpha, then the search can be pruned
- generate list of all captures possible
- iterate through all possible captures
	- execute them
	- call quiescene function recursively(using same method as negamax, just flipping sign)
	- undo the move and return the value of the function
- return alpha the best possible evaluation for current postion

## How does the evaluation function work?
- evaluate_pawn_structure: 
	Used to reward pawns based on whether they are isolated, doubled or passed. 
	- PawnIsolated = -10
	- PawnDoubled = -10
	- PawnPassed[8] = {0,5,10,20,35,60,100,200}
- eval_weight:
	Used to gauge which stage of the game it is. 0 for opening stage and 1 for end of the game
	- gamePhase is calculated by min((3*(no. of knights and bishops) + 5*(no. of rooks) + 10 * (no. of queens)),64) to cap 64 as the gamePhase value for openings
	- returns gamePhase/64 to ensures its between 0 to 1
- king_tropism
	- calculate how far knight, bishops, rooks and queen is from the opposing king and reward being closer to the opposing king
	- factor*(no. of rook moves to reach king(Manhattan distance))
- attack_units & king_zone
	- count the number of attacking and defending pieces in the kings zone
	- kings zone is padded 2 squares on the sides and 3 squares down the board
- pawn_shield & shield_zone
	- calculates the shield strength of king based on how many pawns and how far advanced the pawns in front of it are
- punish_center_kings
	- calculates a penalty based on how close the king is to the center of the board unless it still has castling rights
	- base penalty = -50
	- file punishment = {-10,-30,-50,-70,-70,-50,,-30,-10}
	- rank punishment = {0, -25, -75, -125, -150, -175}
	- returns -50 + file_punishment(FilesBrd(king_sq)) + rank_punishment(relative_rank)
- punish_king_open_files
	- punished if the king is on open file or has neighbouring open files
	- {60,70,60}
- king_safety_score
	- punish_king_open_file with factor of 0.7
	- king_tropism with factor of 0.6
	- pawn_shield with factor of 0.35
	- punish_center_king withh factor of 0.15
	- pawn_storm with factor of 0.3
	- return sum of all this as king_safety_score
- king_mobility
	- score based on how many squares are available for the king to go to
	- {-75, -50, -33, -25, 0, 5, 10, 11, 12}
- count_material
	- total material count based on piece value in that part of the game(opening, middlegame or endgame)
	- piece value predefined using tables in evaluate.h
- rook_on_seventh_rank
	- score of 12 if a rook is on the seventh rank, 24 if both rooks are on the eights rank
- doubled_rooks
	- if rooks are on same file or rank with no piece inbetween then score is 30
- castled_king
	- score +90 for castled king
- bishop_mobility
	- count of squares that is in the bishops range before it meets an enemy pawn or the end of the board
	- score 5* count
- fianchettoed_bishop
	- score 40 if bishop is fianchettoed perfectly
- pawn_shield_for_minor_pieces
	- score +5 for each minor piece protected by pawns
	- check for opponent pawns in the that file or adjacent rank if true give +20, and along with that if no bishop or knights left for opponent +80
