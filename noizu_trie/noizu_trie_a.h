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

#ifndef TRIE_NOT_FOUND
/*!
 * @brief string not found special index value.
 * This special trie index functions as termination well to halt processing of non matched input strings.
 * In generated tries trie head begin at index position (1) with a non parsed special token value. (*) by default.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
#define TRIE_NOT_FOUND 0
#endif

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
#define TRIE_A_UNIT int
#endif

	  /*!
	   * @brief typedef for array encoded trie (entry).
	   * @author Keith Brings
	   * @repo github.com/noizu/trie_gen
	   */
typedef TRIE_A_UNIT noizu_trie_a[4];

/*!
 * @brief advance position through trie structure by find sibling or direct child matching index node. Return TRIE_NOT_FOUND indicator if current node not found or matching entry exists.
 * @author Keith Brings
 * @repo github.com/noizu/trie_gen
 */
TRIE_A_UNIT noizu_trie_a_advance(char key, TRIE_A_UNIT position, noizu_trie_a source[]);
#endif