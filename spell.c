/*
 *
 * ProEdit MP Multi-platform Programming Editor
 * Designed/Developed/Produced by Adrian Michaud
 *
 * MIT License
 *
 * Copyright (c) 2019 Adrian Michaud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "osdep.h"   /* Platform dependent interface */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "proedit.h"
#include "speller/dictionary.h"

#define MAX_WORD_LEN       32
#define MIN_WORD_LEN       2
#define MAX_COMPRESSED_LEN 64

#define SPELL_IGNORE   3
#define SPELL_FIXED    2
#define SPELL_CONTINUE 1
#define SPELL_EXIT     0

#define MAX_WEIGHTS    64
#define MAX_WORDS      16

#define MAX_CACHED_SCANS 128

#define HASH_TABLE_SIZE 0x8000

typedef struct wordChoices_t
{
	char*words[MAX_WORDS];
	int fullscan;
	int total;
}WORD_LIST;

typedef struct scanCache_t
{
	char*word;
	WORD_LIST choices;
}SPELL_SCAN_CACHE;

static int scanCacheIndex;
static SPELL_SCAN_CACHE*scanCache[MAX_CACHED_SCANS];

typedef struct savedWords_t
{
	char*word;
	char*fixed;
	struct savedWords_t*next;
}SAVED_WORDS;

#define WORD_LIST_IGNORE 0
#define WORD_LIST_FIXED  1
#define WORD_LIST_ADDED  2
#define NUM_WORD_LISTS   3

static SAVED_WORDS*wordLists[NUM_WORD_LISTS];

static int WordText(char ch);
static int SpellCheckLine(EDIT_FILE*file, EDIT_LINE*line, int line_num, int
    selOffset);
static void FixCase(char*misspelled, int len, char*correct);
static int FixSpelling(EDIT_FILE*file, EDIT_LINE*clipLine, char*incorrect, int
    line, int column, int len, WORD_LIST*choices, int selOffset);
static int CheckSpelling(EDIT_FILE*file, EDIT_LINE*clipLine, int line, int
    offset, char*word, int len, int selOffset);
static char*CheckWordList(int list, char*word);
static void AddWordList(int list, char*word, char*fixed);

#ifdef DEBUG_MEMORY
static void FreeWordList(int list);
#endif

static int ListSpellings(WORD_LIST*choices, char*result);
static int WeightWord(char*misspelled, int misspelledLen, char*test,
    int testLen);
static int AddWordChoice(char*word, WORD_LIST*choices);
static void PreviousFix(char*word, WORD_LIST*choices);
static int LookupWord(char*word, int len);
static void SwapChars(char*word, int len, WORD_LIST*choices);
static void SwapGroups(char*word, int len, WORD_LIST*choices);
static void BadChar(char*word, int len, WORD_LIST*choices);
static void MissingChar(char*word, int len, WORD_LIST*choices);
static void ExtraChar(char*word, int len, WORD_LIST*choices);
static void DoubleWord(char*word, int len, WORD_LIST*choices);
static void FullWordScan(char*word, int, WORD_LIST*choices);
static int CheckScanCache(char*word, WORD_LIST*choices);
static void AddScanCache(char*word, WORD_LIST*choices);
static void FlushScanCache(int index);
static void HashDictionary(void);
static void HashWord(char*word, int len);
static void AddNewWord(char*word);
static void AddUserWords(void);

static char insertChars[] = {"esianjrtqolcdugmphbyfvkwzx"};

typedef struct dictionaryList
{
	char*word;
	struct dictionaryList*next;
}SPELL_WORD;

static SPELL_WORD*dictionary_hash[HASH_TABLE_SIZE];

static int hashedDictionary;

typedef struct swapGroups_t
{
	char*find;
	int findLen;
	char*replace;
	int replaceLen;
}SPELL_SWAP_GROUPS;

#define SW(a,b) (a),sizeof(a)-1,(b),sizeof(b)-1

