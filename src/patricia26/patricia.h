/*
 * SPDX-License-Identifier: (LGPL-3.0-or-later AND UMich-Merit)
 *
 * Copyright (c) 1997, 1998, 1999 Dave Plonka <plonka@doit.wisc.edu>
 *
 * $Id: COPYRIGHT,v 1.1.1.1 2013/08/15 18:46:09 labovit Exp $
 *
 * Copyright (c) 1999-2013
 *
 * The Regents of the University of Michigan ("The Regents") and Merit
 * Network, Inc.
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ---
 * Modifications by Joel Sommers <jsommers@colgate.edu> in PyTricia 
 * Pytricia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pytricia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * ---
 * Modifications Copyright (c) 2026 Gene C <arch@sapience.com>
 *
 * These modifications and additions are free software; you can redistribute them and/or
 * modify them under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 */

/*
 * $Id: patricia.h 6811 2009-07-06 20:41:10Z robin $
 * Dave Plonka <plonka@doit.wisc.edu>
 *
 * This product includes software developed by the University of Michigan,
 * Merit Network, Inc., and their contributors. 
 *
 * This file had been called "radix.h" in the MRT sources.
 *
 * I renamed it to "patricia.h" since it's not an implementation of a general
 * radix trie.  Also, pulled in various requirements from "mrt.h" and added
 * some other things it could be used as a standalone API.
 */

#ifndef PATRICIA_H
#define PATRICIA_H

#include "patricia-export.h"
#include <stdint.h>
#include <sys/socket.h>

/*
 * bit_test: true iff any bit set in `mask` is also set in `byte_val`.
 * Replaces the old `#define BIT_TEST(f, b) ((f) & (b))` macro - same
 * logic, but as a type-checked inline function instead of a textual
 * substitution.
 */
static inline bool bit_test(unsigned int byte_val, unsigned int mask) {
    return (byte_val & mask) != 0;
}

enum {
    PATRICIA_BITS_PER_BYTE = 8U,
    PATRICIA_BYTE_INDEX_SHIFT = 3U,     /* bit_index / 8, as a shift */
    PATRICIA_BIT_INDEX_MASK = 0x07U,    /* bit_index % 8 */
    PATRICIA_HIGH_BIT = 0x80U,          /* most-significant bit of a byte */
};

/*
 * addr_bit_is_set: is bit number `bit_index` (0 = most significant bit
 * of addr[0]) set in the address byte string `addr`?
 *
 * This is the same `bit_test(addr[bit >> 3], 0x80 >> (bit & 0x07))`
 * expression that used to be written out separately at six call sites
 * across patricia.c, each with its own set of "magic number" and
 * "signed integer operand in a bitwise op" findings (0x80 and 0x07 are
 * plain `int` literals, so `>>`/`&` against them technically mix signed
 * and unsigned operands). Consolidating it here fixes both classes of
 * finding in one place instead of six, and gives the idiom a name.
 */
static inline bool addr_bit_is_set(const uint8_t *addr, unsigned int bit_index) {
    unsigned int byte = addr[bit_index >> (unsigned int)PATRICIA_BYTE_INDEX_SHIFT];
    unsigned int mask = (unsigned int)PATRICIA_HIGH_BIT >> (bit_index & (unsigned int)PATRICIA_BIT_INDEX_MASK);
    return bit_test(byte, mask);
}

#include <netinet/in.h> 

/* { from mrt.h */

/*
 * Unused
typedef struct prefix4_tag {
    uint16_t family;		
    uint16_t bitlen;		
    int ref_count;		
    struct in_addr sin;
} prefix4_t;

typedef struct prefix6_tag {
    uint16_t family;		
    uint16_t bitlen;		
    int ref_count;		
    struct in6_addr sin6;
} prefix6_t;
*/


/**
 * An IP address plus it's network prefix.
 *
 * This structure maps structured multi-family properties (IPv4 and IPv6 layouts) into a
 * unified interface. The IP address is held in a union to handle either IPv4 or IPv6.
 *
 * Dev Note: 
 * - Support for IPv6 is no longer #if defined away via HAVE_IPV6    
 * - bitlen is now 1 byte - long enough for ipv4 and ipv6. 
 * - Add 1 byte padding to keep struct explicitly aligned on even byte boundary
 *
 * Attributes:
 */
typedef struct prefix_tag {
    /** Address protocol (AF_INET | AF_INET6 ) */
    sa_family_t family;		

    /** netowrk bitmask mask length; aka cidr prefix len (e.g., 24 for a /24 block) */
    uint8_t bitlen;	

    uint8_t pad;

    /** Ref couner for allocation tracking with memory reclamation. */
    int ref_count;

    /**
     * Network address (IPV4 or IPv6)
     */
    union {
        /** add.sin: struct for 32-bit IPv4 address */
		struct in_addr sin;

        /** add.sin6 struct for 128-bit IPv6 address */
		struct in6_addr sin6;
    } add;
} prefix_t;

/* } */


/**
 * Core node structure within the Patricia Trie array matrix.
 *
 * Each node is either a glue node or a node with data payload.
 * Each node is part of the tree.
 *
 * Follow pytricia in changing node->prefix to be prefix_t instead of prefix_t *
 * simpliefies malloc/free but 
 *
 * Attributes:
 */
