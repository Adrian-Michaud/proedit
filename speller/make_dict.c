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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if 0
#define DEBUG
#endif

static char*suffixList[] = {
	"ness's",
	"ion's",
	"ions",
	"ness",
	"able",
	"ings",
	"ive",
	"ion",
	"ing",
	"ers",
	"ies",
	"ier",
	"ant",
	"e's",
	"y's",
	"r's",
	"'s",
	"en",
	"ry",
	"ly",
	"ed",
	"er",
	"st",
	"rs",
	"es",
	"r",
	"d",
	"s",
	"y",
	"e",
};

#define MAX_WORD_LEN       32

#define NUM_CHAR_FREQ 256

typedef struct characterFrequency
{
	char ch;
	int occurances;
}CHAR_FREQ;

typedef struct suffixFrequency
{
	char ch;
	int suffix;
	int occurances;
}SUFFIX_FREQ;

static char compressedL1[16];
static char compressedL2[16];
static char compressedL3[16];

static CHAR_FREQ charFreq[NUM_CHAR_FREQ];

static int charLevel[256];
static char charLookup[256];

#define NUM_SFX (sizeof(suffixList)/sizeof(char *))

#define ALPHA_LOW    'a'
#define ALPHA_HIGH   '~'

#define HASH_TABLE_SIZE 160000

typedef struct dictionaryList
{
	char*word;
	char suffix;
	struct dictionaryList*next;
}SPELL_WORD;

static int nibbleIndex;
static char*nibbles;
static int numSorted;
static int allocSize;

static SUFFIX_FREQ suffixFreq[NUM_SFX];
static SPELL_WORD*suffix_hash[HASH_TABLE_SIZE];
static SPELL_WORD*word_hash[HASH_TABLE_SIZE];
static SPELL_WORD*sorted[HASH_TABLE_SIZE];

