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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dictionary.h"

#define HASH_TABLE_SIZE 0x8000

#define MAX_WORD_LEN       32
#define MIN_WORD_LEN       2
#define MAX_COMPRESSED_LEN 64

void HashWord(char*word, int len);
void DumpHashTable(void);

typedef struct dictionaryList
{
	char*word;
	struct dictionaryList*next;
}SPELL_WORD;

static SPELL_WORD*dictionary_hash[HASH_TABLE_SIZE];


/*###########################################################################*/
/*#                                                                         #*/
/*# compressedL1 [0-15] ASCII value for Top Row                             #*/
/*# compressedL2 [0-15] ASCII value for Bottom Row                          #*/
/*# compressedL3 [0-15] ASCII value for Special Row                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int main(void)
{
	char prev[MAX_COMPRESSED_LEN];
	char decompressed[MAX_COMPRESSED_LEN];
	char affix[MAX_COMPRESSED_LEN];
	char*ptr;
	int i, newLen, baseLen;
	int numNibbles = 0;
	char*nibbles;
	int index = 0;

	nibbles = malloc(sizeof(dictionary_words)*2);

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
	DumpHashTable();
	return (0);
}


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
void HashWord(char*word, int len)
{
	unsigned int hash = 5137;
	int i;
	SPELL_WORD*newWord;

	newWord = malloc(sizeof(SPELL_WORD));
	newWord->word = malloc(len + 2);
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
void DumpHashTable(void)
{
	int i;
	SPELL_WORD*base;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		base = dictionary_hash[i];

		if (!base)
			continue;

		while (base) {
			printf("%s\n", base->word + 1);
			base = base->next;
		}
	}
}


