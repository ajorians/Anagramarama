/*
* Anagramarama - A word game.  Like anagrams?  You'll love anagramarama!
* Copyright (C) 2003  Colm Gallagher
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Contact Details: colm@coralquest.com
*  	 12 Weston Terrace, West Kilbride, KA23 9JX.  Scotland.
*/


/*
* Contributors
*
* Colm Gallagher  : Concept and initial programming
* Alan Grier      : Graphics
* THomas Plunkett : Audio
* Shard           : BEOS Port and bugfixes
* Adolfo          : Bugfix for Linux Red-Hat version 7.3
* Pat Thoyts      : C code cleanup, memory leak and cpu usage fixes
*
*
* -------------------------------------------------------------------
* version		who		changes
* -------------------------------------------------------------------
* 0.1		Colm		initial Linux & Windows revisions
*
* 0.2		Shard		Bugfix: buffer overrun in clearWord
*						function corrupted memory.  Strange
*						thing is it crashed BEOS, but not
*						Linux or Windows - guess they handle
*						memory differently or BEOS is much
*						better at detecting exceptions
*
* 0.3		Shard		added BEOS port (new makefile)
*
* 0.4		Adolfo		Bugfix: oops!  in the checkGuess
* 			            function, I tried to initialise
* 			            test[] using a variable  as if I was
* 			            using vb6 !  Have changed this to a 
* 			            static buffer and all is now well.
* 
* 0.5		Colm		Added keyboard input
*
* 0.6		Paulo		Added i18n and Portugues dict
* -------------------------------------------------------------------
*/

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* WIN32 */

#include <os.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <time.h>
//#include <string.h>
//#include <assert.h>
#include <SDL/SDL.h>
//#include "arrow.h"
//#include <SDL/SDL_mixer.h>

#include "dlb.h"
#include "linked.h"
#include "sprite.h"
#include "ag.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define MOUSE_ADJUST			(3)
#define NUMBER_ANSWER_ITEMS_PER_COLUMN	(4)
#define NUMBER_ANSWER_COLUMNS_VISIBLE	(4)
#define PROGRAM_MSG_TITLE		"Anagramarama"

/* functions from ag_code.c */
void ag(struct node **head, struct dlb_node *dlbHead, 
   const char *guess, const char *remain);
void getRandomWord(char *output, size_t length);
int nextBlank(const char *string);

enum Hotboxes { BoxSolve, BoxNew, BoxQuit, BoxShuffle, BoxEnter, BoxClear };
const char *BoxNames[] = {
   "solve", "new", "quit", "shuffle", "enter", "clear"
};
Box hotbox[6] = {
   /* BoxSolve */   { 241, 0, 31, 13 },//612, 0, 66, 30 },
   /* BoxNew */     { 272, 0, 24, 13 },//686, 0, 46, 30 },
   /* BoxQuit */    { 296, 0, 24, 13 },//742, 0, 58, 30 },
   /* BoxShuffle */ { 618, 206, 66, 16 },
   /* BoxEnter */   { 690, 254, 40, 35 },
   /* BoxClear */   { 690, 304, 40, 40 }
};

SDL_Surface *imageBackground;

/* module level variables for game control */
char shuffle[8] = SPACE_FILLED_CHARS;
char answer[8]  = SPACE_FILLED_CHARS;
char language[64];
char txt[50];
char rootWord[9];
int updateAnswers = 0;
int startNewGame = 0;
int solvePuzzle = 0;
int shuffleRemaining = 0;
int clearGuess = 0;

int mouseX = 0;
int mouseY = 0;

int answerPage = 0;

/*time_t*/ Uint32 gameStart = 0;
/*time_t*/ Uint32 gameTime = 0;
int stopTheClock = 0;

int totalScore = 0;
int score = 0;
int answersSought = 0;
int answersGot = 0;
int gotBigWord = 0;
int bigWordLen = 0;
int updateTheScore = 0;
//int gamePaused = 0;
int foundDuplicate = 0;
int quitGame = 0;
int winGame = 0;

int letterSpeed = LETTER_FAST;

/* Graphics cache */
SDL_Surface* letterBank = NULL;
SDL_Surface* smallLetterBank = NULL;
//SDL_Surface* g_Arrow = NULL;
//SDL_Surface* numberBank = NULL;
//struct sprite* clockSprite = NULL;
//struct sprite* scoreSprite = NULL;
nSDL_Font *m_pFont;

/* audio vars */
//Uint32 audio_len;
//Uint8 *audio_pos;
//struct sound {
//	char* name;
//	Mix_Chunk *audio_chunk;
//	struct sound* next;
//};
//struct sound* soundCache = NULL;

/*
* On Windows the standard IO channels are not connected to anything
* so we must use alternative methods to see error and debug output.
* On Unix we can just print the the standard error channel.
*/

#ifndef WIN32
#define Debug Error
static void Error(const char *format, ...)
{
   /*    va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   fputc('\n', stderr);
   va_end(args);*/
}
#else /* WIN32 */
static void Error(const char *format, ...)
{
   /*    va_list args;
   char buffer[1024];
   va_start(args, format);
   _vsnprintf(buffer, sizeof(buffer), format, args);
   buffer[sizeof(buffer)-1] = 0;
   MessageBoxA(NULL, buffer, NULL, MB_OK | MB_ICONERROR);*/
}
static void Debug(const char *format, ...)
{
   /*    va_list args;
   char buffer[1024];
   va_start(args, format);
   _vsnprintf(buffer, sizeof(buffer), format, args);
   buffer[sizeof(buffer)-1] = 0;
   OutputDebugStringA(buffer);*/
}
#endif /* WIN32 */

/***********************************************************
synopsis: walk the module level soundCache until the required
name is found.  when found, return the audio data
if name is not found, return NULL instead.

inputs:   name - the unique id string of the required sound

outputs:  returns a chunk of audio or NULL if not found
***********************************************************/
#if 0
static Mix_Chunk* 
   getSound(const char *name)
{
   struct sound* currentSound = soundCache;

   while (currentSound != NULL) {

      if (!strcmp(currentSound->name, name)) {
         return currentSound->audio_chunk;
      }
      currentSound = currentSound->next;
   }
   return NULL;
}
#endif

/***********************************************************
synopsis: push a sound onto the soundCache

inputs:   soundCache - pointer to the head of the soundCache
name - unique id string for the sound, this is used
to later play the sound
filename - the filename of the WAV file

outputs:  n/a
***********************************************************/
#if 0
static void
   pushSound(struct sound **soundCache, const char *name, const char *filename)
{
   struct sound* thisSound = NULL;

   thisSound = malloc(sizeof(struct sound));
   thisSound->name = malloc(sizeof(name)*(strlen(name) + 1));
   strcpy(thisSound->name, name);
   thisSound->next = *soundCache;

   /* Attempt to load a sample */
   thisSound->audio_chunk = Mix_LoadWAV(filename);

   *soundCache = thisSound;
}
#endif

