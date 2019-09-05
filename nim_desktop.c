/***********************************************************
 * nim.c
 * 
 * This code is for a game where stick are arranged in rows.
 * It is played by taking turns to remove any amount of sticks
 * from any single row of your choice.
 * The player who takes the last stick is the loser.
 * This code is intended for microcontrollers, and the sticks
 * could be LEDs, or some other display.
 *
 * rev 1.0 - August 2019 - shabaz
 * Free for all non-commercial use
 ***********************************************************/

#include <stdio.h>

/********* definitions *****************/
// maximum possible rows containing sticks at the start of a game.
// limit is 8, due to this code using unsigned chars in places
#define MAXROWS 5


/******** global variables **************/
unsigned char numsticks[MAXROWS]; // this array holds the number of sticks in each row
unsigned char level=2; // difficulty, 1-3. 3 is hardest. The easier levels can have less rows.
unsigned char rows=4; // number of rows being played. Max is MAXROWS
unsigned short int randreg=1; // this variable holds a random number
unsigned char current_selection=0; // this variable stores what button was pressed


/****************************************
 * local functions
 ****************************************/
 
/* random_num
 * a random number generator
 */
unsigned char
random_num(void)
{
  char i;
  for (i=0; i<=7; i++)
  {
    randreg ^= randreg>>7;
    randreg ^= randreg<<9;
    randreg ^= randreg>>13;
  }
  return(unsigned char)(randreg & 0xff);
}

/* setup_game
 * this function initializes the numsticks array, and the number of rows in the
 * game, depending on difficulty level. */
void
setup_game(void)
{
  char i;
  switch(level)
  {
    case 3: // hardest. Random number of sticks in each row.
      rows=4;
      for (i=0; i<rows; i++)
      {
        numsticks[i]=random_num() & 0x07;
        numsticks[i]++; // value is between 1 and 8
      }
      numsticks[0]++; // one row can have up to 9
      break;
    case 2: // intermediate. Rows start with a pre-defined number of sticks.
      rows=4;
      numsticks[3]=7;
      numsticks[2]=5;
      numsticks[1]=3;
      numsticks[0]=1;
      break;
    case 1: // easy. Just three rows of pre-defined sticks.
      rows=3;
      numsticks[2]=5;
      numsticks[1]=3;
      numsticks[0]=1;
      break;
  }
}

/* user_play
 * updates the numsticks depending on button press. Returns 9 if
 * it is time for the computer to make a move. */
char
user_play(void)
{
  int selection;
  printf("row to decrement? [1-%d] or [9]computer move: ", rows);
  scanf("%d", &selection);
  if ((selection<=rows) && (selection>0)) // a row button was pressed
  {
    // check that the user isn't trying to take sticks from other rows!
    // once they have chosen a row, they have to stick with that row
    if ((current_selection==0) || (current_selection==selection))
    {
      if (numsticks[selection-1]>0)
      {
        numsticks[selection-1]--;
        current_selection=selection;
      }
    }
  }
  return(selection);
}

/* count_ones
 * this function counts up how many binary digits are 1
 */
unsigned char
count_ones(unsigned char value)
{
  char i;
  char sum=0;

  for (i=0; i<8; i++)
  {
    if ((value & (1<<i)) != 0)
      sum++;
  }
  return(sum);
}


/* computer_play
 * This function is the computer's algorithm, to try to beat the user.
 */
