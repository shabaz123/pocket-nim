/***********************************************************
 * pocket-nim
 * This code is for a game where stick are arranged in rows.
 * It is played by taking turns to remove any amount of sticks
 * from any single row of your choice.
 * The player who takes the last stick is the loser.
 * This code is intended for microcontrollers, and the sticks
 * could be LEDs, or some other display.
 *
 * This current version works with Infineon XMC 2GO
 * microcontroller board (
 *
 * rev 1.0 - August 2019 - shabaz
 * Free for all non-commercial use
 ***********************************************************/

// # uncomment to enable debug
//#define DO_DEBUG

/*************** include files ***************/
#include <DAVE.h>
#ifdef DO_DEBUG
#include <stdio.h>
#endif

/*************** definitions *****************/
#define led_address 0xe0
// maximum possible rows containing sticks at the start of a game.
// limit is 8, due to this code using unsigned chars in places
#define MAXROWS 5
#define NUM_BUTTONS 6
#define COMPUTER_BUTTON NUM_BUTTONS-1
#define FOREVER 1

// deliberate weakening on the easier levels
// lower number makes the computer play weaker.
// 200U is about right it seems. Set to (say) 128U for an easier game
#define WEAKNESS 200U

// button status definitions
#define UNPRESSED 0
#define FIRST_PRESS 1
#define PRESS_ACTIONED 2

// button debounce times
#define MILLISEC 1000
#define DEBOUNCE_TICK_PERIOD 60
#define RELEASE_TICK_PERIOD 60

// display related
#define ORIENTATION 0

// debug related
#define HEARTBEAT_DELAY 500

/****** const variables *****************/
const uint8_t display_init_data[3]={0x21, 0x81, 0xef}; // system osc. on, display on, max brightness
// const bitmap for alphabet font is near the end of the file

/******** global variables **************/
unsigned char numsticks[MAXROWS]; // this array holds the number of sticks in each row
unsigned char level=2; // difficulty, 1-5 with 5 being is hardest. The easier levels can have less rows.
unsigned char rows=4; // number of rows being played. Max is MAXROWS
unsigned short int randreg=1; // this variable holds a random number
unsigned char current_selection=0; // this variable stores what button was pressed

uint32_t timer_id;

DIGITAL_IO_t* button_handle[NUM_BUTTONS];
unsigned char button_status[NUM_BUTTONS];
unsigned char do_all_button_inhibit=0; // a lockout for not accepting any button presses
unsigned int press_ticks=0; // a counter used primarily for debounce purposes
unsigned char command_press=0; // A button press after the computer button was held down
char playing=0; // this variable is set to 1 when a game is being played

uint16_t display_ram[8];
unsigned int general_timer=0;  // used for debug
unsigned int display_update_timer=0; // used to allow the display some time after updates
unsigned int heartbeat_timer=HEARTBEAT_DELAY; // used to flash an LED on the microcontroller board

/***** extern function prototypes *******/
#ifdef DO_DEBUG
extern void initialise_monitor_handles(void); // used to allow printf to work
#endif

/******** function prototypes ***********/
// core game algorithm related
void setup_game(void);
void show_status(void);
char user_play(void);
void computer_play(void);

// button related
void fast_tick(void);
char a_button_pressed(void);

// display related
void display_init(void);
void display_write(void);
void display_ram_blank(void);
void plot_ram_pixel(int x, int y);
void plot_ram_rows(unsigned char* rows_arr);
void scroll_text(char* text, char len, char all);

// sound related
void play_tone(char type);

// debug related
void set_led(char state); // controls LED2 on the microcontroller board

/****************************************
 * main function
 ****************************************/