static SPELL_SWAP_GROUPS swapGroups[] = {
	{SW("a", "ei")}, {SW("ei", "a")}, {SW("a", "ey")},
	{SW("ey", "a")}, {SW("ai", "ie")}, {SW("ie", "ai")},
	{SW("are", "air")}, {SW("are", "ear")}, {SW("are", "eir")},
	{SW("air", "are")}, {SW("air", "ere")}, {SW("ere", "air")},
	{SW("ere", "ear")}, {SW("ere", "eir")}, {SW("ear", "are")},
	{SW("ear", "air")}, {SW("ear", "ere")}, {SW("eir", "are")},
	{SW("eir", "ere")}, {SW("ch", "te")}, {SW("te", "ch")},
	{SW("ch", "ti")}, {SW("ti", "ch")}, {SW("ch", "tu")},
	{SW("tu", "ch")}, {SW("ch", "s")}, {SW("s", "ch")},
	{SW("ch", "k")}, {SW("k", "ch")}, {SW("f", "ph")},
	{SW("ph", "f")}, {SW("gh", "f")}, {SW("f", "gh")},
	{SW("i", "igh")}, {SW("igh", "i")}, {SW("i", "uy")},
	{SW("uy", "i")}, {SW("i", "ee")}, {SW("ee", "i")},
	{SW("j", "di")}, {SW("di", "j")}, {SW("j", "gg")},
	{SW("gg", "j")}, {SW("j", "ge")}, {SW("ge", "j")},
	{SW("s", "ti")}, {SW("ti", "s")}, {SW("s", "ci")},
	{SW("ci", "s")}, {SW("k", "cc")}, {SW("cc", "k")},
	{SW("k", "qu")}, {SW("qu", "k")}, {SW("kw", "qu")},
	{SW("o", "eau")}, {SW("eau", "o")}, {SW("o", "ew")},
	{SW("ew", "o")}, {SW("oo", "ew")}, {SW("ew", "oo")},
	{SW("ew", "ui")}, {SW("ui", "ew")}, {SW("oo", "ui")},
	{SW("ui", "oo")}, {SW("ew", "u")}, {SW("u", "ew")},
	{SW("oo", "u")}, {SW("u", "oo")}, {SW("u", "oe")},
	{SW("oe", "u")}, {SW("u", "ieu")}, {SW("ieu", "u")},
	{SW("ue", "ew")}, {SW("ew", "ue")}, {SW("uff", "ough")},
	{SW("oo", "ieu")}, {SW("ieu", "oo")}, {SW("ier", "ear")},
	{SW("ear", "ier")}, {SW("ear", "air")}, {SW("air", "ear")},
	{SW("w", "qu")}, {SW("qu", "w")}, {SW("z", "ss")},
	{SW("ss", "z")}, {SW("shun", "tion")},
	{SW("shun", "sion")}, {SW("shun", "cion")}
};