void
computer_play(void)
{
  unsigned char x=numsticks[0]; // x is the variable X in the Wikipedia article for Nim
  unsigned char interim_xor[MAXROWS]; // this array holds "the nim-sum of X and heap-size" for each row (see Wikipedia article for Nim)
  unsigned char playable_rows_bitmap=0;
  unsigned char num_playable_rows=0;
  unsigned char rows_remaining=0;
  unsigned char candidatemask=0;
  unsigned char candidate[MAXROWS]; // this array holds an idea for the the number of sticks to leave remaining in a row.
  unsigned char quality[MAXROWS]; // a quality value (higher is better) for how good the candidate idea is
  unsigned char peak_quality=0;
  unsigned char peak_candidate=0;
  unsigned char unitychecknotneeded=0; // we need to check how many rows have only one stick remaining, as part of the algorithm. This variable helps with the algorithm.
  
  int i;
  char j, k, temp, nomove;
  char unityheaps=0;
  
  // now the computer is playing. Reset the button selection for the user,
  // so that when it is their turn, they will be free to choose any row.
  current_selection=0;
  
  for (i=0; i<MAXROWS; i++)
  {
    quality[i]=0; // initialize the quality array
  }
  
  // find the best possible move, using the sum of
  // powers of two method, which is basically a 
  // lot of XORing
  for (i=1; i<rows; i++)
  {
    x=x^numsticks[i];
  }
  printf("x=%d\n", x);
  if (x>0)   
  {
    for (i=0; i<rows; i++)
    {
      interim_xor[i]=x^numsticks[i];
      if (interim_xor[i]<numsticks[i])
      {
        playable_rows_bitmap |= 1<<i;
      }
    }
    printf("interim_xor are %d, %d, %d, %d\n", interim_xor[0], interim_xor[1], interim_xor[2], interim_xor[3]);
    num_playable_rows=count_ones(playable_rows_bitmap);
    printf("num_playable_rows=%d\n", num_playable_rows);
    if (num_playable_rows>=1)
    {
      // overall strategy: find some possible moves, and give them a weight
      // and then we'll pick the best of the lot.
      for (i=rows-1; i>=0; i--)
      {
        if ((playable_rows_bitmap & (1<<i)) != 0) // i is a playable row
        {
          unitychecknotneeded=0;
          printf("testing candidate %d\n", i);
          temp=interim_xor[i]; // reduce size of heap to XOR of its original size with x
          // if we make the temp move real, will it leave only heaps with size 1?

          if (temp==1)
            unityheaps++;
          for (j=0; j<rows; j++)
          {
            if (j!=i)
            {
              if (numsticks[j]==1)
              {
                unityheaps++;
              }
              else if (numsticks[j]>1)
              {
                unitychecknotneeded=1;
              }
            }
          }
          if ((temp<=1) && (unitychecknotneeded==0))
          {
            // is unityheaps odd? We want that..
            if ((unityheaps & 1) != 0)
            {
              // make the move permanent
              candidate[i]=temp;
              quality[i]+=10;
              //numsticks[i]=temp;
            } // unityheaps would be even with that move. so take different action.
            else if (temp==1)
            {
              // we can reduce to zero.
              candidate[i]=0;
              quality[i]+=5; // this move should be good too
            }
            else
            {
              if (temp==0)
              {
                if (numsticks[i]>1)
                {
                  // we can leave one stick, to make it odd again
                  candidate[i]=1;
                  quality[i]+=9; // this move should be good
                }
              }
              else
              {
                // this won't be a nice move to make. But it is a valid move.
                candidate[i]=temp;
                quality[i]+=1;
              }
            }
          }
          else
          {
            // this action won't result in all heaps containing 1
            // it could be a good move.
            candidate[i]=temp;
            quality[i]+=9;
            
          }
          printf("candidate %d quality is %d\n", i, quality[i]);
          
        } // end of if ((playable_rows_bitmap & 1<<i) != 0)
        // loop to get another candidate
      } // end of for (i=rows-1; i>=0; i--)
      // ok we have at least one playable move. Find the highest quality move.
      printf("quality table: %d %d %d %d\n", quality[0], quality[1], quality[2], quality[3]);
      printf("finding highest quality move\n");
      for (i=0; i<rows; i++)
      {
        if (quality[i]>=peak_quality)
        {
          peak_quality=quality[i];
          peak_candidate=i;
          printf("best so far is candidate %d\n", i);
        }
      }
      numsticks[peak_candidate]=candidate[peak_candidate];
      
      
    } // end of if (num_playable_rows>=1)
    else
    {
      // we don't have a playable row in the bitmap!
      // play any row we can..
      for (i=0; i<rows; i++)
      {
        if (numsticks[i]>0)
        {
          numsticks[i]--;
          i=rows;
        }
      }
    }
  } // end of if (x>0)
  else
  {
    // no strategy any more. play any row we can..
    for (i=0; i<rows; i++)
    {
      if (numsticks[i]>0)
      {
        numsticks[i]--;
        i=rows;
      }
    }
  }
}

void
show_status(void)
{
  char i;
  
  printf("\n");
  for (i=0; i<rows; i++)
  {
    printf("%d:  ", i+1);
  }
  printf("\n");
  for (i=0; i<rows; i++)
  {
    printf("%d   ", numsticks[i]);
  }
  printf("\n");
}

int
main(void)
{
  int i;
  char ret;
  
  setup_game();
  while(1)
  {
     show_status();
     ret=user_play();
     if (ret==9)
       computer_play();
  }
}
 