int main(void)
{
  DAVE_STATUS_t status;
  int i;
  char sel;
  char check_winner;
  char winner_announced;
  unsigned char oldnumsticks[MAXROWS]; // used to blink the computer move a few times on the display

  status = DAVE_Init();           /* Initialization of DAVE APPs  */
  if(status != DAVE_STATUS_SUCCESS)
  {
    XMC_DEBUG("DAVE APPs initialization failed\n");
    while(1);
  }

  set_led(1); // turn on the LED on the microcontroller board (LED2) briefly for debug purposes

  // create and start up the timer for the periodic tick function
  timer_id=(uint32_t)SYSTIMER_CreateTimer(MILLISEC,
		   SYSTIMER_MODE_PERIODIC,(void*)fast_tick,NULL);
  SYSTIMER_StartTimer(timer_id);
#ifdef DO_DEBUG
  initialise_monitor_handles();
#endif

display_update_timer=10;
while(display_update_timer>0); // delay to allow power to settle
display_ram_blank();
display_init();
display_update_timer=100;
while(display_update_timer>0); // delay to allow display to be initialised
set_led(0);

#ifdef DO_DEBUG
  printf("Hello\n");
#endif

  scroll_text("HELLO  ", 7, 0); // lowercase is not supported! And pad with 2 spaces at the end.

  button_handle[0]=(DIGITAL_IO_t*)&button1;
  button_handle[1]=(DIGITAL_IO_t*)&button2;
  button_handle[2]=(DIGITAL_IO_t*)&button3;
  button_handle[3]=(DIGITAL_IO_t*)&button4;
  button_handle[4]=(DIGITAL_IO_t*)&button5;
  button_handle[5]=(DIGITAL_IO_t*)&button_computer;
  for (i=0; i<NUM_BUTTONS; i++)
  {
	  button_status[i]=UNPRESSED;
  }

  while(FOREVER)
  {
    setup_game();
    winner_announced=0; // no-one has won this new game yet
    show_status();
    // wait in case a button is pressed, for it to be released
    while(a_button_pressed());
    playing=1;
    while(playing)
    {
       sel=user_play();
       if (sel==9) // a selection of 9 means the user has pressed the Computer button
       {
         // time for the computer to play. But first check, has the
         // user actually won?
         check_winner=0;
         for (i=0; i<rows; i++)
         {
           check_winner+=numsticks[i];
         }
         if (check_winner==1) // user has won
         {
           if (winner_announced==0)
           {
             display_update_timer=1000;
             while(display_update_timer); // wait a bit. Because the computer is a sore loser
             play_tone(1); // play rising tone
             scroll_text("YOU WIN  ", 9, 0);
             winner_announced=1;
           }
         }
         else
         {
           check_winner=0;
         }
         // make a backup of the number of sticks before the computer plays
         // so we can do a blinking animation of the computer move
         for (i=0; i<rows; i++)
         {
           oldnumsticks[i]=numsticks[i];
         }
         if (winner_announced==0)
         {
           computer_play();
         }
         // lets blink the computer played move a few times
         for (i=0; i<2; i++)
         {
           plot_ram_rows(numsticks);
           display_write();
           display_update_timer=200;
           while(display_update_timer);
           plot_ram_rows(oldnumsticks);
           display_write();
           display_update_timer=200;
           while(display_update_timer);
         }
         // has computer won?
         if ((check_winner==0) && (winner_announced==0)) // computer has not lost yet..
         {
           for (i=0; i<rows; i++)
           {
             check_winner+=numsticks[i];
           }
           if (check_winner==1) // computer won
           {
             show_status();
             display_update_timer=1000;
             while(display_update_timer);
             play_tone(0); // play falling tone
             scroll_text("LOSER  ", 7, 0);
             winner_announced=1;
           }
         }
       }
       else if (sel>100) // this signifies that a command has arrived (Computer button was held down and another button pressed)
       {
         // start a new game, at the level selected in the command
         level=sel-100;
         playing=0;
         command_press=0;
       }
       show_status();
    }
  }

  return(0); // this should never execute
}



/****************************************
 * functions
 ****************************************/

/* a_button_pressed returns 1 if any button is pressed according to the button press
 * status array
 */
char
a_button_pressed(void)
{
  char status=0; // assume no button is pressed
  unsigned char i;
  for (i=0; i<NUM_BUTTONS; i++)
  {
    if (button_status[i]!=UNPRESSED)
    {
      status=1; // a button was pressed!
      break;
    }
  }
  if (status==0)
  {
    if (do_all_button_inhibit==1)
      status=1; // we're in a button release debounce period. Consider it still pressed!
  }

  return(status);
}

/* fast_tick
 * occurs every millisecond
 * we use this single timer to do several things,
 * but mainly to handle button debounce
 */
