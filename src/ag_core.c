/*
 * This file contains all the anagram handling code so that the
 * validity of the anagram management can be tested independently of
 * the SDL graphical management code.
 */

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <stdlib.h>
//#include <stdio.h>
//#include <string.h>
//#include <assert.h>
#include <os.h>
#include "dlb.h"
#include "linked.h"
#include "ag.h"

extern char txt[50];
extern char language[64];

#define assert(x) do { /*if( !(x) ) Debug(#x);*/ } while(0)

/***********************************************************
synopsis: determine the next blank space in a string 
	  blanks are indicated by pound not space
	  
inputs:   pointer the string to check

outputs:  returns position of next blank (1 is first character)
	  or 0 if no blanks found
***********************************************************/
int
nextBlank(const char *string)
{
    //fprintf(stderr, "Looking for space in [%s]\n", string);
    const char *p = strchr(string, SPACE_CHAR);
    if (p)
        return 1 + (p - string);
    return 0;
}


/***********************************************************
synopsis: shift a string of characters 1 character to the left
          truncating the leftmost character

inputs:   pointer to string to shift

outputs:  pointer to the shifted string
***********************************************************/
static char *
shiftLeftKill(const char *str)
{
    char *d = malloc(strlen(str));
    if( d == NULL) return NULL;//No memory
    strcpy(d,str+1);
    return d;
    //return strdup(str + 1);
}

/***********************************************************
synopsis: shift a string of characters 1 character to the left
	  move the first character to the end of the string
	  so it wraps around

inputs:   pointer to string to shift

outputs:  pointer to the shifted string
***********************************************************/
static char *
shiftLeft(char *string)
{
    char c = *string;
    char *p = string, *q = string+1;
    for (; p && *p && q && *q; ) {
        *p++ = *q++;
    }
    *p = c;
    return string;
}

/***********************************************************
synopsis: Generate all possible combinations of the root word
	  the initial letter is fixed, so to work out all
	  anagrams of a word, prefix with space.

inputs:   head - pointer to the answers list
	  dlbHead - pointer to the dictionary
	  guess - pointer to the current guess
	  remain - pointer to the remaining letters

outputs:  all parameters are in/out
***********************************************************/
void
ag(struct node** head, struct dlb_node* dlbHead, 
   const char* guess, const char* remain)
{
    char*  newGuess;
    char*  newRemain;
    int    totalLen=0, guessLen=0, remainLen=0;
    
	/* allocate space for our working variables */
	guessLen = strlen(guess);
	remainLen = strlen(remain);
	totalLen = guessLen + remainLen;
    
	newGuess = malloc(sizeof(char) * (totalLen+1));
	newRemain = malloc(sizeof(char) * (totalLen+1));
    
	/* move last remaining letter to end of guess */
	strcpy(newGuess, guess);
	strcpy(newRemain, remain);
	newGuess[guessLen] = newRemain[remainLen-1];
	newGuess[guessLen+1] = '\0';
	newRemain[remainLen-1] = '\0';

	if (strlen(newGuess) > 3){
//	fprintf(stderr, "about to do a shiftLeftKill; newGuess is [%s]\n", newGuess);
        char *str = shiftLeftKill(newGuess);
//        fprintf(stderr, "did a shiftLeftKill; str is [%s]\n", str);
           if (dlb_lookup(dlbHead, str)) {
               //fprintf(stderr, "Not found so adding [%s]\n", str);
               push(head, str);
           }
           else
           {
              //fprintf(stderr, "found so not adding [%s]\n", str);
           }
        free(str);
	}
	
        if (strlen(newRemain)) {
        size_t i;
		ag(head, dlbHead, newGuess, newRemain);

		for (i = totalLen-1; i > 0; i--) {
			if ((size_t)(strlen(newRemain)) > i){
                newRemain = shiftLeft(newRemain);
                ag(head, dlbHead, newGuess, newRemain);
			}
		}
	}
	/* free the space */
	free(newGuess);
	free(newRemain);
}

/* checkGuess - needs to return a sound to play */

/***********************************************************
synopsis: spin the word file to a random location and then
	  loop until a 7 or 8 letter word is found
	  this is quite a weak way to get a random word
	  considering we've got a nice dbl Dictionary to
	  hand - but it works for now.

inputs: n/a

outputs: a random word
***********************************************************/
int ReadWord2(FILE* fp, char buffer[64])
{
   int nRet = fgets(buffer, 64, fp) != NULL;
   //fprintf(stderr, "Length rand word: %d\n", strlen(buffer));
   buffer[strlen(buffer)-1] = '\0';//fgets keeps newlines
   //fprintf(stderr, "Randword: [%s]\n", buffer);
   return nRet;
}
void
getRandomWord(char *output, size_t length)
{
//fprintf(stderr, "getRandomWord\n");

    FILE *fp;
    struct stat statbuf;
    char buffer[64];
    
	strcpy(txt,language);
    strcat(txt,"wordlist.txt.tns");
	fp = fopen(txt,"r");

    if( fp == NULL ) {
      fprintf(stderr, "Didn't open file\n");
    }

    stat(txt, &statbuf);
    fprintf(stderr, "Size of file is %d bytes\n", (int)(long long)statbuf.st_size);
    fseek(fp, (rand() % statbuf.st_size), SEEK_SET);

    if (ReadWord2(fp, buffer) == 0)
        fseek(fp, 0, SEEK_SET);

	// ok random location reached 
	while (strlen(buffer) != 7) {
		//fprintf(stderr, "%s", buffer);
        if (ReadWord2(fp, buffer) == 0) {
            // go back to the start of the file 
            int i;
            fseek(fp, 0L, SEEK_SET);
            i = ReadWord2(fp, buffer);// fscanf(fp, "%63s", buffer);
		}
	}
	
	fclose(fp);

	// add in our space character 
    strcpy(output, buffer);
    strcat(output, " ");
	return;
}

