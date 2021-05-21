/*!
* @file noizu_trie_s.h
* @brief structed encoded static trie implementation.
*
* @author Keith Brings
* @copyright Noizu Labs, Inc. 2019.
*/

#ifndef __NOIZU_TRIE_S_H__
#define __NOIZU_TRIE_S_H__


#include "../noizu_trie.h"

#ifndef TRIE_S_UNIT
 /*!
  * @brief string not found special index value.
  * This special trie index functions as termination well to halt processing of non matched input strings.
  * In generated tries trie head begin at index position (1) with a non parsed special token value. (*) by default.
  */
#define TRIE_S_UNIT TRIE_TOKEN
#endif

#ifndef TRIE_S_TOKEN
#define TRIE_S_TOKEN TRIE_TOKEN
#endif

/*!
 * @brief trie node structure. Static trie encoded as an array of structs or pointer to allocation of structs.
 */
typedef struct noizu_trie_s{
	TRIE_CHAR_CODE key;
	TRIE_S_UNIT next_sibling;
	TRIE_S_UNIT first_child;
	TRIE_S_TOKEN termination_code;
} noizu_trie_s;

typedef struct noizu_trie__struct__definition {
	noizu_trie_s* trie;
	TRIE_S_UNIT trie_length;
} noizu_trie_array_definition;


/*!
 * @brief advance position through trie structure by find sibling or direct child matching index node. Return TRIE_NOT_FOUND indicator if current node not found or matching entry exists.
 * @deprecated
 */
TRIE_S_UNIT noizu_trie_s_advance(char k, TRIE_S_UNIT current_node, noizu_trie_s* source);

TRIE_TOKEN noizu_trie__struct__init(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
TRIE_TOKEN noizu_trie__struct__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode);
TRIE_TOKEN noizu_trie__struct__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition);
TRIE_TOKEN noizu_trie__struct__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition);

#endif