void
fast_tick(void)
{
	unsigned char pressed=0;
	unsigned char i;
	unsigned char already_recorded_press=0;

	randreg++; // this acts like a seed to the random number generator

	// some timers that can be set and read from the application
	if (general_timer>0)
	  general_timer--;

	heartbeat_timer--;
	if (heartbeat_timer==0)
	{
	  heartbeat_timer=HEARTBEAT_DELAY;
	  DIGITAL_IO_ToggleOutput(&led2);
	}

	// this timer is important for giving the display time to do its thing.
	// otherwise the display can hang.
	if (display_update_timer>0)
	  display_update_timer--;

	// Handle button presses. The strategy is that on each tick, the
	// buttons are checked, and if one is pressed then it is registered
	// as FIRST_PRESS in the button_status array. But if the array
	// contains any non-UNPRESSED value, then no button is
	// registered. After a minimum of a debounce period, the button is
	// unregistered, if it has been released.
	// The point of all this is to do button debounce, but also to
	// immediately take action on a button press (and not wait for
	// a debounce period, or for the button to be released).
	// No button is allowed to be registered until a further
	// minimum of a release period.

	if (do_all_button_inhibit==0)
	{
		// check for button newly pressed
		for (i=0; i<NUM_BUTTONS; i++)
		{
			if (button_status[i]==UNPRESSED)
			{
				if (DIGITAL_IO_GetInput(button_handle[i])!=1)
				{
					pressed=i+1;
				}
			}
			else
			{
				// a button is already recorded as pressed
				already_recorded_press=i+1;
			}
		}
		// special case. Was the computer button first held down, and
		// then another button also pressed? That is a special command
		// (i.e. to start the gave over
		if (button_status[NUM_BUTTONS-1]!=UNPRESSED)
		{
		  if ((pressed>0) && (pressed<NUM_BUTTONS))
		  {
		    if (playing)
		    {
		      command_press=pressed;
		    }
		  }
		}

		if ((already_recorded_press==0) && (pressed>0))
		{
			button_status[pressed-1]=FIRST_PRESS;
#ifdef DO_DEBUG
			printf("pressed: %d\n", pressed-1);
#endif
			press_ticks=0;
		}
		else if (already_recorded_press>0) // a button was already recorded as pressed
		{
			press_ticks++;
			if (press_ticks<DEBOUNCE_TICK_PERIOD)
			{
				// we assume button remains pressed for at least the
				// debounce period
			}
			else
			{
				if (DIGITAL_IO_GetInput(button_handle[already_recorded_press-1])!=1)
				{
					// the button is still pressed.
				}
				else
				{
					// the button is released.
					button_status[already_recorded_press-1]=UNPRESSED;
					press_ticks=0;
					do_all_button_inhibit=1;
				}
			}
		}
	}
	else
	{
		// button presses are being ignored.
		press_ticks++;
		if (press_ticks<RELEASE_TICK_PERIOD)
		{
			// we're not done.. keep inhibiting button presses
		}
		else
		{
			do_all_button_inhibit=0;
			press_ticks=0;
		}
	}
}