/***********************************************************
synopsis: push all the game sounds onto the soundCache
linked list.  Not that soundCache is passed into
pushSound by reference, so that the head pointer
can be updated

inputs:   pointer to the soundCache

outputs:  n/a
***********************************************************/
#if 0
static void
   bufferSounds(struct sound **soundCache)
{
   pushSound(soundCache, "click-answer", "audio/click-answer.wav");
   pushSound(soundCache, "click-shuffle", "audio/click-shuffle.wav");
   pushSound(soundCache, "foundbig", "audio/foundbig.wav");
   pushSound(soundCache, "found", "audio/found.wav");
   pushSound(soundCache, "clear", "audio/clearword.wav");
   pushSound(soundCache, "duplicate", "audio/duplicate.wav");
   pushSound(soundCache, "badword", "audio/badword.wav");
   pushSound(soundCache, "shuffle", "audio/shuffle.wav");
   pushSound(soundCache, "clock-tick", "audio/clock-tick.wav");
}
#endif

/***********************************************************
synopsis: free all of the data in the audio buffer
the audio buffer is a module variable

inputs:   n/a

outputs:  n/a
***********************************************************/
#if 0
static void
   clearSoundBuffer()
{
   struct sound* currentSound = soundCache, *previousSound = NULL;

   while (currentSound!=NULL) {
      Mix_FreeChunk(currentSound->audio_chunk);
      free(currentSound->name);
      previousSound = currentSound;
      currentSound = currentSound->next;
      free(previousSound);
   }
}
#endif

//Returns a number of answer pages there are to be.  Note: answerPage is a 0-based value of which page we are on
int NumberOfAnswerPages(struct node* head)
{
   struct node* current = head;
   int numWords = 0;

   while (current != NULL){
      numWords++;
      current = current->next;
   }

   return ((numWords + NUMBER_ANSWER_ITEMS_PER_COLUMN-1) / NUMBER_ANSWER_ITEMS_PER_COLUMN);
}

//0 = Left
//1 = Right
void MoveAnswerPage(int nRotate, struct node* head)
{
   if( nRotate == 0 ) {
      if( answerPage > 0 )
         answerPage--;
   } else {
      if( (answerPage+1) < NumberOfAnswerPages(head) ) {//The +1 is answerPage is 0-based
         answerPage++;
      }
   }

   fprintf(stderr, "Answer page [%d]\n", answerPage);
   
}

/***********************************************************
synopsis: for each letter to each answer in the nodelist,
display a neat little box.  If the answer has
been found display the letter for each box.
if the answer was guessed (as opposed to set found
by solveIt), display a white background, otherwise
display a blue background.

inputs:   head - pointer to the answers linked list
screen - pointer to the SDL_Surface to update

outputs:  n/a
***********************************************************/
static void
   displayAnswerBoxes(struct node* head, SDL_Surface* screen)
{
   struct node* current = head;
   SDL_Rect outerrect, innerrect, letterBankRect;
   int i;
   int numWords = 0;
   int numTotalWords = 0;
   int acrossOffset = 25;//70;
   int numLetters = 0;
   int listLetters = 0;

   // width and height are always the same 
   outerrect.w = 16;
   outerrect.h = 16;
   outerrect.x = acrossOffset;
   outerrect.y = 150;//380;

   letterBankRect.w = 10;
   letterBankRect.h = 16;
   letterBankRect.y = 0;
   letterBankRect.x = 0; // letter is chosen by 10*letter where a is 0 

   while (current != NULL){

      // new word 
      int nPage = numTotalWords / NUMBER_ANSWER_ITEMS_PER_COLUMN;//before numTotalWords is updated
      numTotalWords++;

      if( answerPage <= nPage && (answerPage + NUMBER_ANSWER_COLUMNS_VISIBLE) > nPage ) {
         numWords++;
         numLetters = 0;

         // update the x for each letter 
         for (i=0;i<current->length;i++) {

            numLetters++;

            SDL_FillRect(screen, &outerrect,0);
            innerrect.w = outerrect.w - 1;
            innerrect.h = outerrect.h - 1;
            innerrect.x = outerrect.x + 1;
            innerrect.y = outerrect.y + 1;

            if (current->guessed){
               SDL_FillRect(screen, &innerrect,SDL_MapRGB(screen->format,255,255,255));
            }
            else{
               SDL_FillRect(screen, &innerrect,SDL_MapRGB(screen->format,217,220,255));
            }
 
            if (current->found) {
               int c = (int)(current->anagram[i] - 'a');
               //assert(c > -1);
               innerrect.x += 2;
               letterBankRect.x = 10 * c;
               SDL_SetColorKey(smallLetterBank, SDL_SRCCOLORKEY, SDL_MapRGB(smallLetterBank->format,255,0,255));
               SDL_BlitSurface(smallLetterBank, &letterBankRect, screen, &innerrect);
            }

            outerrect.x += 18;
         }

         if (numLetters > listLetters){
            listLetters = numLetters;
         }

         if (numWords == NUMBER_ANSWER_ITEMS_PER_COLUMN/*11*/){
            numWords = 0;
            acrossOffset += (listLetters * 18) + 9;
            outerrect.y = 150;//380;
            outerrect.x = acrossOffset;
         }
         else{
            outerrect.x = acrossOffset;
            outerrect.y += 19;
         }
      }

      current=current->next;
   }

   /*if( (answerPage + NUMBER_ANSWER_COLUMNS_VISIBLE) < NumberOfAnswerPages(head) ){
      SDL_Rect rectDest;
      rectDest.x = 320-16;
      rectDest.y = 150 + 41;
      rectDest.w = 16;
      rectDest.h = 16;
      SDL_BlitSurface(g_Arrow, NULL, screen, &rectDest);
   }*/
}

/***********************************************************
synopsis: update all of the answers to "found"

inputs:   head - pointer to the answers linked list

outputs:  n/a
***********************************************************/
static void
   solveIt(struct node *head)
{
   struct node* current = head;

   while (current != NULL){
      current->found = 1;
      current = current->next;
   }
}