typedef struct patricia_node_tag {
    /** The specific bit index position */
    uint32_t bit;

    /** The prefix for this node */
    prefix_t prefix;

    /** Pointer to the left child node */
    struct patricia_node_tag *l;

    /** Pointer to the right child node */
    struct patricia_node_tag *r;

    /** Pointer to the parent node */
    struct patricia_node_tag *parent;

    /** The data payload for non-glue nodes */
    void *data;

    /* unused */
    void	*user1;
} patricia_node_t;

/**
 * Top level struct for a Patricia Trie instance.
 *
 * This is the master anchor for the network tree,
 * Has the head node  along with the max number of IP bits, the number of active bodes
 * and a marker whether the tree is *frozen* or not.
 *
 * Attributes:
 */
typedef struct patricia_tree_tag {
    /** Pointer to the top-most tree node */
    patricia_node_t 	*head;

    /** Max number of bits per network IP address */
    uint32_t		maxbits;

    /** Tracks total number of populated nodes */
    int num_active_node;

    /** Marks the tree as frozen or not */
    uint16_t frozen;
} patricia_tree_t;



/* 
 * Optional memory clean up functions passed to Clear_Patricia
 */
typedef void (*void_fn_t)(void *);
typedef void (*void_fn_2_t)(struct prefix_tag *, void *);

enum { PATRICIA_MAXBITS = 128U };

/*
 * Private function declarations
 */
int comp_with_mask(const void *addr, const void *dest, unsigned int mask);
patricia_node_t * patricia_search_best2 (patricia_tree_t *patricia, prefix_t *prefix, int inclusive);
void patricia_process (patricia_tree_t *patricia, void_fn_2_t func);

/*
 * prefix_tochar: raw address bytes behind a prefix_t (nullptr-safe).
 * This is the sole implementation now - it used to coexist with an
 * unchecked `prefix_touchar` macro that did the same thing without the
 * nullptr guard; all call sites now go through this function instead.
 */
uint8_t *prefix_tochar(prefix_t *prefix);

char *prefix_toa(prefix_t *prefix);
char *prefix_toa2(prefix_t *prefix, char *buff);

/*
 * patricia_nbit/patricia_nbyte: bit mask / byte offset for bit index
 * `bit_index` within a prefix's address bytes. Replace the old
 * PATRICIA_NBIT(x) and PATRICIA_NBYTE(x) macros - kept as part of the
 * public API in case external callers used them, even though nothing
 * in this codebase still does.
 *
 * The original PATRICIA_NBIT(x) macro was `0x80 >> ((x) & 0x7f)`: masking
 * x to 0-127 and then shifting a plain `int` (0x80) right by up to 127
 * places is undefined behavior in C for any shift count >= 32 (the width
 * of int). Every real call site elsewhere in this file has always used
 * `0x80 >> (bit & 0x07)` - masking to 0-7, i.e. "which bit within this
 * byte" - so 0x7f here looks like a long-standing typo for 0x07 in code
 * that (per its own header comment) nothing in this project actually
 * calls. Fixed to match the pattern used everywhere else.
 */
static inline unsigned int patricia_nbit(unsigned int bit_index) {
    return (unsigned int)PATRICIA_HIGH_BIT >> (bit_index & (unsigned int)PATRICIA_BIT_INDEX_MASK);
}

static inline unsigned int patricia_nbyte(unsigned int bit_index) {
    return bit_index >> (unsigned int)PATRICIA_BYTE_INDEX_SHIFT;
}

/*
 * The following used to also live here and have been removed as dead
 * code - nothing in patricia.c, patriciapy26.pyx, or any other file in
 * this project referenced them:
 *
 *  - PATRICIA_DATA_GET(node, type) / PATRICIA_DATA_SET(node, value):
 *    trivial casts to/from node->data. PATRICIA_DATA_GET's whole reason
 *    to be a macro rather than a function was taking `type` as a
 *    parameter, which a real C function can't do (no generics); if you
 *    need this back, `(type *)node->data` and `node->data = (void *)value`
 *    inline at the call site are exactly what the macros expanded to.
 *  - PATRICIA_WALK / PATRICIA_WALK_ALL / PATRICIA_WALK_BREAK /
 *    PATRICIA_WALK_END: iterative pre-order tree walk. Clear_Patricia()
 *    and patricia_process() in patricia.c each already inline their own
 *    copy of this walk (with an explicit stack array) rather than using
 *    the macro pair, so there is no remaining call site.
 */

/*
 * Exported Public Function Declarations
 */
PATRICIA_EXPORT patricia_tree_t *New_Patricia (int maxbits);
PATRICIA_EXPORT prefix_t *New_Prefix(int family, void *dest, int bitlen, prefix_t *prefix);
PATRICIA_EXPORT patricia_node_t *patricia_lookup (patricia_tree_t *patricia, prefix_t *prefix);
PATRICIA_EXPORT patricia_node_t *patricia_search_best (patricia_tree_t *patricia, prefix_t *prefix);
PATRICIA_EXPORT patricia_node_t *patricia_search_exact (patricia_tree_t *patricia, prefix_t *prefix);
PATRICIA_EXPORT char *prefix_toa2x(prefix_t *prefix, char *buff, int with_len);
PATRICIA_EXPORT void Destroy_Patricia (patricia_tree_t *patricia, void_fn_t func);
PATRICIA_EXPORT void patricia_remove (patricia_tree_t *patricia, patricia_node_t *node);
PATRICIA_EXPORT void Clear_Patricia (patricia_tree_t *patricia, void_fn_t func);


#endif /* PATRICIA_H */
