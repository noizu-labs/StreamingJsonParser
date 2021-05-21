/*!
* @file noizu_trie_c.h
* @brief ultra compact struct encoded static trie implementation.
*
* @author Keith Brings
* @copyright Noizu Labs, Inc. 2021.
*/

#ifndef __NOIZU_TRIE_C_H__
#define __NOIZU_TRIE_C_H__

#include "../noizu_trie.h"

struct noizu_trie__compact__definition;
struct noizu_trie__compact__state;

/*
 *  @brief function to map input characters to compact format. 0 returned to indicate no encoding found. Positive return value indicates index position of character (plus 1).
 */
typedef TRIE_CHAR_CODE(*noizu_trie_c_char_code)(TRIE_CHAR_CODE);

/*!
 * @brief set token, token_lookup_memo and token_index of state pointer.
 * @param clear - set token and token_index to zero if no match.
 * @return true - token found, false - no token found
 */
typedef TRIE_TOKEN(*noizu_trie_c_token_code)(uint32_t index, struct noizu_trie_definition*, uint8_t* has_token);

/*!
* @brief super compact trie tree definition. Trie tree and token's are encoded in binary data stream of variable width depending on characteristics of input.
 */
typedef struct noizu_trie__compact__definition {
	uint16_t size; //!< total nodes in trie tree
	uint16_t tokens; //!< total tokens defined
	uint16_t characters; //!< total unique characters in trie.
	uint8_t bit_length; //!< total record with
	uint8_t bit_length__character_code; //!< number of bits used to encode each character. 
	uint8_t bit_length__sibling_relative_index;  //!< number of bits used to encode distance (in nodes) to next sibling
	uint8_t bit_length__child_relative_offset; //!< to avoid calculations., just char + sib length
	uint8_t bit_length__child_relative_index; //!< number of bits used to encode distance (in nodes) to next child
	uint8_t* trie_raw; //!< binary stream of tokens in form of  {char_bits, next_sibling_bits, next_child_bits}
	uint32_t trie_raw_length; //!< total bytes to avoid overruns.
	TRIE_CHAR_CODE* char_map; //!< character look up table, the trie for "Hello World" for example would have a char map of ['H', 'e', 'l', 'o', 'W', 'r', 'd'] and would use 3 bits to encode each character
    
	noizu_trie_c_token_code token_code; //!< map node index to token function, 
	noizu_trie_c_char_code char_code;
} noizu_trie_compact_definition;

TRIE_TOKEN noizu_trie__compact__reset(struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
TRIE_TOKEN noizu_trie__compact__init(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
TRIE_TOKEN noizu_trie__compact__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode);
TRIE_TOKEN noizu_trie__compact__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition);
TRIE_TOKEN noizu_trie__compact__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition);

#endif