/***********************************************************
synopsis: walk the linked list of answers checking
for our guess.  If the guess exists, mark it as
found and guessed.
if it's the longest word play the foundbig 
sound otherwise play the got word sound.
If the word has already been found, play the
duplicate sound.
If it cannot be found, play the badword sound

inputs:   answer - the string that we're checking
head - pointer to the linked list of answers

outputs:  n/a
***********************************************************/
static void
   checkGuess(char* answer, struct node* head)
{
   // check the guess against the answers 
   struct node* current = head;
   int i, len;
   int foundWord = 0;
   char test[8];

   memset(test, 0, sizeof(test));
   len = nextBlank(answer) - 1;
   if (len == -1) len = sizeof(test);
   for (i = 0; i < len; i++) {
      //assert(i < sizeof(test));
      test[i] = answer[i];
   }
#ifdef DEBUG
   Debug("check guess len:%d answer:'%s' test:'%s'", len, answer, test);
#endif

   int nAnswerNum = 0;
   while (current != NULL) {
      if (!strcmp(current->anagram, test)) {
         foundWord = 1;
         if (!current->found) {
            score += current->length;
            totalScore += current->length;
            answersGot++;
            if (len-1 == bigWordLen) {
               gotBigWord = 1;
               //Mix_PlayChannel(-1, getSound("foundbig"), 0);
            } else {
               // just a normal word 
               //Mix_PlayChannel(-1, getSound("found"),0);
            }
            if (answersSought == answersGot) {
               // getting all answers gives us the game score again!!
               totalScore += score;
               winGame = 1;
            }
            current->found = 1;
            current->guessed = 1;
            updateTheScore = 1;

            //Update the answer page if not visible
            int nAnswerPage = nAnswerNum / NUMBER_ANSWER_ITEMS_PER_COLUMN;
            if( answerPage > nAnswerPage || (nAnswerPage - NUMBER_ANSWER_COLUMNS_VISIBLE) >= answerPage ) {
               answerPage = nAnswerPage;//I guess just go to the page
               //updateAnswerPage = 1;
            }
         } else {
            foundDuplicate = 1;
            //Mix_PlayChannel(-1, getSound("duplicate"),0);
         }
         updateAnswers = 1;
         break;
      }

      current = current->next;
      nAnswerNum++;
   }

   if (!foundWord) {
      fprintf(stderr, "Word: [%s] was not found\n", answer);
      fflush(stderr);
      //Mix_PlayChannel(-1, getSound("badword"),0);
   }
}

/***********************************************************
synopsis: determine the next blank space in a string 
blanks are indicated by pound not space.
When a blank is found, move the chosen letter
from one box to the other.
i.e. If we're using the ANSWER box,
move the chosen letter from the SHUFFLE box 
to the ANSWER box and move a SPACE back to the
SHUFFLE box. and if we're using the SHUFFLE box
move the chosen letter from ANSWER to SHUFFLE
and move a SPACE into ANSWER.

inputs:   box - the ANSWER or SHUFFLE box
*index - pointer to the letter we're interested in

outputs:  retval : the coords of the next blank position
*index : pointer to the new position were interested in
***********************************************************/
int 
   nextBlankPosition(int box, int* index)
{
   int i=0;

   switch(box){
   case ANSWER:
      for(i=0;i<7;i++){
         if (answer[i]==SPACE_CHAR){
            break;
         }
      }
      answer[i] = shuffle[*index];
      shuffle[*index] = SPACE_CHAR;
      break;
   case SHUFFLE:
      for(i=0;i<7;i++){
         if (shuffle[i]==SPACE_CHAR){
            break;
         }
      }
      shuffle[i] = answer[*index];
      answer[*index] = SPACE_CHAR;
      break;
   default:
      break;

   }

   *index = i;

   return i * (GAME_LETTER_WIDTH+GAME_LETTER_SPACE)+BOX_START_X;
}

/***********************************************************
synopsis: handle the keyboard events
BACKSPACE & ESCAPE - clear letters
RETURN - check guess
SPACE - shuffle
a-z - select the first instance of that letter
in the shuffle box and move to the answer box

inputs: event - the key that has been pressed
node - the top of the answers list
letters - the letter sprites

outputs:  n/a
***********************************************************/
static void
   handleKeyboardEvent(SDL_Event *event, struct node* head,
struct sprite** letters)
{
   struct sprite* current = *letters;
   /*if (!gamePaused)*/{

      switch(event->key.keysym.sym){

      case SDLK_BACKSPACE: case SDLK_ESCAPE:
         // clear has been pressed 
         clearGuess = 1;
         break;

      case SDLK_RETURN:
         // enter has been pressed 
         checkGuess(answer, head);
         break;

      case SDLK_TAB:
         solvePuzzle = 1;
         break;

      case SDLK_PLUS:
        startNewGame = 1; 
	break;

      case SDLK_LEFTPAREN:
        fprintf(stderr, "MoveAnswerPage");
        MoveAnswerPage(0, head);
        break;

      case SDLK_RIGHTPAREN:
        fprintf(stderr, "MoveAnswerPage");
        MoveAnswerPage(1, head);
        break;

      case SDLK_LEFT:
        mouseX-=MOUSE_ADJUST;
        if( mouseX < 0 ) mouseX = 0;
        SDL_WarpMouse(mouseX, mouseY);
        break;
      case SDLK_RIGHT:
        mouseX+=MOUSE_ADJUST;
        if( mouseX >= SCREEN_WIDTH ) mouseX = SCREEN_WIDTH - 1;
SDL_WarpMouse(mouseX, mouseY);
        break;
      case SDLK_UP:
        mouseY-=MOUSE_ADJUST;
        if( mouseY < 0 ) mouseY = 0;
SDL_WarpMouse(mouseX, mouseY);
        break;
      case SDLK_DOWN:
        mouseY+=MOUSE_ADJUST;
        if( mouseY >= SCREEN_HEIGHT ) mouseY = SCREEN_HEIGHT - 1;
SDL_WarpMouse(mouseX, mouseY);
        break;

      case ' ':
         // shuffle has been pressed 
         shuffleRemaining = 1;
         //Mix_PlayChannel(-1, getSound("shuffle"),0);
         break;
      default:
		fprintf(stderr, "keyname [%s]\n", SDL_GetKeyName(event->key.keysym.sym));
         // loop round until we find the first instance of the 
         //                 * selected letter in SHUFFLE

         while (current!=NULL&&current->box!=CONTROLS){
            if (current->box == SHUFFLE){
               if (current->letter == (char)event->key.keysym.sym){
                  current->toX = nextBlankPosition(ANSWER, &current->index);
                  current->toY = ANSWER_BOX_Y;
                  current->box = ANSWER;
                  //Mix_PlayChannel(-1, getSound("click-shuffle"), 0);
                  break;
               }
            }
            current=current->next;
         }

      }
   }
}

static int IsInside(Box box, int x, int y)
{
   return ((x > box.x) && x < (box.x + box.width)
      && (y > box.y) && (y < box.y + box.height));
}


