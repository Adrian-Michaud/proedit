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

#define HASH_TABLE_SIZE 160000

typedef struct dictionaryList
{
	char*word;
	char suffix;
	struct dictionaryList*next;
}SPELL_WORD;

static SPELL_WORD*word_hash[HASH_TABLE_SIZE];

static void HashWord(char*word, int len);
static int HashIt(char*word, int len);
static int LoadDict(char*dict, int mode);


/*###########################################################################*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*#                                                                         #*/
/*###########################################################################*/
int main(int argv, char**argc)
{
	if (argv != 3) {
		printf("Usage: comp_dict <dictionary1> <dictionary2>\n");
		return (0);
	}

	LoadDict(argc[1], 0);
	LoadDict(argc[2], 1);

	return (0);
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
static int LoadDict(char*dict, int mode)
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

		if (mode)
			printf("Added from %s: %s\n", dict, list);

		HashWord(list, len);
		total++;
	}

	fclose(fp);

	return (total);
}