/* random_num
 * a random number generator. The magic numbers in this function represent wiring in
 * a linear feedback shift register, for a pseudorandom sequence.
 * The returned random number is in the range 0..255
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
  unsigned char i;
  switch(level)
  {
    case 5: // hardest. Random number of sticks in each row.
      // Note: the 8x8 LED matrix implementation will only have 4 rows
      // and 4 buttons, so this level selection will not be possible.
      rows=5;
      for (i=0; i<rows; i++)
      {
        numsticks[i]=random_num() & 0x07;
        numsticks[i]++; // value is between 1 and 8
      }
      //numsticks[0]++; // one row can have up to 9 (not possible with this display)
      break;
    case 4: // hard. Random number of sticks in each row.
      rows=4;
      for (i=0; i<rows; i++)
      {
        numsticks[i]=random_num() & 0x07;
        numsticks[i]++; // value is between 1 and 8
      }
      break;
    case 3: // moderate. Rows start with a pre-defined number of sticks.
      rows=4;
      numsticks[3]=7;
      numsticks[2]=5;
      numsticks[1]=3;
      numsticks[0]=1;
      break;
    case 2: // intermediate. Rows start with a pre-defined number of sticks, and weakened play by the computer
      rows=4;
      numsticks[3]=7;
      numsticks[2]=5;
      numsticks[1]=3;
      numsticks[0]=1;
      break;
    case 1: // easy. Just three rows of pre-defined sticks and weakened play by the computer
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
  unsigned char waiting_for_press=1;
  int selection=0;
  unsigned char i;

  while(waiting_for_press)
  {
    for (i=0; i<NUM_BUTTONS; i++)
    {
      if (button_status[i]==FIRST_PRESS)
      {
    	  // a button has just been pressed. Action it.
    	  selection=i+1;
        button_status[i]=PRESS_ACTIONED;
        waiting_for_press=0;
    	  if (i==COMPUTER_BUTTON)
    	  {
    	    selection=9; // arbitrarily use 9 to represent the computer move button
    	    // unlike the other buttons, we wait for the computer button to be
    	    // released before actioning it. This is maybe a bit more intuitive for
    	    // the computer move, but more importantly, we want to catch if really
    	    // the user is issuing an overall game command to start a new game
    	    while(button_status[COMPUTER_BUTTON]==PRESS_ACTIONED)
    	    {
    	      if (command_press)
    	        break;
    	    }
    	  }
    	  break;
      }
    }
    if (command_press)
    {
      // A dual button sequence. The computer play button was held down,
      // and another button was pressed. We use it to start the game again,
      // at a level depending on what button was pressed.
      // we use the number 100 to encode that this game command has been invoked.
      selection=100+command_press;
      current_selection=0; // reset, because we're starting a new game soon..
    }
  }

#ifdef DO_DEBUG
  XMC_DEBUG("row to decrement? [1-%d] or [9]computer move: ", rows);
  printf("selection is %d", selection);
  //TODO //scanf("%d", &selection);
#endif
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
  unsigned char candidate[MAXROWS]; // this array holds an idea for the the number of sticks to leave remaining in a row.
  unsigned char quality[MAXROWS]; // a quality value (higher is better) for how good the candidate idea is
  unsigned char peak_quality=0;
  unsigned char peak_candidate=0;
  unsigned char unitychecknotneeded=0; // we need to check how many rows have only one stick remaining, as part of the algorithm. This variable helps with the algorithm.

  int i;
  char j, temp;
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
  XMC_DEBUG("x=%d\n", x);
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
    XMC_DEBUG("interim_xor are %d, %d, %d, %d\n", interim_xor[0], interim_xor[1], interim_xor[2], interim_xor[3]);
    num_playable_rows=count_ones(playable_rows_bitmap);
    XMC_DEBUG("num_playable_rows=%d\n", num_playable_rows);
    if (num_playable_rows>=1)
    {
      // overall strategy: find some possible moves, and give them a weight
      // and then we'll pick the best of the lot.
      for (i=rows-1; i>=0; i--)
      {
        if ((playable_rows_bitmap & (1<<i)) != 0) // i is a playable row
        {
          unitychecknotneeded=0;
          XMC_DEBUG("testing candidate %d\n", i);
          temp=interim_xor[i]; // reduce size of heap to XOR of its original size with x
          // if we make the temp move real, will it leave only heaps with size 1?

          if (temp==1)
            unityheaps++;
          for (j=0; j<rows; j++)
          {
            if (j!=i)
            {
              if (numsticks[(unsigned char)j]==1)
              {
                unityheaps++;
              }
              else if (numsticks[(unsigned char)j]>1)
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
          XMC_DEBUG("candidate %d quality is %d\n", i, quality[i]);

        } // end of if ((playable_rows_bitmap & 1<<i) != 0)
        // loop to get another candidate
      } // end of for (i=rows-1; i>=0; i--)
      // ok we have at least one playable move. Find the highest quality move.
      XMC_DEBUG("quality table: %d %d %d %d\n", quality[0], quality[1], quality[2], quality[3]);
      XMC_DEBUG("finding highest quality move\n");
      for (i=0; i<rows; i++)
      {
        if (quality[i]>=peak_quality)
        {
          peak_quality=quality[i];
          peak_candidate=i;
          XMC_DEBUG("best so far is candidate %d\n", i);
        }
      }
      // change the quality values to make the computer play weaker
      // depending on level.
      if ((level==1) || (level==2))
      {
        for (i=0; i<rows; i++)
        {
          if (numsticks[i]>1)
          {
            if (random_num()>WEAKNESS)
            {
              candidate[i]=numsticks[i]-1;
              quality[i]=peak_quality+1;
              if (random_num()>128U)
              {
                quality[i]++;
              }
            }
          }
        }
        // now find peak candidate again
        peak_quality=0;
        for (i=0; i<rows; i++)
        {
          if (quality[i]>=peak_quality)
          {
            peak_quality=quality[i];
            peak_candidate=i;
            XMC_DEBUG("weakened best candidate so far %d\n", i);
          }
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
#ifdef DO_DEBUG
  char i;
#endif

  plot_ram_rows(numsticks);
  display_write();

#ifdef DO_DEBUG
  printf("\n");
  for (i=0; i<rows; i++)
  {
	  printf("%d:  ", i+1);
  }
  printf("\n");
  for (i=0; i<rows; i++)
  {
	  printf("%d   ", numsticks[(unsigned char)i]);
  }
  printf("\n");
#endif
}

/* set_led
 * used to turn on and off a small LED (LED2) on the microcontroller board
 * just for debug or heartbeat indication purposes
 */