/***********************************************************
synopsis: checks where the mouse click occurred - if it's in
a defined hotspot then perform the appropriate action

Hotspot	        Action
-----------------------------------------------------
A letter		set the new x,y of the letter
and play the appropriate sound

ClearGuess		set the clearGuess flag

checkGuess		pass the current answer to the
checkGuess routine

solvePuzzle		set the solvePuzzle flag

shuffle		set the shuffle flag and
play the appropriate sound

newGame		set the newGame flag

quitGame		set the quitGame flag

inputs:  button - mouse button that has ben clicked
x, y - the x,y coords of the mouse
screen - the SDL_Surface to display the image
head - pointer to the top of the answers list
letters - pointer to the letters sprites

outputs:  n/a
***********************************************************/
static void
   clickDetect(int button, int x, int y, SDL_Surface *screen, 
struct node* head, struct sprite** letters)
{
   struct sprite* current = *letters;

   /*if (!gamePaused)*/ {

      while (current!=NULL&&current->box!=CONTROLS){
         if (x>= current->x && x<= current->x+current->w && y>= current->y && y<=current->y + current->h){
            if (current->box == SHUFFLE){
               current->toX = nextBlankPosition(ANSWER, &current->index);
               current->toY = ANSWER_BOX_Y;
               current->box = ANSWER;
               //Mix_PlayChannel(-1, getSound("click-shuffle"), 0);
            }
            else{
               current->toX = nextBlankPosition(SHUFFLE, &current->index);
               current->toY = SHUFFLE_BOX_Y;
               current->box = SHUFFLE;
               //Mix_PlayChannel(-1, getSound("click-answer"), 0);
            }

            break;
         }
         current=current->next;
      }

      if (IsInside(hotbox[BoxClear], x, y)) {
         // clear has been pressed 
         clearGuess = 1;
      }

      // check the other hotspots 
      if (IsInside(hotbox[BoxEnter], x, y)) {
         // enter has been pressed 
         checkGuess(answer, head);
      }
      if (IsInside(hotbox[BoxSolve], x, y)) {
         // solve has been pressed 
         solvePuzzle = 1;
      }

      if (IsInside(hotbox[BoxShuffle], x, y)) {
         // shuffle has been pressed 
         shuffleRemaining = 1;
         //Mix_PlayChannel(-1, getSound("shuffle"),0);
      }
   }

   if (IsInside(hotbox[BoxNew], x, y)) {
      // new has been pressed 
      startNewGame = 1;
   }

   if (IsInside(hotbox[BoxQuit], x, y)) {
      // new has been pressed 
      quitGame = 1;
   }
}

/***********************************************************
synopsis: move all letters from answer to shuffle

inputs:  letters - the letter sprites

outputs:  n/a
***********************************************************/
static int
   clearWord(struct sprite** letters)
{
   struct sprite* current = *letters;
   struct sprite* orderedLetters[7];
   int i;
   int count = 0;

   for (i = 0; i < (int)(sizeof(orderedLetters)/sizeof(orderedLetters[0])); ++i) {
      orderedLetters[i] = NULL;
   }

   // move the letters back up 
   while (current != NULL) {
      if (current->box == ANSWER) {
         count ++;
         orderedLetters[current->index] = current;
         current->toY = SHUFFLE_BOX_Y;
         current->box = SHUFFLE;
      }
      current=current->next;
   }

   for (i=0; i < 7; i++) {
      if (orderedLetters[i] != NULL)
         orderedLetters[i]->toX = nextBlankPosition(SHUFFLE, &orderedLetters[i]->index);
   }

   return count;
}

void reverse(char *str,int len) {
	int i=0;
	char ch;
	for(i=0;i<=(len-1)/2;i++) {
		ch=str[i];
		str[i]=str[len-1-i];
		str[len-1-i]=ch;
	}
}

#if 0
void itoa(int number, char str[256]) {
	//char *str=malloc(sizeof(char)*20);
	int negFlag=0,pos=0;
	if(number<0) {
		negFlag=1;
		number=-number;
	}
	while(number>0) {
		str[pos++]='0'+number%10;
		number=number/10;
	}
	if(negFlag) {
		str[pos++]='-';
	}
	str[pos]='\0';
	reverse(str,pos);
}
#endif

/***********************************************************
synopsis: display the score graphic

inputs: screen - the SDL_Surface to display the image

outputs: n/a
***********************************************************/
static void
   updateScore(SDL_Surface* screen)
{
   // we'll display the total Score, this is the game score 

   char buffer [256];
   size_t i;
   SDL_Rect fromrect, torect;//, blankRect;

   fromrect.x = 0;
   fromrect.y = 0;
   fromrect.w = SCORE_WIDTH;
   fromrect.h = SCORE_HEIGHT;

   torect.y = 0;
   torect.w = SCORE_WIDTH;
   torect.h = SCORE_HEIGHT;

   // move the totalScore into a string 
#if 1
   snprintf (buffer, sizeof (buffer), "%i", totalScore);
#else
   itoa(totalScore,buffer);
#endif

   nSDL_DrawString(screen, m_pFont, SCORE_X, SCORE_Y, buffer);
   /*for (i = 0; i < (size_t)(strlen(buffer)); i++){
      fromrect.x = SCORE_WIDTH * ((int)buffer[i]-48);
      torect.x = SCORE_WIDTH * i;
      SDL_BlitSurface(numberBank, &fromrect, scoreSprite->sprite, &torect);
   }*/
}

/***********************************************************
synopsis: displays the graphical representation of time

inputs: screen - the SDL_Surface to display the image

outputs: n/a
***********************************************************/
static void
   updateTime(SDL_Surface* screen)
{
   // the time is x seconds  minus the number of seconds of game time 
   int thisTime;
   int seconds;
   int minutes;
   int minute_units;
   int minute_tens;
   int second_units;
   int second_tens;

   SDL_Rect fromrect, torect, blankRect;

   blankRect.x = CLOCK_WIDTH * 11;
   blankRect.y = 0;
   blankRect.w = CLOCK_WIDTH;
   blankRect.h = CLOCK_HEIGHT;

   fromrect.x = 0;
   fromrect.y = 0;
   fromrect.w = CLOCK_WIDTH;
   fromrect.h = CLOCK_HEIGHT;

   torect.y = 0;
   torect.w = CLOCK_WIDTH;
   torect.h = CLOCK_HEIGHT;

   thisTime = (gameTime / 1000) - (gameStart / 1000);//(int)((time_t)AVAILABLE_TIME - gameTime);
   minutes = thisTime/60;
   seconds = thisTime-(minutes*60);
   minute_tens = minutes/10;
   minute_units = minutes-(minute_tens*10);
   second_tens = seconds/10;
   second_units = seconds-(second_tens*10);

   char buffer[6];
   buffer[0] = minute_tens + '0';
   buffer[1] = minute_units + '0';
   buffer[2] = ':';
   buffer[3] = second_tens + '0';
   buffer[4] = second_units + '0';
   buffer[5] = '\0';
   nSDL_DrawString(screen, m_pFont, CLOCK_X, CLOCK_Y, buffer);

   /*fromrect.x = CLOCK_WIDTH * minute_tens;
   torect.x = CLOCK_WIDTH * 0;
   SDL_BlitSurface(numberBank, &fromrect, clockSprite->sprite, &torect);
   fromrect.x = CLOCK_WIDTH * minute_units;
   torect.x = CLOCK_WIDTH * 1;
   SDL_BlitSurface(numberBank, &fromrect, clockSprite->sprite, &torect);
   fromrect.x = CLOCK_WIDTH * second_tens;
   torect.x = CLOCK_WIDTH * 3;
   SDL_BlitSurface(numberBank, &fromrect, clockSprite->sprite, &torect);
   fromrect.x = CLOCK_WIDTH * second_units;
   torect.x = CLOCK_WIDTH * 4;
   SDL_BlitSurface(numberBank, &fromrect, clockSprite->sprite, &torect);*/

   // tick out the last 10 seconds 
   if (thisTime<=10 && thisTime>0) {
   //Mix_PlayChannel(-1, getSound("clock-tick"), 0);
   }
}

