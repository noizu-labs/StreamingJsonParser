/*!
* @file noizu_trie_a.h
* @brief array encoded static trie implementation.
*
* @author Keith Brings
* @repo github.com/noizu/trie_gen
* @copyright Noizu Labs, Inc. 2019.
*/

#ifndef __NOIZU_TRIE_A_H__
#define __NOIZU_TRIE_A_H__

#include "..\noizu_trie.h"

/*!
 * @brief Array Index holding Trie Node's Character Code.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_A_CHARACTER 0

/*!
 * @brief Array Index holding Trie Node's Adjacent Sibling
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_A_SIBLING 1

/*!
 * @brief Array Index holding Trie Node's First Child Position.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_A_CHILD 2

/*!
 * @brief Array Index holding Trie Node's String Identifier/Token Code
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_A_TOKEN 3

#ifndef TRIE_A_UNIT
/*!
 * @brief may be used to override default trie array index size. To allow for long sized indexed keys for large tries, or unsigned shorts for short and memory conscious tries.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_A_UNIT uint8_t
#define TRIE_A_UNIT_SIGNED 1
#define TRIE_A_UNIT_BITS 8
#endif

/*!
 * @brief typedef for array encoded trie (entry).
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
typedef TRIE_A_UNIT noizu_trie_a[4];

typedef struct noizu_trie__array__definition {
	noizu_trie_a* trie;
	TRIE_A_UNIT trie_length;
} noizu_trie_array_definition;


/*!
 * @brief advance position through trie structure by find sibling or direct child matching index node. Return TRIE_NOT_FOUND indicator if current node not found or matching entry exists.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
TRIE_A_UNIT noizu_trie_a_advance(char key, TRIE_A_UNIT position, noizu_trie_a source[]);



TRIE_TOKEN noizu_trie__array__init(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
TRIE_TOKEN noizu_trie__array__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition);
TRIE_TOKEN noizu_trie__array__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode);
TRIE_TOKEN noizu_trie__array__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition);



#endif