void
set_led(char state)
{
  switch(state)
  {
  case 0:
    DIGITAL_IO_SetOutputLow(&led2);
    break;
  case 1:
    DIGITAL_IO_SetOutputHigh(&led2);
    break;
  default:
    break;
  }
}

void
play_tone(char type)
{
  unsigned int i;
  PWM_CCU4_Start(&pwm1);
  for (i=0; i<50; i++)
  {
    if (type==0) // falling tone
    {
      PWM_CCU4_SetFreq(&pwm1, 500+((50-i)*20));
    }
    else if (type==1) // rising tone
    {
      PWM_CCU4_SetFreq(&pwm1, 500+(i*20));
    }
    general_timer=50;
    while(general_timer);
  }
  PWM_CCU4_Stop(&pwm1);
}

/************** 8x8 LED Matrix display handling functions ************/

/* display_init
 * initializes the display chip
 */
void
display_init(void)
{
  unsigned char i;
  for (i=0; i<3; i++)
  {
    I2C_MASTER_Transmit(&i2c_bus, true, led_address, (uint8_t*)(&display_init_data[i]), 1, true);
    while(I2C_MASTER_IsTxBusy(&i2c_bus));
  }
}

/* display_write
 * sends the display ram data to the display
 */
void
display_write(void)
{
  unsigned char i;

  I2C_MASTER_SendStart(&i2c_bus, led_address, XMC_I2C_CH_CMD_WRITE);
  //while(I2C_MASTER_GetFlagStatus(&i2c_bus, XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED) == 0U);
  //I2C_MASTER_ClearFlag(&i2c_bus, XMC_I2C_CH_STATUS_FLAG_ACK_RECEIVED);

  I2C_MASTER_TransmitByte(&i2c_bus, 0x00); // select display address 0x00
  while(I2C_MASTER_IsTxBusy(&i2c_bus));

  // now write out 128 bits, of which 64 correspond to the LEDs on an 8x8 module
  for (i=0; i<8; i++)
  {
    I2C_MASTER_TransmitByte(&i2c_bus, display_ram[i] & 0xff);
    while(I2C_MASTER_IsTxBusy(&i2c_bus));
    I2C_MASTER_TransmitByte(&i2c_bus, 0); // display IC has 16x8 bits RAM but display is 8x8 bits
    while(I2C_MASTER_IsTxBusy(&i2c_bus));
  }
  I2C_MASTER_SendStop(&i2c_bus);

  display_update_timer=10;
  while(display_update_timer);

}

/************* display ram related functions ***********/

// all of these functions make modifications to the local ram representation
// of the display. Nothing is actually updated on the display, until
// display_write is called.

/* display_ram_blank
 * sets the display ram to all blank.
 * Doesn't update to the display, call display_write to do that.
 */