static void HashSuffix(char*word, int len, int suffix);
static void HashWord(char*word, int len);
static int HashIt(char*word, int len);
static int SortChar(const void*arg1, const void*arg2);
static int LoadDict(char*dict);
static SPELL_WORD*WordLookup(char*word, int len);
static void OtherSuffixCheck(char*root, int len, int suffix);
static void SortHash(SPELL_WORD*words);
static int SortWords(const void*arg1, const void*arg2);
static void CompressPrefixes(void);
static void PrintStrings(FILE*fp, SPELL_WORD*words);
static void CompressLine(char*line, int len);
static int SortCharFreq(const void*arg1, const void*arg2);
static void BuildFreqTable(void);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static SPELL_WORD*WordLookup(char*word, int len)
{
	SPELL_WORD*walk;

	/* Check to see if other suffixs match this root word. */
	walk = word_hash[HashIt(word, len)];

	while (walk) {
		if (walk->word) {
			if (!strcmp(walk->word, word))
				return (walk);
		}
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
int main(int argv, char**argc)
{
	int i, j, len, alen;
	char*word;
	FILE*fp;
	char list[50];
	char item[20];
	char flag;
	int total;
	SPELL_WORD*hashWalk, *hashWalk2;

	if (argv >= 2) {
		for (i = 1; i < argv; i++)
			LoadDict(argc[i]);
	} else
		LoadDict("dictionary.txt");

	printf("Creating wordlist.tmp\n");

	fp = fopen("wordlist.tmp", "w");
	for (i = 0; i < HASH_TABLE_SIZE; i++)
		PrintStrings(fp, word_hash[i]);
	fclose(fp);

	printf("Processing suffixes\n");

	/* Walk through all of the hashed words looking for suffixes. */
	for (j = 0; j < NUM_SFX; j++) {
		alen = strlen(suffixList[j]);

		for (i = 0; i < HASH_TABLE_SIZE; i++) {
			hashWalk = word_hash[i];

			while (hashWalk) {
				if (hashWalk->word && !hashWalk->suffix) {
					len = strlen(hashWalk->word);

					if (len > alen) {
						/* Did we find a suffix in this word? If so, add it to */
						/* the suffix hash table and remove it from the word   */
						/* hash table.                                         */
						if (!strcmp(&hashWalk->word[len - alen],
						    suffixList[j])) {
							word = hashWalk->word;

							word[len - alen] = 0;

							suffixFreq[j].suffix = j;
							suffixFreq[j].occurances++;

							HashSuffix(word, len - alen, j);

							/* If there is a root word, mark it. */
							hashWalk2 = WordLookup(word, len - alen);

							if (hashWalk2)
								hashWalk2->suffix = 1;

							/* Remove this suffix word from the word list because */
							/* it's now located on the suffix list.               */
							hashWalk->word = 0;

							/* Process this suffix, and all other suffixes that */
							/* use this same root word.                         */
							OtherSuffixCheck(word, len - alen, j);
						}
					}
				}
				hashWalk = hashWalk->next;
			}
		}
	}

	printf("Combining suffixes\n");

	/* Find all duplicate root words and combine their suffixes into one */
	/* word + suffix list.                                               */
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hashWalk = suffix_hash[i];

		while (hashWalk) {
			if (hashWalk->word) {
				sprintf(list, "%c", ALPHA_LOW + hashWalk->suffix);
				total = 0;

				hashWalk2 = suffix_hash[HashIt(hashWalk->word,
				    strlen(hashWalk->word))];

				while (hashWalk2) {
					if (hashWalk2->word) {
						if (hashWalk2->word != hashWalk->word) {
							if (!strcmp(hashWalk2->word, hashWalk->word)) {
								sprintf(item, "%c", ALPHA_LOW + hashWalk2->
								    suffix);
								strcat(list, item);
								hashWalk2->word = 0;
								total++;
							}
						}
					}
					hashWalk2 = hashWalk2->next;
				}

				flag = '/';

				/* Check if this root word exists by itself. If so, them mark */
				/* the suffix list using a '+' to indicate that the root word */
				/* is also a word by itself with adding any suffixes to it.   */
				hashWalk2 = word_hash[HashIt(hashWalk->word, strlen(hashWalk->
				    word))];

				while (hashWalk2) {
					/* Is this a root word? */
					if (hashWalk2->word) {
						if (!strcmp(hashWalk->word, hashWalk2->word)) {
							flag = '+';
							hashWalk2->word = 0;
							break;
						}
					}
					hashWalk2 = hashWalk2->next;
				}

				/* Is this a single word with a single suffix?  If the suffix is */
				/* less than 2 characters, then don't bother because we're not   */
				/* saving any space by using the suffex.                         */
				if (!total && (flag == '/') && (strlen(suffixList[(int)
				    hashWalk->suffix]) <= 2)) {
					word = (char*)malloc(40);
					sprintf(word, "%s%s", hashWalk->word, suffixList[(int)
					    hashWalk->suffix]);
					hashWalk->word = word;
				} else {
					word = (char*)malloc(40);
					qsort(list, strlen(list), 1, SortChar);
					sprintf(word, "%s%c%s", hashWalk->word, flag, list);
					hashWalk->word = word;
				}
			}
			hashWalk = hashWalk->next;
		}
	}

	printf("Sorting words\n");
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		SortHash(word_hash[i]);
		SortHash(suffix_hash[i]);
	}

	qsort(sorted, numSorted, sizeof(SPELL_WORD*), SortWords);

	printf("Creating suffixes.tmp\n");

	fp = fopen("suffixes.tmp", "w");
	for (i = 0; i < numSorted; i++)
		fprintf(fp, "%s\n", sorted[i]->word);
	fclose(fp);

	printf("Compressing words\n");

	CompressPrefixes();

	BuildFreqTable();

	nibbles = malloc(allocSize*2);
	nibbleIndex = 0;

	for (i = 0; i < numSorted; i++)
		CompressLine(sorted[i]->word, strlen(sorted[i]->word));

	if (nibbleIndex&1)
		nibbles[nibbleIndex++] = 0xf;

	printf("Creating dictionary.h\n");

	fp = fopen("dictionary.h", "w");

	fprintf(fp,
	    "/*###########################################################################*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#              ProEdit MP Multi-platform Programming Editor               #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#           Designed/Developed/Produced by Adrian J. Michaud              #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#        (C) 2006-2007 by Adrian J. Michaud. All Rights Reserved.         #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#      Unpublished - rights reserved under the Copyright Laws of the      #*/\n");
	fprintf(fp,
	    "/*#      United States.  Use, duplication, or disclosure by the             #*/\n");
	fprintf(fp,
	    "/*#      Government is subject to restrictions as set forth in              #*/\n");
	fprintf(fp,
	    "/*#      subparagraph (c)(1)(ii) of the Rights in Technical Data and        #*/\n");
	fprintf(fp,
	    "/*#      Computer Software clause at 252.227-7013.                          #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#      This software contains information of a proprietary nature         #*/\n");
	fprintf(fp,
	    "/*#      and is classified confidential.                                    #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*#      ALL INFORMATION CONTAINED HEREIN SHALL BE KEPT IN CONFIDENCE.      #*/\n");
	fprintf(fp,
	    "/*#                                                                         #*/\n");
	fprintf(fp,
	    "/*###########################################################################*/\n");

	fprintf(fp, "\n");

	fprintf(fp, "\nstatic char compressedL1[] = {\n   ");
	for (i = 0; i < sizeof(compressedL1); i++)
		if (compressedL1[i])
			fprintf(fp, "0x%02x,", compressedL1[i]);
	fprintf(fp, "\n};\n");

	fprintf(fp, "\nstatic char compressedL2[] = {\n   ");
	for (i = 0; i < sizeof(compressedL2); i++)
		if (compressedL2[i])
			fprintf(fp, "0x%02x,", compressedL2[i]);
	fprintf(fp, "\n};\n");

	fprintf(fp, "\nstatic char compressedL3[] = {\n   ");
	for (i = 0; i < sizeof(compressedL3); i++)
		if (compressedL3[i])
			fprintf(fp, "0x%02x,", compressedL3[i]);
	fprintf(fp, "\n};\n");

	fprintf(fp, "\n");
	fprintf(fp, "\nstatic char *suffixes[] = {\n");

	for (i = 0; i < NUM_SFX; i++) {
		if (suffixFreq[i].occurances)
			fprintf(fp, "  \"%s\", \t/* %d hits */\n", suffixList[i],
			    suffixFreq[i].occurances);
		else
			fprintf(fp, "  (char*)0, \t/* \"%s\" */\n", suffixList[i]);
	}

	fprintf(fp, "};\n\n");


	fprintf(fp, "\nstatic unsigned char dictionary_words[]= {\n");

	total = 0;
	for (i = 0; i < nibbleIndex; i += 2) {
		fprintf(fp, "0x%02x,", ((unsigned char)(nibbles[i] << 4) | (unsigned
		    char)nibbles[i + 1]));

		if (++total >= 15) {
			total = 0;
			fprintf(fp, "\n");
		}
	}

	fprintf(fp, "\n};\n");
	fclose(fp);
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void PrintStrings(FILE*fp, SPELL_WORD*words)
{
	while (words) {
		if (words->word)
			fprintf(fp, "%s\n", words->word);
		words = words->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void OtherSuffixCheck(char*root, int len, int suffix)
{
	int i, alen;
	char newword[64];
	SPELL_WORD*word;


	for (i = 0; i < NUM_SFX; i++) {
		if (i == suffix)
			continue;

		alen = strlen(suffixList[i]);

		sprintf(newword, "%s%s", root, suffixList[i]);

		#ifdef DEBUG
		printf("Checking %s (%s and %s)\n", newword, root, suffixList[i]);
		#endif

		word = WordLookup(newword, len + alen);

		if (word) {
			#ifdef DEBUG
			printf("Found '%s'\n", newword);
			#endif

			suffixFreq[i].suffix = i;
			suffixFreq[i].occurances++;

			HashSuffix(root, len, i);

			/* Remove this suffix word from the word list because */
			/* it's now located on the suffix list.               */
			word->word = 0;
		}
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void SortHash(SPELL_WORD*words)
{
	while (words) {
		if (words->word)
			sorted[numSorted++] = words;
		words = words->next;
	}
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortChar(const void*arg1, const void*arg2)
{
	char c1, c2;

	c1 = *(char*)arg1;
	c2 = *(char*)arg2;

	return (c1 - c2);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HashSuffix(char*word, int len, int suffix)
{
	int index;
	SPELL_WORD*newWord;

	newWord = malloc(sizeof(SPELL_WORD));
	newWord->suffix = (char)suffix;
	newWord->word = malloc(len + 1);
	memcpy(newWord->word, word, len);
	newWord->word[len] = 0;

	index = HashIt(word, len);

	newWord->next = suffix_hash[index];

	suffix_hash[index] = newWord;

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void HashWord(char*word, int len)
{
	int index;
	SPELL_WORD*newWord;

	newWord = malloc(sizeof(SPELL_WORD));
	newWord->word = malloc(len + 1);
	newWord->suffix = 0;
	memcpy(newWord->word, word, len);
	newWord->word[len] = 0;

	index = HashIt(word, len);

	newWord->next = word_hash[index];

	word_hash[index] = newWord;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int HashIt(char*word, int len)
{
	unsigned int hash = 5137;
	int i;

	for (i = 0; i < len; i++)
		hash = ((hash << 5) + hash) + word[i];

	return (hash%HASH_TABLE_SIZE);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int LoadDict(char*dict)
{
	FILE*fp;
	int i, total = 0, len, dup = 0;
	char list[256];
	SPELL_WORD*hashWalk;

	fp = fopen(dict, "r");

	if (!fp) {
		printf("\nUnable to open dictionary file %s\n", dict);
		return (0);
	}

	while (fgets(list, sizeof(list) - 1, fp)) {
		len = strlen(list);

		while (len && (list[len - 1] == 10 || list[len - 1] == 13)) {
			len--;
			list[len] = 0;
		}

		/* Throw away 1 letter words */
		if (len < 2) {
			printf("Ignored '%s' 1 letter word.\n", list);
			continue;
		}

		/* Throw away words that are too long. */
		if (len > MAX_WORD_LEN) {
			printf("Ignored '%s' because it's too lonig\n", list);
			continue;
		}

		/* Convert word to lower case */
		for (i = 0; i < len; i++)
			if (list[i] >= 'A' && list[i] <= 'Z')
				list[i] += 32;

		/* Throw away words with non standard letters. */
		for (i = 0; i < len; i++) {
			if ((list[i] >= 'a' && list[i] <= 'z') || list[i] == '\'')
				continue;
			break;
		}

		if (i != len) {
			printf("Ignored '%s' due to non-standard letters.\n", list);
			continue;
		}

		/* Check for any duplicates. */
		hashWalk = word_hash[HashIt(list, len)];

		while (hashWalk) {
			if (hashWalk->word) {
				if (!strcmp(hashWalk->word, list))
					break;
			}
			hashWalk = hashWalk->next;
		}

		/* Throw away any duplicates. */
		if (hashWalk) {
			dup++;
			continue;
		}

		HashWord(list, len);
		total++;
	}

	fclose(fp);
	printf("%d words loaded, and %d duplicates from %s\n", total, dup, dict);

	return (total);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortWords(const void*arg1, const void*arg2)
{
	SPELL_WORD*word1 = *(SPELL_WORD**)arg1;
	SPELL_WORD*word2 = *(SPELL_WORD**)arg2;

	return (strcmp(word1->word, word2->word));
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static int SortCharFreq(const void*arg1, const void*arg2)
{
	CHAR_FREQ*freq1 = (CHAR_FREQ*)arg1;
	CHAR_FREQ*freq2 = (CHAR_FREQ*)arg2;

	return (freq2->occurances - freq1->occurances);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void CompressPrefixes(void)
{
	int i, j, len;

	char previous[64];

	strcpy(previous, "");

	for (j = 0; j < numSorted; j++) {
		len = strlen(sorted[j]->word);

		if (strlen(previous)) {
			for (i = 0; i < len; i++)
				if (previous[i] != sorted[j]->word[i])
					break;

			strcpy(previous, sorted[j]->word);

			if (i >= 2) {
				if (i > 15)
					i = 15;

				sprintf(sorted[j]->word, "%c%s", (unsigned char)(0xF0 | i), &
				    previous[i]);
			}
		} else
			strcpy(previous, sorted[j]->word);
	}

}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void CompressLine(char*line, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (i == 0) {
			if ((unsigned char)line[i] >= 0xf0) {
				nibbles[nibbleIndex++] = (unsigned char)line[i]&0xf;
				continue;
			} else
				nibbles[nibbleIndex++] = 0x00;
		}

		switch (charLevel[(unsigned char)line[i]]) {
		case 1 :
			nibbles[nibbleIndex++] = charLookup[(unsigned char)line[i]];
			break;

		case 2 :
			nibbles[nibbleIndex++] = 0x0d;
			nibbles[nibbleIndex++] = charLookup[(unsigned char)line[i]];
			break;

		case 3 :
			nibbles[nibbleIndex++] = 0x0e;
			nibbles[nibbleIndex++] = charLookup[(unsigned char)line[i]];
			break;

			default :
			{
				printf("\nError, Unknown byte '%c' 0x%02x at offset %d\n", (
				    unsigned char)line[i], (unsigned char)line[i], i);
				exit(0);
			}
		}
	}

	nibbles[nibbleIndex++] = 0xf;
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
static void BuildFreqTable(void)
{
	int i;
	int len, j;
	unsigned char ch;

	memset(charFreq, 0, sizeof(charFreq));
	allocSize = 0;

	for (i = 0; i < numSorted; i++) {
		len = strlen(sorted[i]->word);

		for (j = 0; j < len; j++) {
			ch = sorted[i]->word[j];

			if (ch >= 0xf0)
				continue;

			charFreq[ch].ch = ch;
			charFreq[ch].occurances++;
		}
		allocSize += len;
	}

	qsort(charFreq, 256, sizeof(CHAR_FREQ), SortCharFreq);

	for (i = 0; i < 13; i++) {
		ch = charFreq[i].ch;
		compressedL1[i] = ch;
		charLevel[ch] = 1;
		charLookup[ch] = i;
	}

	for (i = 0; i < 13; i++) {
		ch = charFreq[i + 13].ch;
		compressedL2[i] = ch;
		charLevel[ch] = 2;
		charLookup[ch] = i;
	}

	for (i = 0; i < 13; i++) {
		ch = charFreq[i + 26].ch;
		compressedL3[i] = ch;
		charLevel[ch] = 3;
		charLookup[ch] = i;
	}

	printf("\nLevel 1:");
	for (i = 0; i < 13; i++)
		if (compressedL1[i])
			printf("%c=%d ", compressedL1[i], i);

	printf("\nLevel 2:");
	for (i = 0; i < 13; i++)
		if (compressedL2[i])
			printf("%c=%d ", compressedL2[i], i);

	printf("\nLevel 3:");
	for (i = 0; i < 13; i++)
		if (compressedL3[i])
			printf("%c=%d ", compressedL3[i], i);

	printf("\n\nFrequency table:\n");
	for (i = 0; i < 256; i++)
		if (charFreq[i].occurances)
			printf("%c = %d occurances\n", charFreq[i].ch, charFreq[i].
			    occurances);

}