/***********************************************************
synopsis: replace characters randomly

inputs: string to randomise (in/out)

outputs: n/a
***********************************************************/
static void
   shuffleWord(char *word)
{
   char tmp;
   int a, b, n;
   int count = (rand() % 7) + 20;
   for (n = 0; n < count; ++n) {
      a = rand() % 7;
      b = rand() % 7;
      tmp = word[a];
      word[a] = word[b];
      word[b] = tmp;
   }
}

/***********************************************************
synopsis: returns the index of a specific letter in a string

inputs: string - the string to check
letter - the letter to return

outputs: the index of the letter
***********************************************************/
static int
   whereinstr(const char *s, char c)
{
   const char *p;
   if ((p = strchr(s, c)) != NULL) {
      return (p - s);
   }
   return 0;
}

/***********************************************************
synopsis: same as shuffle word, but also tell the letter 
sprites where to move to

inputs: thisWord - the string to shuffle (in/out)
letters - the letter sprites

outputs: n/a
***********************************************************/
static void
   shuffleAvailableLetters(char *word, struct sprite **letters)
{
   struct sprite *thisLetter = *letters;
   int from, to;
   char swap, posSwap;
   char shuffleChars[8];
   char shufflePos[8];
   int i = 0;
   int numSwaps;

   for (i = 0; i < 7; i++) {
      shufflePos[i] = i + 1;
   }
   shufflePos[7] = '\0';

   strcpy(shuffleChars, word);

   numSwaps = rand()%10+20;

   for (i = 0; i < numSwaps; i++) {
      from = rand()%7;
      to = rand()%7;

      swap = shuffleChars[from];
      shuffleChars[from]=shuffleChars[to];
      shuffleChars[to]=swap;

      posSwap = shufflePos[from];
      shufflePos[from]=shufflePos[to];
      shufflePos[to] = posSwap;
   }

   while (thisLetter != NULL) {
      if (thisLetter->box == SHUFFLE) {
         thisLetter->toX = (whereinstr(shufflePos, (char)(thisLetter->index+1)) * (GAME_LETTER_WIDTH + GAME_LETTER_SPACE)) + BOX_START_X;
         thisLetter->index = whereinstr(shufflePos, (char)(thisLetter->index+1));
      }

      thisLetter = thisLetter->next;
   }

   strcpy(word, shuffleChars);
}

/***********************************************************
synopsis: build letter string into linked list of letter graphics

inputs: letters - letter sprites head node (in/out)
screen - the SDL_Surface to display the image

outputs: n/a
***********************************************************/
void
   buildLetters(struct sprite** letters, SDL_Surface* screen)
{
   struct sprite *thisLetter = NULL, *previousLetter = NULL;
   int i;
   int len;
   SDL_Rect rect, blankRect;
   Uint32 flags = SDL_SRCCOLORKEY;
   Uint8 bpp;
   Uint32 rmask, gmask, bmask, amask;
   int index = 0;

   blankRect.x = 27 * GAME_LETTER_WIDTH;
   blankRect.y = 0;
   blankRect.w = GAME_LETTER_WIDTH;
   blankRect.h = GAME_LETTER_HEIGHT;

   rect.y = 0;
   rect.w = GAME_LETTER_WIDTH;
   rect.h = GAME_LETTER_HEIGHT;

   len = strlen(shuffle);

   if(screen->flags & SDL_SWSURFACE)
      flags |= SDL_SWSURFACE;
   if(screen->flags & SDL_HWSURFACE)
      flags |= SDL_HWSURFACE;

   bpp = screen->format->BitsPerPixel;
   rmask = screen->format->Rmask;
   gmask = screen->format->Gmask;
   bmask = screen->format->Bmask;
   amask = screen->format->Amask;

   for (i=0; i < len; i++) {

      thisLetter = malloc(sizeof(struct sprite));

      // determine which letter we're wanting and load it from 
      //         * the letterbank
      if (shuffle[i] != ASCII_SPACE && shuffle[i] != SPACE_CHAR) {
         int chr = (int)(shuffle[i] - 'a');
         //assert(chr > -1);
         rect.x = chr * GAME_LETTER_WIDTH;
         thisLetter->sprite = SDL_CreateRGBSurface(flags, GAME_LETTER_WIDTH, GAME_LETTER_HEIGHT, bpp, rmask, gmask, bmask, amask);
         thisLetter->replace = SDL_CreateRGBSurface(flags, GAME_LETTER_WIDTH, GAME_LETTER_HEIGHT, bpp, rmask, gmask, bmask, amask);

         SDL_BlitSurface(letterBank, &rect, thisLetter->sprite, NULL);
         SDL_BlitSurface(letterBank, &blankRect, thisLetter->replace, NULL);
         thisLetter->x = rand() % 320;//799;//i * (GAME_LETTER_WIDTH + GAME_LETTER_SPACE) + BOX_START_X;
         thisLetter->y = rand() % 240;//599; // SHUFFLE_BOX_Y; 
         thisLetter->letter = shuffle[i];
         thisLetter->h = thisLetter->sprite->h;
         thisLetter->w = thisLetter->sprite->w;
         thisLetter->toX = i * (GAME_LETTER_WIDTH + GAME_LETTER_SPACE) + BOX_START_X;
         thisLetter->toY = SHUFFLE_BOX_Y;
         thisLetter->next = previousLetter;
         thisLetter->box = SHUFFLE;
         thisLetter->index = index++;

         previousLetter = thisLetter;

         *letters = thisLetter;

         thisLetter = NULL;
      }
      else{
         shuffle[i] = SPACE_CHAR;
         //	rect.x = 26 * GAME_LETTER_WIDTH;
      }

   }
}