void
display_ram_blank(void)
{
  unsigned char i;
  for (i=0; i<8; i++)
  {
    display_ram[i]=0;
  }
}

/* plot_ram_pixel
 * sets a pixel in the display ram.
 * Doesn't update the display, call display_write to do that.
 */
void
plot_ram_pixel(int x, int y)
{
  switch(ORIENTATION) // for future use if the display orientation ever changes
  {
  case 0:
    x=8-x-1;
    break;
  default:
    break;
  }
  x = x+7; // fixes an unusual mapping in the 8x8 display matrix
  x = x%8; //

  display_ram[y] |= 1<<x; // place the pixel into the local display ram
}

/* plot_ram_rows
 * puts each row content into the local display ram. Use display_write to
 * then send it to the display module.
 */
void
plot_ram_rows(unsigned char* rows_arr)
{
  unsigned char i, j;
  display_ram_blank();
  for (i=0; i<rows; i++)
  {
    if (rows_arr[i]>0)
    {
      for (j=0; j<rows_arr[i]; j++)
      {
        plot_ram_pixel((int)i*2, (int)j);
      }
    }
  }
}

// a portion of the ASCII table as a 5x7 bitmap
// stored as row bitmaps (i.e 7 values per character)
// to make it a bit easier to map to the 8x8 LED display,
// at the expense of a bit more ROM usage than storing
// column bitmaps.
// Also characters are reflected: the rightmost part of each character is the
// LSB in this bitmap, just to map easier to the 8x8 display.
// There may be visual errors in some of these bitmaps that may need
// correction, since it was hand-coded, not automated, and not fully
// checked yet.
const unsigned char alpha_bitmap[]={
0, 0, 0, 0, 0, 0, 0,         /*   */
4, 0, 4, 4, 4, 4, 4,         /* ! */
0, 0, 0, 0, 10, 10, 10,      /* " */
10, 10, 31, 10, 31, 10, 10,  /* # */
4, 30, 5, 14, 20, 15, 4,     /* $ */
3, 19, 8, 4, 2, 25, 24,      /* % */
13, 18, 21, 8, 20, 18, 12,   /* & */
0, 0, 0, 0, 0, 4, 4,         /* ' */
2, 3, 8, 8, 8, 4, 2,         /* ( */
8, 4, 2, 2, 2, 4, 8,         /* ) */
0, 4, 21, 14, 21, 4, 0,      /* * */
0, 4, 4, 31, 4, 4, 0,        /* + */
8, 4, 12, 0, 0, 0, 0,        /* , */
0, 0, 0, 31, 0, 0, 0,        /* - */
12, 12, 0, 0, 0, 0, 0,       /* . */
0, 16, 8, 4, 2, 1, 0,        /* / */
14, 17, 17, 17, 17, 17, 14,  /* 0 */
14, 4, 4, 4, 4, 12, 4,       /* 1 */
31, 16, 8, 6, 1, 17, 14,     /* 2 */
14, 17, 1, 6, 1, 17, 14,     /* 3 */
2, 2, 31, 18, 10, 6, 2,      /* 4 */
14, 17, 1, 1, 30, 16, 31,    /* 5 */
14, 17, 17, 30, 16, 8, 6,    /* 6 */
8, 8, 8, 4, 2, 1, 31,        /* 7 */
14, 17, 17, 14, 17, 17, 14,  /* 8 */
12, 2, 1, 15, 17, 17, 14,    /* 9 */
0, 12, 12, 0, 12, 12, 0,     /* : */
8, 4, 12, 0, 12, 12, 0,      /* ; */
2, 4, 8, 16, 8, 4, 2,        /* < */
0, 0, 31, 0, 31, 0, 0,       /* = */
8, 4, 2, 1, 2, 4, 8,         /* > */
4, 0, 4, 2, 1, 17, 14,       /* ? */
14, 21, 21, 13, 1, 17, 14,   /* @ */
17, 17, 31, 17, 17, 10, 4,   /* A */
30, 9, 9, 14, 9, 9, 30,      /* B */
14, 17, 16, 16, 16, 17, 14,  /* C */
30, 9, 9, 9, 9, 9, 30,       /* D */
31, 16, 16, 30, 16, 16, 31,  /* E */
16, 16, 16, 30, 16, 16, 31,  /* F */
15, 17, 17, 19, 16, 17, 14,  /* G */
17, 17, 17, 31, 17, 17, 17,  /* H */
14, 4, 4, 4, 4, 4, 14,       /* I */
12, 18, 2, 2, 2, 2, 7,       /* J */
17, 18, 20, 24, 20, 18, 17,  /* K */
31, 16, 16, 16, 16, 16, 16,  /* L */
17, 17, 17, 21, 21, 27, 17,  /* M */
17, 17, 19, 21, 25, 17, 17,  /* N */
14, 17, 17, 17, 17, 17, 14,  /* O */
16, 16, 16, 30, 17, 17, 30 , /* P */
13, 18, 21, 17, 17, 17, 14,  /* Q */
17, 18, 20, 30, 17, 17, 30,  /* R */
14, 17, 1, 14, 16, 17, 14,   /* S */
4, 4, 4, 4, 4, 4, 31,        /* T */
14, 17, 17, 17, 17, 17, 17,  /* U */
4, 10, 17, 17, 17, 17, 17,   /* V */
10, 21, 21, 21, 17, 17, 17,  /* W */
17, 17, 10, 4, 10, 17, 17,   /* X */
4, 4, 4, 10, 17, 17, 17,     /* Y */
31, 16, 8, 4, 2, 1, 31,      /* Z */
};