#define NUM_SWAP_GROUPS ((sizeof(swapGroups)/sizeof(SPELL_SWAP_GROUPS)))


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HashDictionary(void)
{
	char prev[MAX_COMPRESSED_LEN];
	char decompressed[MAX_COMPRESSED_LEN];
	char affix[MAX_COMPRESSED_LEN];
	char*ptr;
	int i, newLen, baseLen;
	int numNibbles = 0;
	char*nibbles;
	int index = 0;

	nibbles = OS_Malloc(sizeof(dictionary_words)*2);

	for (i = 0; i < (int)sizeof(dictionary_words); i++) {
		nibbles[numNibbles] = (dictionary_words[i] >> 4);
		nibbles[numNibbles + 1] = (dictionary_words[i]&0x0f);
		numNibbles += 2;
	}

	while (index < numNibbles - 1) {
		newLen = 0;

		if (nibbles[index]) {
			memcpy(&decompressed[newLen], prev, nibbles[index]);
			newLen += nibbles[index];
		}
		index++;

		while (nibbles[index] != 0x0f) {
			if (nibbles[index] == 0x0d) {
				index++;
				decompressed[newLen++] =
				compressedL2[(unsigned char)nibbles[index++]];
			} else
				if (nibbles[index] == 0x0e) {
					index++;
					decompressed[newLen++] =
					compressedL3[(unsigned char)nibbles[index++]];
				} else {
					decompressed[newLen++] =
					compressedL1[(unsigned char)nibbles[index++]];
				}
		}

		index++;

		decompressed[newLen] = 0;
		memcpy(prev, decompressed, newLen + 1);

		ptr = strstr(decompressed, "+");

		if (ptr)
			HashWord(decompressed, ptr - decompressed);
		else
			ptr = strstr(decompressed, "/");

		if (ptr) {
			baseLen = ptr - decompressed;

			strcpy(affix, ptr + 1);

			for (i = 0; i < (newLen - baseLen) - 1; i++) {
				strcpy(ptr, suffixes[affix[i]-'a']);
				HashWord(decompressed, baseLen + strlen(ptr));
			}
		} else
			HashWord(decompressed, newLen);
	}
	OS_Free(nibbles);

	/* Add user words. */
	AddUserWords();
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HashWord(char*word, int len)
{
	unsigned int hash = 5137;
	int i;
	SPELL_WORD*newWord;

	newWord = OS_Malloc(sizeof(SPELL_WORD));
	newWord->word = OS_Malloc(len + 2);
	newWord->word[0] = (char)len;
	memcpy(newWord->word + 1, word, len);
	newWord->word[1 + len] = 0;

	for (i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + word[i];

	newWord->next = dictionary_hash[hash%HASH_TABLE_SIZE];

	dictionary_hash[hash%HASH_TABLE_SIZE] = newWord;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int SpellCheck(EDIT_FILE*file, EDIT_CLIPBOARD*clipboard)
{
	EDIT_LINE*walk;
	int line_num;
	int selOffset;


	if (!hashedDictionary) {
		CenterBottomBar(0, "[+] One Moment, Preparing Dictionary [+]");
		HashDictionary();
		hashedDictionary = 1;
	}

	if (file->copyStatus&COPY_BLOCK)
		selOffset = MIN(file->copyFrom.offset, file->copyTo.offset);
	else
		selOffset = 0;

	line_num = MIN(file->copyFrom.line, file->copyTo.line);

	walk = clipboard->lines;

	CenterBottomBar(0, "[+] One Moment, Spell Checking [+]");

	while (walk) {
		if (walk->len) {
			if (SpellCheckLine(file, walk, line_num, selOffset) == SPELL_EXIT) {
				CenterBottomBar(1, "[+] Spell Check Aborted [+]");
				return (0);
			}
		}

		line_num++;

		walk = walk->next;
	}

	CenterBottomBar(1, "[+] Spell Check Complete [+]");

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SpellCheckLine(EDIT_FILE*file, EDIT_LINE*line, int line_num, int
    selOffset)
{
	int i, index, retCode;

	for (i = 0; i < line->len; i++) {
		if (WordText(line->line[i])) {
			index = i;

			while (i < line->len && WordText(line->line[i]))
				i++;

			retCode = CheckSpelling(file, line, line_num, index, &line->line[
			    index], i - index, selOffset);

			if (retCode == SPELL_FIXED)
				i = index - 1;

			if (retCode == SPELL_EXIT)
				return (retCode);
		}
	}
	return (SPELL_CONTINUE);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int WordText(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return (1);

	if (ch >= 'A' && ch <= 'Z')
		return (1);

	if (ch == '\'')
		return (1);

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CheckSpelling(EDIT_FILE*file, EDIT_LINE*clipLine, int line, int
    offset, char*word, int len, int selOffset)
{
	int i;
	char test[32];
	WORD_LIST choices;
	int retCode;

	choices.total = 0;
	choices.fullscan = 0;

	if (len >= MAX_WORD_LEN || len < MIN_WORD_LEN)
		return (SPELL_CONTINUE);

	memcpy(test, word, len);

	test[len] = 0;

	if (CheckWordList(WORD_LIST_IGNORE, test))
		return (SPELL_CONTINUE);

	if (test[0] == '\'')
		return (SPELL_CONTINUE);

	for (i = 0; i < len; i++)
		if (test[i] >= 'A' && test[i] <= 'Z')
			test[i] += 32;

	if (LookupWord(test, len))
		return (SPELL_CONTINUE);

	PreviousFix(test, &choices);

	SwapChars(test, len, &choices);

	SwapGroups(test, len, &choices);

	BadChar(test, len, &choices);

	MissingChar(test, len, &choices);

	ExtraChar(test, len, &choices);

	DoubleWord(test, len, &choices);

	if (!choices.total)
		FullWordScan(test, len, &choices);

	for (i = 0; i < choices.total; i++)
		FixCase(word, len, choices.words[i]);

	retCode = FixSpelling(file, clipLine, word, line, offset, len, &choices,
	    selOffset);

	for (i = 0; i < choices.total; i++)
		OS_Free(choices.words[i]);

	CenterBottomBar(0, "[+] One Moment, Spell Checking [+]");

	return (retCode);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FixCase(char*misspelled, int len, char*correct)
{
	int i;

	for (i = 0; i < len; i++) {
		if (misspelled[i] == '\'')
			continue;

		if (!(misspelled[i] >= 'A' && misspelled[i] <= 'Z'))
			break;
	}

	/* Convert correct to uppercase? */
	if (i == len) {
		for (i = 0; i < (int)strlen(correct); i++) {
			if (correct[i] >= 'a' && correct[i] <= 'z')
				correct[i] -= 32;
		}
		return ;
	}

	/* Convert first letter to uppercase? */
	if (i == 1) {
		if (correct[0] >= 'a' && correct[0] <= 'z')
			correct[0] -= 32;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int ListSpellings(WORD_LIST*choices, char*result)
{
	int index;

	index = PickList("", choices->total, 60, "Select Word", choices->words, 1);

	if (index) {
		strcpy(result, choices->words[index - 1]);
		return (1);
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddNewWord(char*word)
{
	int i, len;
	FILE_HANDLE*fp;
	char filename[MAX_FILENAME];

	len = strlen(word);

	if (len < MAX_WORD_LEN) {
		for (i = 0; i < len; i++)
			if (word[i] >= 'A' && word[i] <= 'Z')
				word[i] += 32;

		HashWord(word, len);
		AddWordList(WORD_LIST_ADDED, word, "");

		OS_GetSessionFile(filename, SESSION_WORDS, 0);
		fp = OS_Open(filename, "a");

		if (fp) {
			OS_Write(word, len, 1, fp);
			OS_Write("\n", strlen("\n"), 1, fp);
			OS_Close(fp);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddUserWords(void)
{
	FILE_HANDLE*fp;
	char filename[MAX_FILENAME];
	char word[MAX_WORD_LEN];
	int len;

	OS_GetSessionFile(filename, SESSION_WORDS, 0);

	fp = OS_Open(filename, "r");

	if (fp) {
		while (OS_ReadLine(word, sizeof(word), fp)) {
			len = strlen(word);

			if (!LookupWord(word, len))
				HashWord(word, len);
		}

		OS_Close(fp);
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int FixSpelling(EDIT_FILE*file, EDIT_LINE*clipLine, char*incorrect, int
    line, int column, int len, WORD_LIST*choices, int selOffset)
{
	int i, ch;
	int oldLen;
	char correct[MAX_WORD_LEN];
	char original[MAX_WORD_LEN];

	memcpy(original, incorrect, len);
	original[len] = 0;

	strcpy(correct, choices->words[0]);

	GotoPosition(file, line + 1, selOffset + column + 1 + len);

	SetupSelectBlock(file, line, selOffset + column, len);
	Paint(file);

	for (; ; ) {
		ch = Question(
		    "Suggest \"%s\", [C]hange, [I]gnore, [A]dd, [L]ist, [S]kip:",
		    correct);

		file->paint_flags |= (CURSOR_FLAG);

		if (ch == 'a' || ch == 'A') {
			CancelSelectBlock(file);
			memcpy(original, incorrect, len);
			original[len] = 0;
			AddNewWord(original);
			return (SPELL_CONTINUE);
		}

		if (ch == 'l' || ch == 'L') {
			/* If a full scan was not done, do one now so they have all the */
			/* possible word choices to choose from.                        */
			if (choices->fullscan == 0) {
				FullWordScan(original, len, choices);

				for (i = 0; i < choices->total; i++)
					FixCase(original, len, choices->words[i]);
			}

			if (ListSpellings(choices, correct))
				ch = 'c';
		}

		if (ch == 'c' || ch == 'C') {
			oldLen = file->cursor.line->len;
			DeleteSelectBlock(file);
			InsertText(file, correct, strlen(correct), 0);
			Paint(file);

			if (clipLine->allocSize)
				OS_Free(clipLine->line);

			clipLine->len = MIN(clipLine->len + (file->cursor.line->len -
			    oldLen), file->cursor.line->len - selOffset);
			clipLine->allocSize = clipLine->len;
			clipLine->line = OS_Malloc(clipLine->allocSize);
			memcpy(clipLine->line, &file->cursor.line->line[selOffset],
			    clipLine->len);

			file->paint_flags |= (CURSOR_FLAG);
			AddWordList(WORD_LIST_FIXED, original, correct);
			return (SPELL_FIXED);
		}

		if (ch == 's' || ch == 'S') {
			CancelSelectBlock(file);
			return (SPELL_CONTINUE);
		}

		if (ch == 'i' || ch == 'I' || ch == ED_KEY_CR) {
			CancelSelectBlock(file);
			AddWordList(WORD_LIST_IGNORE, original, original);
			return (SPELL_IGNORE);
		}

		if (ch == ED_KEY_ESC) {
			CancelSelectBlock(file);
			return (SPELL_EXIT);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static char*CheckWordList(int list, char*word)
{
	SAVED_WORDS*walk;

	walk = wordLists[list];

	while (walk) {
		if (!OS_Strcasecmp(walk->word, word))
			return (walk->fixed);

		walk = walk->next;
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddWordList(int list, char*word, char*fixed)
{
	SAVED_WORDS*add, *walk;

	walk = wordLists[list];

	/* If Word already exists, just update the fixed component. */
	while (walk) {
		if (!OS_Strcasecmp(walk->word, word)) {
			OS_Free(walk->fixed);
			walk->fixed = OS_Malloc(strlen(fixed) + 1);
			strcpy(walk->fixed, fixed);
			return ;
		}
		walk = walk->next;
	}

	add = (SAVED_WORDS*)OS_Malloc(sizeof(SAVED_WORDS));

	add->word = OS_Malloc(strlen(word) + 1);
	add->fixed = OS_Malloc(strlen(fixed) + 1);

	strcpy(add->word, word);
	strcpy(add->fixed, fixed);

	add->next = wordLists[list];

	wordLists[list] = add;
}


#ifdef DEBUG_MEMORY
/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void FreeWordLists(void)
{
	SPELL_WORD*base, *next;
	int i;

	for (i = 0; i < NUM_WORD_LISTS; i++)
		FreeWordList(i);

	for (i = 0; i < MAX_CACHED_SCANS; i++)
		FlushScanCache(i);

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		base = dictionary_hash[i];

		while (base) {
			next = base->next;

			OS_Free(base->word);
			OS_Free(base);

			base = next;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FreeWordList(int list)
{
	SAVED_WORDS*walk, *next;

	walk = wordLists[list];

	while (walk) {
		next = walk->next;

		OS_Free(walk->word);

		if (walk->fixed)
			OS_Free(walk->fixed);

		OS_Free(walk);

		walk = next;
	}

	wordLists[list] = 0;
}
#endif


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int LookupWord(char*word, int len)
{
	unsigned int hash = 5137;
	int i;
	SPELL_WORD*base;

	for (i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + word[i];

	base = dictionary_hash[hash%HASH_TABLE_SIZE];

	while (base) {
		if (base->word[0] == (char)len)
			if (!memcmp(base->word + 1, word, len))
				return (1);

		base = base->next;
	}

	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void MissingChar(char*word, int len, WORD_LIST*choices)
{
	int i, extra, newlen;
	char newword[MAX_WORD_LEN];

	for (extra = 0; extra <= len; extra++) {
		/* Pad out a word with an extra space. */
		for (newlen = 0, i = 0; i < len; i++) {
			if (i == extra)
				newlen++;

			newword[newlen++] = word[i];
		}

		if (i == extra)
			newlen++;

		newword[newlen] = 0;

		for (i = 0; i < (int)sizeof(insertChars); i++) {
			newword[extra] = insertChars[i];

			/* Look it up in the dictionary. */
			if (LookupWord(newword, newlen))
				if (!AddWordChoice(newword, choices))
					return ;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void DoubleWord(char*word, int len, WORD_LIST*choices)
{
	char temp;
	int i;
	char newword[MAX_WORD_LEN];

	if (len < 4)
		return ;

	for (i = 2; i <= len - 2; i++) {
		if (LookupWord(word + i, len - i)) {
			temp = word[i];
			word[i] = 0;

			if (LookupWord(word, i)) {
				sprintf(newword, "%s %c%s", word, temp, &word[i + 1]);
				AddWordChoice(newword, choices);
			}

			word[i] = temp;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void ExtraChar(char*word, int len, WORD_LIST*choices)
{
	int i, extra, newlen;
	char newword[MAX_WORD_LEN];

	for (extra = 0; extra < len; extra++) {
		for (newlen = 0, i = 0; i < len; i++) {
			if (i == extra)
				continue;

			newword[newlen++] = word[i];
		}
		newword[newlen] = 0;

		/* Look it up in the dictionary. */
		if (LookupWord(newword, newlen))
			if (!AddWordChoice(newword, choices))
				return ;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void BadChar(char*word, int len, WORD_LIST*choices)
{
	char temp;
	int i, j;

	for (i = 0; i < len; i++) {
		/* Save the character before it's replaced. */
		temp = word[i];

		for (j = 0; j < (int)sizeof(insertChars); j++) {
			/* Replace the character */
			word[i] = insertChars[j];

			/* Look it up in the dictionary. */
			if (LookupWord(word, len))
				AddWordChoice(word, choices);
		}

		/* Restore the character */
		word[i] = temp;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SwapChars(char*word, int len, WORD_LIST*choices)
{
	char temp;
	int i;

	for (i = 0; i < len - 1; i++) {
		/* Swap each letter pair */
		temp = word[i];
		word[i] = word[i + 1];
		word[i + 1] = temp;

		/* Look it up in the dictionary. */
		if (LookupWord(word, len))
			AddWordChoice(word, choices);

		/* Restore swapped pair */
		temp = word[i];
		word[i] = word[i + 1];
		word[i + 1] = temp;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SwapGroups(char*word, int len, WORD_LIST*choices)
{
	char newword[MAX_WORD_LEN];
	int i;
	char*ptr;

	for (i = 0; i < (int)NUM_SWAP_GROUPS; i++) {
		ptr = strstr(word, swapGroups[i].find);

		while (ptr) {
			memcpy(newword, word, ptr - word);
			strcpy(&newword[ptr - word], swapGroups[i].replace);
			strcpy(&newword[(ptr - word) + swapGroups[i].replaceLen], ptr +
			    swapGroups[i].findLen);

			/* Look it up in the dictionary. */
			if (LookupWord(newword, len))
				AddWordChoice(newword, choices);

			ptr = strstr(ptr + 1, swapGroups[i].find);
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PreviousFix(char*word, WORD_LIST*choices)
{
	char*fixed;

	fixed = CheckWordList(WORD_LIST_FIXED, word);

	if (fixed)
		AddWordChoice(fixed, choices);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int AddWordChoice(char*word, WORD_LIST*choices)
{
	int i;

	/* Is the list full? */
	if (choices->total >= MAX_WORDS)
		return (0);

	/* Does this word already exist in the list? If so, then ignore it. */
	for (i = 0; i < choices->total; i++)
		if (!strcmp(word, choices->words[i]))
			return (1);

	/* Add a new word to the list. */
	choices->words[choices->total] = OS_Malloc(strlen(word) + 1);
	strcpy(choices->words[choices->total], word);
	choices->total++;

	return (1);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FullWordScan(char*word, int len, WORD_LIST*choices)
{
	WORD_LIST words[MAX_WEIGHTS*2];
	int weight;
	int i, j;
	SPELL_WORD*base;

	/* Was this word recently scanned? If so, pull the choices from cache. */
	if (CheckScanCache(word, choices))
		return ;

	CenterBottomBar(0, "[+] One Moment, Spell Checking [+]");

	choices->fullscan = 1;

	memset(words, 0, sizeof(words));

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		base = dictionary_hash[i];

		if (!base)
			continue;

		while (base) {
			weight = WeightWord(word, len, base->word + 1, *base->word) +
			MAX_WEIGHTS;

			if (weight >= MAX_WEIGHTS*2)
				weight = (MAX_WEIGHTS*2) - 1;

			if (weight >= 0)
				words[weight].words[words[weight].total++%MAX_WORDS] =
				base->word + 1;

			base = base->next;
		}
	}

	for (i = (MAX_WEIGHTS*2) - 1; i >= 0; i--) {
		for (j = 0; j < MAX_WORDS; j++) {
			if (words[i].words[j])
				AddWordChoice(words[i].words[j], choices);
		}
	}

	AddScanCache(word, choices);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int CheckScanCache(char*word, WORD_LIST*choices)
{
	int i, j;

	for (i = 0; i < MAX_CACHED_SCANS; i++) {
		if (scanCache[i]) {
			if (!strcmp(word, scanCache[i]->word)) {
				for (j = 0; j < scanCache[i]->choices.total; j++)
					AddWordChoice(scanCache[i]->choices.words[j], choices);
				return (1);
			}
		}
	}
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void AddScanCache(char*word, WORD_LIST*choices)
{
	SPELL_SCAN_CACHE*cache;
	int i;

	cache = (SPELL_SCAN_CACHE*)OS_Malloc(sizeof(SPELL_SCAN_CACHE));

	cache->word = OS_Malloc(strlen(word) + 1);
	strcpy(cache->word, word);

	cache->choices.total = 0;

	for (i = 0; i < choices->total; i++)
		AddWordChoice(choices->words[i], &cache->choices);

	FlushScanCache(scanCacheIndex);

	scanCache[scanCacheIndex++] = cache;

	scanCacheIndex = (scanCacheIndex%MAX_CACHED_SCANS);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void FlushScanCache(int index)
{
	int i;

	if (scanCache[index]) {
		OS_Free(scanCache[index]->word);

		for (i = 0; i < scanCache[index]->choices.total; i++)
			OS_Free(scanCache[index]->choices.words[i]);

		OS_Free(scanCache[index]);
		scanCache[index] = 0;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int WeightWord(char*misspelled, int misspelledLen, char*test,
    int testLen)
{
	char saved;
	int weight = 0;
	int i, matches, matchLen, searchLen;
	char*search;

	for (matchLen = 1; matchLen <= 3; matchLen++) {
		matches = 0;
		searchLen = misspelledLen - matchLen;

		for (i = 0; i <= searchLen; i++) {
			search = misspelled + i;
			saved = search[matchLen];

			search[matchLen] = 0;

			if (strstr(test, search))
				matches++;

			search[matchLen] = saved;
		}

		weight += matches;

		if (matches < 2)
			break;
	}

	matches = (testLen - misspelledLen) - 2;

	/* Extra score if the words start with the same sequence */
	while (*(misspelled++) == *(test++))
		weight++;

	return (weight - ((matches > 0) ? matches : 0));
}


