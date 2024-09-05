## Sicilian Search 
Sicilian Search is a chess engine made in C language. It uses a variation of minimax algorithm called the negamax algorithm with alpha beta pruning along with other optimizations for quicker and more accurate searching. It uses an evalution function to evalute the score of any postion and does it for every root node of the game tree. It is compatible with UCI format.

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