/* scroll_text
 * This function will display a text message.
 * It doesn't use any C library features, so
 * it is limited. To fully scroll off a
 * message, ensure there are two spaces at the end
 * of the text array (and len increased by 2 to
 * reflect that). The variable all, if set to 1,
 * will wipe the bottom row of the display.
 * If set to zero, it will leave the final
 * stick at the bottom of the display.
 */
void
scroll_text(char* text, char len, char all)
{
  char first=1;
  char i, xmov, y, startx;
  char a, b; // two partial characters can be scrolled on the 8-wide display since each character is 5 bits wide
  unsigned int idx_a, idx_b;
  unsigned short int ab_slice; // this variable stores a bitmap of a row, for two characters
  for (i=0; i<len-1; i++)
  {
    // this algorithm revolves around reducing the problem to scrolling only
    // two characters. At the appropriate point in the animation, the for loop
    // is used to update the a and b variables to make a become the previous b, and
    // and the next character gets placed in b.
    a=text[(unsigned char)i];
    b=text[(unsigned char)(i+1)];
    idx_a=(unsigned int)(a-' '); // get an index into the alphabet bitmap
    idx_a=idx_a*7;               //

    idx_b=(unsigned int)(b-' '); // get an index into the alphabet bitmap
    idx_b=idx_b*7;               //

    if (all)
    {
      display_ram[0]=0; // bottom row is blank
    }
    if (first)
      startx=0;
    else
      startx=6;         // after the very first character, subsequent characters are placed at the correct point in the animation
    for (xmov=startx; xmov<11; xmov++) // step through to scroll two characters
    {
      for (y=0; y<7; y++)
      {
        // the left byte of ab_slice will ultimately get displayed.
        // the left character (a) is put into ab_slice so that only the leftmost part of the character will appear in the rightmost part of the display
        ab_slice=((unsigned short int)(alpha_bitmap[idx_a+y]))<<(xmov+4);
        if (xmov>5)
        {
          // the next character (b) needs to start showing on the display.
          // // the b character is butted next to a, with a space of a single bit
          ab_slice |= (((unsigned short int)((alpha_bitmap[idx_b+y])))<<(xmov-2));
        }
        // now shift it all to the right, so that the part to be displayed is in the
        // lower 8 bits
        display_ram[y+1]=(((unsigned short int)(ab_slice))>>8);
        // the 8x8 display module has weird mapping. we need to fix in software
        if (display_ram[y+1] & 0x01)
        {
          display_ram[y+1]=display_ram[y+1]>>1;
          display_ram[y+1]|=0x80;
        }
        else
        {
          display_ram[y+1]=display_ram[y+1]>>1;
        }
      }
      // display the ram
      display_write();
      display_update_timer=70; // 70msec scrolling delay
      while(display_update_timer);
    }
    first=0;
  }
}

