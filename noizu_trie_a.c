/*!
* @file noizu_trie_a.c
* @brief array encoded static trie implementation.
*
* @author Keith Brings
* @copyright Noizu Labs, Inc. 2019.
*/

#include "noizu_trie_a.h"

/*!
 * @brief advance position through trie structure by find sibling or direct child matching index node. Return TRIE_NOT_FOUND indicator if current node not found or matching entry exists.
 */
TRIE_A_UNIT noizu_trie_a_advance(char key, TRIE_A_UNIT current_node, noizu_trie_a source[]) {
	if (current_node == TRIE_NOT_FOUND) return TRIE_NOT_FOUND;
	TRIE_A_UNIT trie_node = source[current_node][TRIE_A_CHILD];
	while (trie_node != TRIE_NOT_FOUND) {
		if (source[trie_node][TRIE_A_CHARACTER] == key) {
			return trie_node;
		}
		if (source[trie_node][TRIE_A_CHARACTER] < key) {
			trie_node = source[trie_node][TRIE_A_SIBLING];
		}
	}
	return TRIE_NOT_FOUND;
}
