#include "defs.h"
#include <stdint.h>

inline uint8_t is_material_draw(const S_BOARD *pos, int net_material)
{

#define MAX_MINOR_PIECE 310

	if (net_material <= MAX_MINOR_PIECE)
	{

		uint8_t man_count = CountBits(pos->occupancy[BOTH]);

		uint8_t wQ_num = pos->pceNum[wQ];
		uint8_t wR_num = pos->pceNum[wR];
		uint8_t wB_num = pos->pceNum[wB];
		uint8_t wN_num = pos->pceNum[wN];

		uint8_t bQ_num = pos->pceNum[bQ];
		uint8_t bR_num = pos->pceNum[bR];
		uint8_t bB_num = pos->pceNum[bB];
		uint8_t bN_num = pos->pceNum[bN];

		if (man_count == 7)
		{
			if (
					((wQ_num == 1) && (wN_num == 1) && (bR_num == 1) && (bB_num == 1) && (bN_num)) ||
					((bQ_num == 1) && (bN_num == 1) && (wR_num == 1) && (wB_num == 1) && (wN_num)))
			{

				return FALSE;
			}
			else if (
					((wQ_num == 1) && ((bN_num + bB_num) == 4)) ||
					((bQ_num == 1) && ((wN_num + wB_num) == 4)))
			{

				return FALSE;
			}
		}

		if (man_count == 6)
		{
			if (
					((wQ_num == 1) && (wB_num == 1) && (bR_num == 2)) ||
					((bQ_num == 1) && (bB_num == 1) && (wR_num == 2)))
			{

				return FALSE;
			}
			else if ((man_count == 6) && (wQ_num == 1) && (wR_num == 1) && (bQ_num == 1) && (bR_num == 1))
			{

				return FALSE;
			}
			else if (
					((wR_num == 1) && (wB_num == 1) && (bN_num == 2)) ||
					((bR_num == 1) && (bB_num == 1) && (wN_num == 2)))
			{

				return FALSE;
			}
			else if (
					((wB_num == 1) && (wN_num == 2) && (bR_num == 1)) ||
					((bB_num == 1) && (bN_num == 2) && (wR_num == 1)))
			{

				return TRUE;
			}
			else if (
					((wR_num == 1) && (wB_num == 1) && (bB_num == 1) && (bN_num)) ||
					((bR_num == 1) && (bB_num == 1) && (wB_num == 1) && (wN_num)))
			{

				if (isOppColBishops(pos))
				{
					return FALSE;
				}
				else
				{
					return TRUE;
				}
			}
		}

		if (man_count == 5)
		{
			if (
					((wQ_num == 1) && (bN_num == 2)) ||
					((bQ_num == 1) && (wN_num == 2)))
			{

				return TRUE;
			}
		}

		if (man_count == 4)
		{
			if ((man_count == 4) && ((wN_num == 2) || (bN_num == 2)))
			{

				return TRUE;
			}
		}

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}