/***********************************************************
synopsis: add the clock to the sprites
keep a module reference to it for quick and easy update
this sets the clock to a fixed 5:00 start

inputs: letters - letter sprites head node (in/out)
screen - the SDL_Surface to display the image

outputs: n/a
***********************************************************/
/*static void
   addClock(struct sprite** letters, SDL_Surface* screen)
{
   struct sprite *thisLetter = NULL;
   struct sprite *previousLetter = NULL;
   struct sprite *current = *letters;
   int i;
   SDL_Rect fromrect, torect, blankRect;
   Uint32 flags = SDL_SRCCOLORKEY;
   Uint8 bpp;
   Uint32 rmask, gmask, bmask, amask;
   int index = 0;

   blankRect.x = CLOCK_WIDTH * 11;
   blankRect.y = 0;
   blankRect.w = CLOCK_WIDTH;
   blankRect.h = CLOCK_HEIGHT;

   fromrect.x = 0;
   fromrect.y = 0;
   fromrect.w = CLOCK_WIDTH;
   fromrect.h = CLOCK_HEIGHT;

   torect.y = 0;
   torect.w = CLOCK_WIDTH;
   torect.h = CLOCK_HEIGHT;

   if(screen->flags & SDL_SWSURFACE)
      flags |= SDL_SWSURFACE;
   if(screen->flags & SDL_HWSURFACE)
      flags |= SDL_HWSURFACE;

   bpp = screen->format->BitsPerPixel;
   rmask = screen->format->Rmask;
   gmask = screen->format->Gmask;
   bmask = screen->format->Bmask;
   amask = screen->format->Amask;

   // add the clock onto the end - so we don't slow letter processing any
   while (current != NULL){
      previousLetter = current;
      current =current->next;
   }

   thisLetter=malloc(sizeof(struct sprite));

   thisLetter->sprite = SDL_CreateRGBSurface(flags, CLOCK_WIDTH*5, CLOCK_HEIGHT, bpp, rmask, gmask, bmask, amask);
   thisLetter->replace = SDL_CreateRGBSurface(flags, CLOCK_WIDTH*5, CLOCK_HEIGHT, bpp, rmask, gmask, bmask, amask);

   // initialise with 05:00
   for (i=0;i<5;i++){

      //	printf("i:%i\n", CLOCK_WIDTH * i); 

      torect.x = CLOCK_WIDTH * i;
      switch(i){

      case 1:
         fromrect.x = 5 * CLOCK_WIDTH;
         break;
      case 2: 
         fromrect.x = CLOCK_WIDTH * 10; // the colon 
         break;
      case 0:
      case 3:
      case 4:
         fromrect.x = 0;
         break;
      default:
         break;
      }

      SDL_BlitSurface(numberBank, &fromrect, thisLetter->sprite, &torect);

      SDL_BlitSurface(numberBank, &blankRect, thisLetter->replace, &torect);
   }

   thisLetter->x = CLOCK_X;
   thisLetter->y = CLOCK_Y;
   thisLetter->h = thisLetter->sprite->h;
   thisLetter->w = thisLetter->sprite->w;
   thisLetter->toX = thisLetter->x;
   thisLetter->toY = thisLetter->y;
   thisLetter->next = NULL;
   thisLetter->box = CONTROLS;
   thisLetter->index = index++;

   previousLetter->next = thisLetter;
   clockSprite = thisLetter;
}*/

/***********************************************************
synopsis: add the Score to the sprites
keep a module reference to it for quick and easy update

inputs: letters - letter sprites head node (in/out)
screen - the SDL_Surface to display the image

outputs: n/a
***********************************************************/
/*static void
   addScore(struct sprite** letters, SDL_Surface* screen)
{
   struct sprite *thisLetter = NULL;
   struct sprite *previousLetter = NULL;
   struct sprite *current = *letters;
   SDL_Rect fromrect, torect, blankRect;
   Uint32 flags = SDL_SRCCOLORKEY;
   Uint8 bpp;
   Uint32 rmask, gmask, bmask, amask;
   int index = 0;
   int i;

   blankRect.x = SCORE_WIDTH * 11;
   blankRect.y = 0;
   blankRect.w = SCORE_WIDTH;
   blankRect.h = SCORE_HEIGHT;

   fromrect.x = 0;
   fromrect.y = 0;
   fromrect.w = SCORE_WIDTH;
   fromrect.h = SCORE_HEIGHT;

   torect.y = 0;
   torect.w = SCORE_WIDTH;
   torect.h = SCORE_HEIGHT;

   if(screen->flags & SDL_SWSURFACE)
      flags |= SDL_SWSURFACE;
   if(screen->flags & SDL_HWSURFACE)
      flags |= SDL_HWSURFACE;

   bpp = screen->format->BitsPerPixel;
   rmask = screen->format->Rmask;
   gmask = screen->format->Gmask;
   bmask = screen->format->Bmask;
   amask = screen->format->Amask;

   // add the score onto the end - so we don't slow letter processing any 
   while (current != NULL){
      previousLetter = current;
      current =current->next;
   }

   // previousLetter = clockSprite;

   thisLetter = malloc(sizeof(struct sprite));

   thisLetter->sprite = SDL_CreateRGBSurface(flags, SCORE_WIDTH*5, SCORE_HEIGHT, bpp, rmask, gmask, bmask, amask);
   thisLetter->replace = SDL_CreateRGBSurface(flags, SCORE_WIDTH*5, SCORE_HEIGHT, bpp, rmask, gmask, bmask, amask);

   for (i=0;i<5;i++){
      if (i==0)
         fromrect.x = 0;
      else
         fromrect.x = SCORE_WIDTH*11;
      torect.x = i * SCORE_WIDTH;
      SDL_BlitSurface(numberBank, &fromrect, thisLetter->sprite, &torect);
      SDL_BlitSurface(numberBank, &blankRect, thisLetter->replace, &torect);
   }

   thisLetter->x = SCORE_X;
   thisLetter->y = SCORE_Y;
   thisLetter->h = thisLetter->sprite->h;
   thisLetter->w = thisLetter->sprite->w;
   thisLetter->toX = thisLetter->x;
   thisLetter->toY = thisLetter->y;
   thisLetter->next = NULL;
   thisLetter->box = CONTROLS;
   thisLetter->index = index++;

   previousLetter->next = thisLetter;
   scoreSprite = thisLetter;
}*/

