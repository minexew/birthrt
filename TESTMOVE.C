/* ========================================================================
   Function    - TestLegalMoves
   Description - check all legal moves
   Returns     - any legal moves
   ======================================================================== */
BOOL TestLegalMoves(SHORT Row, SHORT Column)
{
	SHORT i,j,NumTests,Mask;
	SHORT R,C;
	
	//GEH forgive the HACKyness of this code, I will try to clean
	//    it up a little later on...
	
	/* -----------------------------------------------------------------
	   Test up and left
	   ----------------------------------------------------------------- */

	// Movement paths tested
	NumTests = 0x0001 << Movement;
	
	for(i=0;i<NumTests;i++)
	{
		R = Row;
		C = Column;
		Mask = 0x0001;
		for(j=0;j<Movement;j++)
		{
			SHORT l;	// grid location
			
			if((Mask & i) == 0)	// move up
			{
				R += 2;
				
				if(R >= GRID_MAX_ROWS)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			else				// move left
			{
				C -= 1;
				
				if(C < 0)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			
			// if no one from my army is in this square, hilight
			if(WhosWhere[R][C] == -1)
			{
				SetButtonHilight( MenuId, l+2, TRUE);
			}
			else
			{
				break; // this path is invalid
			}
			
			// are enemy troops in this square? if so I can move here
			// but no further
			if(WhosWhere[R+1][C] != -1)
			{
				break; // end of valid path
			}
			
			Mask <<= 1; // shift to test next bit
		}
	}
	/* -----------------------------------------------------------------
	   Test up and right
	   ----------------------------------------------------------------- */

	// Movement paths tested
	NumTests = 0x0001 << Movement;
	
	for(i=0;i<NumTests;i++)
	{
		R = Row;
		C = Column;
		Mask = 0x0001;
		for(j=0;j<Movement;j++)
		{
			SHORT l;	// grid location
			
			if((Mask & i) == 0)	// move up
			{
				R += 2;
				
				if(R >= GRID_MAX_ROWS)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			else				// move right
			{
				C += 1;
				
				if(C >= GRID_MAX_COLS)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			
			// if no one from my army is in this square, hilight
			if(WhosWhere[R][C] == -1)
			{
				SetButtonHilight( MenuId, l+2, TRUE);
			}
			else
			{
				break; // this path is invalid
			}
			
			// are enemy troops in this square? if so I can move here
			// but no further
			if(WhosWhere[R+1][C] != -1)
			{
				break; // end of valid path
			}
			
			Mask <<= 1; // shift to test next bit
		}
	}
	/* -----------------------------------------------------------------
	   Test down and left
	   ----------------------------------------------------------------- */

	// Movement paths tested
	NumTests = 0x0001 << Movement;
	
	for(i=0;i<NumTests;i++)
	{
		R = Row;
		C = Column;
		Mask = 0x0001;
		for(j=0;j<Movement;j++)
		{
			SHORT l;	// grid location
			
			if((Mask & i) == 0)	// move down
			{
				R -= 2;
				
				if(R < 0)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			else				// move left
			{
				C -= 1;
				
				if(C < 0)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			
			// if no one from my army is in this square, hilight
			if(WhosWhere[R][C] == -1)
			{
				SetButtonHilight( MenuId, l+2, TRUE);
			}
			else
			{
				break; // this path is invalid
			}
			
			// are enemy troops in this square? if so I can move here
			// but no further
			if(WhosWhere[R+1][C] != -1)
			{
				break; // end of valid path
			}
			
			Mask <<= 1; // shift to test next bit
		}
	}
	/* -----------------------------------------------------------------
	   Test down and right
	   ----------------------------------------------------------------- */

	// Movement paths tested
	NumTests = 0x0001 << Movement;
	
	for(i=0;i<NumTests;i++)
	{
		R = Row;
		C = Column;
		Mask = 0x0001;
		for(j=0;j<Movement;j++)
		{
			SHORT l;	// grid location
			
			if((Mask & i) == 0)	// move down
			{
				R -= 2;
				
				if(R < 0)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			else				// move right
			{
				C += 1;
				
				if(C >= GRID_MAX_COLS)
					break; // this path is invalid
				
				l = ((R/2) * 5) + C;
			}
			
			// if no one from my army is in this square, hilight
			if(WhosWhere[R][C] == -1)
			{
				SetButtonHilight( MenuId, l+2, TRUE);
			}
			else
			{
				break; // this path is invalid
			}
			
			// are enemy troops in this square? if so I can move here
			// but no further
			if(WhosWhere[R+1][C] != -1)
			{
				break; // end of valid path
			}
			
			Mask <<= 1; // shift to test next bit
		}
	}
	return TRUE;
}