/***********************************************************
synopsis: do all of the initialisation for a new game:
build the screen
get a random word and generate anagrams
(must get less than 66 anagrams to display on screen)
initialise all the game control flags

inputs: head - first node in the answers list (in/out)
dblHead - first node in the dictionary list
screen - the SDL_Surface to display the image
letters - first node in the letter sprites (in/out)

outputs: n/a
***********************************************************/
static void
   newGame(struct node** head, struct dlb_node* dlbHead, 
   SDL_Surface* screen, struct sprite** letters)
{
   char guess[9];
   char remain[9];
   int happy = 0;
   // we don't want any more than ones with 66 answers 
   // - that's all we can show... 
   int i;

   destroyLetters(letters);

   while (!happy) {
      char buffer[9];
      getRandomWord(buffer, sizeof(buffer));

      strcpy(guess,"");
      strcpy(rootWord, buffer);
      bigWordLen = strlen(rootWord)-1;
      strcpy(remain, rootWord);

      rootWord[bigWordLen] = '\0';
      //fprintf(stderr, "Root WORD: [%s]\n", rootWord);

      // destroy answers list 
      destroyAnswers(head);

      // generate anagrams from random word 
      ag(head, dlbHead, guess, remain);
      answersSought = Length(*head);
      //fprintf(stderr, "answersSought is %d\n", answersSought);
      happy = ((answersSought <= 77) && (answersSought >= 6));

   }
   // now we have a good set of words - sort them alphabetically 
   sort(head);

   for (i = bigWordLen; i < 7; i++){
      remain[i] = SPACE_CHAR;
   }
   remain[7] = '\0';
   remain[bigWordLen]='\0';
   shuffleWord(remain);
   strcpy(shuffle, remain);
   strcpy(answer, SPACE_FILLED_STRING);

   // build up the letter sprites 
   //assert(*letters == NULL && screen != NULL);
   buildLetters(letters, screen);

   //addClock(letters, screen);
   //addScore(letters, screen);

   gotBigWord = 0;
   score = 0;
   updateTheScore = 1;
   //gamePaused = 0;
   winGame = 0;
   answersGot = 0;

   gameTime = gameStart = SDL_GetTicks();//time(0);
   //gameTime = 0;
   stopTheClock = 0;
}

static Uint32
   TimerCallback(Uint32 interval, void *param)
{
   SDL_UserEvent evt;
   evt.type = SDL_USEREVENT;
   evt.code = 0;
   evt.data1 = 0;
   evt.data2 = 0;
   SDL_PushEvent((SDL_Event *)&evt);
   return 0;
}

void UpdateDisplay(SDL_Surface* screen, struct node **head, struct sprite **letters)
{
   // Blit onto the screen surface.
   SDL_SetColorKey(imageBackground, SDL_SRCCOLORKEY, SDL_MapRGB(imageBackground->format,255,0,255));
   SDL_BlitSurface(imageBackground, NULL, screen, NULL);

   updateTime(screen);
   updateScore(screen);

   displayAnswerBoxes(*head, screen);

   //TODO: Add move left/right graphic! :)

   moveSprites(&screen, letters, 50);
}

static void
   gameLoop(struct node **head, struct dlb_node *dlbHead, 
   SDL_Surface *screen, struct sprite **letters)
{
   int done=0;
   SDL_Event event;
   time_t timeNow;
   //SDL_TimerID timer;
   int timer_delay = 20;

   //timer = SDL_AddTimer(timer_delay, TimerCallback, NULL);
   // main game loop 
   while (!done) {

      //Update screen
      UpdateDisplay(screen, head, letters);

      if (startNewGame) {
         // move letters back down again 
         if (!gotBigWord){
            totalScore = 0;
         }
	 answerPage = 0;
         newGame(head, dlbHead, screen, letters);

         startNewGame = 0;
      }
      
      while( SDL_PollEvent(&event) )
      {
         if (event.type == SDL_MOUSEBUTTONDOWN) {
            clickDetect(event.button.button, event.button.x,
               event.button.y, screen, *head, letters);
         } else if (event.type == SDL_MOUSEMOTION) {
             fprintf(stderr, "MouseMotion!\n");
         } else if (event.type == SDL_KEYUP) {
            handleKeyboardEvent(&event, *head, letters);
         } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE ) {
            done = 1;
            break;
         } else if (event.type == SDL_QUIT) {
            done = 1;
            break;
         }
      }
      
      if (winGame) {
         stopTheClock = 1;
         solvePuzzle = 1;
      }

      if( !stopTheClock ) {
         gameTime = SDL_GetTicks();
      }
      /*if ((gameTime < AVAILABLE_TIME) && !stopTheClock) {
         //TODO://			timeNow = time(0) - gameStart;
         //if (timeNow != gameTime){
        // 	gameTime = timeNow;
         //	updateTime(screen);
         //}
      } else {
         if (!stopTheClock){
            stopTheClock = 1;
            solvePuzzle = 1;
         }
      }*/

      // check messages 
      if (solvePuzzle) {
         // walk the list, setting everything to found 
         solveIt(*head);
         clearWord(letters);
         strcpy(shuffle, SPACE_FILLED_STRING);
         strcpy(answer, rootWord);
         //displayLetters(screen);
         //gamePaused = 1;
         if (!stopTheClock){
            stopTheClock = 1;
         }

         solvePuzzle = 0;
      }

      if (updateAnswers){
         // move letters back down again 
         clearWord(letters);
         // displayLetters(screen);

         updateAnswers = 0;
      }

      if (updateTheScore) {
         updateScore(screen);
         updateTheScore = 0;
      }

      if (shuffleRemaining) {
         // shuffle up the shuffle box 
         char shuffler[8];
         strcpy(shuffler, shuffle);
         shuffleAvailableLetters(shuffler, letters);
         strcpy(shuffle, shuffler);
         shuffleRemaining = 0;
      }

      if (clearGuess) {
         // clear the guess; 
         if (clearWord(letters) > 0) {
            //Mix_PlayChannel(-1, getSound("clear"),0);
         }
         clearGuess = 0;
      }

      if (quitGame) {
         done = 1;
      }

   }
}

static int
   is_valid_locale(const char *path)
{
   FILE *fp = NULL;
   char buffer[260];
   strcpy(buffer, path);
   if (buffer[strlen(buffer)-1] != '/')
      strcat(buffer, "/");
   strcat(buffer, "wordlist.txt.tns");
   if ((fp = fopen(buffer, "r")) != NULL)
      fclose(fp);
   //Debug("testing %s: %s", buffer, (fp == NULL)?"failed":"present");
   return (fp != NULL);
}

/*
* parse a config line eg: solve = 555 30 76 20
*/
static int
   configBox(Box *pbox, const char *line)
{
   int x, y, w, h;
   const char *p = strchr(line, '=');
   if (p && sscanf(p+1, "%d %d %d %d", &x, &y, &w, &h) == 4) {
      pbox->x = x;
      pbox->y = y;
      pbox->width = w;
      pbox->height = h;
      return 1;
   }
   return 0;
}

/*
* read any locale-specific configuration information from an ini
* ini file. This can reconfigure the positions of the boxes to account
* for different word sizes or alternative background layouts
*/
static void
   loadConfig(const char *path)
{
   FILE *fp = NULL;
   char line[80], *p;
   if ((fp = fopen(path, "r")) != NULL) {
      //Debug("loading configuration from %s", path);
      while (!feof(fp)) {
         if ((p = fgets(line, sizeof(line), fp)) != NULL) {
            int i;
            while (*p && isspace(*p))
               ++p;
            if (*p == 0 || *p == ';')
               continue;

            for (i = 0; i < (int)(sizeof(BoxNames)/sizeof(BoxNames[0])); ++i) {
               if (strncmp(BoxNames[i], p, strlen(BoxNames[i])) == 0) {
                  configBox(&hotbox[i], p);
                  break;
               }
            }
         }
      }
      fclose(fp);
   }
}

/*
* Get the current language string from either the environment or
* the command line. This is used to location the wordlist and
* other localized resources.
*
* Sets the global variable 'language'
*/

static void
   init_locale(int argc, char *argv[])
{
   char *lang = NULL, *p = NULL;

   strcpy(language,"i18n/");
   if (argc == 2) {
      strcat(language, argv[1]);
      if (is_valid_locale(language))
         return;
   }

   lang = getenv("LANG");
   if (lang != NULL) {
      strcpy(language,"i18n/");
      strcat(language, lang);
      if (is_valid_locale(language))
         return;
      while ((p = strrchr(language, '.')) != NULL) {
         *p = 0;
         if (is_valid_locale(language))
            return;
      }
      if ((p = strrchr(language, '_')) != NULL) {
         *p = 0;
         if (is_valid_locale(language))
            return;
      }
   }

   // last resort - use the english locale 
   strcpy(language, DEFAULT_LOCALE_PATH);
}

/***********************************************************
synopsis: initialise graphics and sound, build the dictionary
cache the images and start the game loop
when the game is done tidy up

inputs: argc - argument count
argv - arguments

outputs: retval  0 = success   1 = failure
***********************************************************/

#define SCREEN_BIT_DEPTH        (16)
#define SCREEN_VIDEO_MODE (SDL_SWSURFACE|SDL_FULLSCREEN|SDL_HWPALETTE)

int
   main(int argc, char *argv[])
{
   struct node* head = NULL;
   struct dlb_node* dlbHead = NULL;
   SDL_Surface *screen;
   struct sprite* letters = NULL;

   // identify the resource locale 
   //init_locale(argc, argv);
   //if (language[strlen(language)-1] != '/')
      //strcat(language, "/");
   fprintf(stderr, "argv[0] is [%s]\n", argv[0]);
   char* pstr = argv[0];
   int nLastSlash = strlen(pstr) - 1;
   while(nLastSlash >= 0 ){
      if( *(pstr + nLastSlash) == '/' )
         break;//This had better break somewhere!
      nLastSlash--;
   }
   fprintf(stderr, "found\n");

   int n;
   for(n=0; n<=nLastSlash; n++)
      language[n] = *(pstr + n);
   fprintf(stderr, "language: [%s]\n", language);

   // create dictionary 
   strcpy(txt, language);
   if (!dlb_create(&dlbHead, strcat(txt, "wordlist.txt.tns"))) {
      show_msgbox(PROGRAM_MSG_TITLE, 
                  "Unable to create the dictionary\n"
                  "This is probably because you did not\n"
                  "copy over wordlist.txt.tns file.\n"
                  "This program does not run with that file"
      );
      //Error("failed to open word list file");
      exit(1);
   }

   if (SDL_Init(SDL_INIT_VIDEO/*|SDL_INIT_TIMER*/) < 0){
      show_msgbox(PROGRAM_MSG_TITLE,
                  "Problems starting SDL\n"
                  "This program will exit");
      //Error("Unable to init SDL: %s", SDL_GetError());
      exit(1);
   }

   screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BIT_DEPTH, SCREEN_VIDEO_MODE);
   if (screen == NULL)
   {
      show_msgbox(PROGRAM_MSG_TITLE,
                  "Unable to set video mode\n"
                  "This program will exit");
      //Error("Unable to set 800x600 video: %s", SDL_GetError());
      exit(1);
   }

   /* Hide the cursor */
   SDL_ShowCursor(SDL_DISABLE);

   //SDL_WM_SetCaption("Anagramarama", "ANAGRAMARAMA");
   //fprintf(stderr, "txt: %s\n", txt);
   //fprintf(stderr, "argv[0]: %s\n", argv[0]);

   // cache in-game graphics 
   strcpy(txt, language);
   letterBank = SDL_LoadBMP(strcat(txt,"letterBank.bmp.tns"));
   strcpy(txt, language);
   smallLetterBank = SDL_LoadBMP(strcat(txt,"smallLetterBank.bmp.tns"));
#if 0
   m_pFont = nSDL_LoadFont(NSDL_FONT_VGA, 0, NSDL_FONTCFG_DEFAULT);
#else
   m_pFont = nSDL_LoadFont(NSDL_FONT_VGA, 0, 0, 0);
#endif
   //g_Arrow = nSDL_LoadImage(arrow);
   //SDL_SetColorKey(g_Arrow, SDL_SRCCOLORKEY, SDL_MapRGB(g_Arrow->format, 255, 255, 255));
   //strcpy(txt, language);
   //numberBank = SDL_LoadBMP(strcat(txt,"numberBank.bmp.tns"));
   // load locale specific configuration (really only works if you have a mouse)
   //strcpy(txt, language);
   //strcat(txt, "config.ini.tns");
   //loadConfig(txt);
   strcpy(txt, language);
   imageBackground = SDL_LoadBMP(strcat(txt,"background.bmp.tns"));

   newGame(&head, dlbHead, screen, &letters);

   gameLoop(&head, dlbHead, screen, &letters);

   // tidy up and exit 
   dlb_free(dlbHead);
   destroyLetters(&letters);
   destroyAnswers(&head);
   SDL_FreeSurface(screen);
   SDL_FreeSurface(letterBank);
   SDL_FreeSurface(smallLetterBank);
   //SDL_FreeSurface(numberBank);
   nSDL_FreeFont(m_pFont);
   //SDL_FreeSurface(g_Arrow);
   SDL_FreeSurface(imageBackground);
   SDL_Quit(); 
   return 0;
}

/*
* Local variables:
* mode: c
* indent-tabs-mode: t
* tab-width: 4
* End:
*/
