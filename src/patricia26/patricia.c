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
 *
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
 * $Id: patricia.c 6811 2009-07-06 20:41:10Z robin $
 * Dave Plonka <plonka@doit.wisc.edu>
 *
 * This product includes software developed by the University of Michigan,
 * Merit Network, Inc., and their contributors. 
 *
 * This file had been called "radix.c" in the MRT sources.
 *
 * I renamed it to "patricia.c" since it's not an implementation of a general
 * radix trie.  Also I pulled in various requirements from "prefix.c" and
 * "demo.c" so that it could be used as a standalone API.
 */

/*
 * C23 modernization notes (2026) - see patricia.h for the header-side notes.
 * macro HAVE_IPV6 has been dropped.
 *
 * Dropped some non-core utilities. 
 * Either unused (hopefully) or should be moved to demo.c
 *  - make_and_lookup()
 *  - try_search_exact()
 *  - ascii2prefix()
 *  - my_inet_pton()
 *  - patricia_node_t change (following PyTricia) uses
 *      - prefix_t prefix (original uses prefix_t *prefix)
 *      - simplifies memory management
 *      - Ref_Prefix()
 *      - Deref_Prefix()
 *  - patricia_walk_inorder()
 *
 * Real bugs fixed along the way, not just style:
 *  - Every calloc() call site was previously used with no NULL/OOM check
 *    before immediate dereference (New_Patricia, New_Prefix, and all three
 *    call sites in patricia_lookup). All now check and propagate failure
 *    (nullptr/0, matching each function's existing failure convention)
 *    instead of crashing on allocation failure.
 *  - comp_with_mask()'s `(-1) << n` is undefined behavior in C (left-shift
 *    of a negative value). Rewritten using unsigned arithmetic to produce
 *    the identical bit pattern without UB.
 *  - prefix_toa2x()'s doc comment claims "thread safe", but the actual
 *    compiled branch (the THREAD_SPECIFIC_DATA branch is #if 0'd out) uses
 *    a plain `static` buffer shared across all threads - not thread safe
 *    as shipped. Fixed - is now thread safe.
 *    zero behavior change for single-threaded callers.
 *  - Dropped the `#define Delete free` indirection in favor of calling
 *    free() directly - it added a layer of indirection with no benefit and
 *    was internal to this file only.
 */

#define COPYRIGHT \
"This product includes software developed by the University of Michigan, Merit "\
"Network, Inc., and their contributors."

#include <arpa/inet.h> 
#include <assert.h> 
#include <ctype.h> 
#include <errno.h>
#include <netinet/in.h>
#include <stdarg.h> 
#include <stddef.h> 
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h>

#include "patricia.h"

enum {
    MAX_THREADS = 16U,
    IPV4_BITS = 32U,
    IPV6_BITS = 128U,
    NET_STR_LEN = 53U,            // INET6_ADDRSTRLEN + 2 + 5 ipv6 (ipv4 is smaller) - 53 to match orig
};

/* { from prefix.c */

/*
 * Debug prints. Activate by #define PATRICIA_DEBUG and uncomment //deb_print
 * #define PATRICIA_DEBUG to activate 
 * There is 5-6% overhead if deb_print is not completely invisible to compiler
 */
#ifdef PATRICIA_DEBUG
    #define deb_print(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif


/* 
 * prefix_tochar
 * convert prefix information to bytes
 */
uint8_t *prefix_tochar(prefix_t *prefix) {
    if (!prefix) {
        return nullptr;
    }
    return (uint8_t *)&prefix->add.sin;
}

int comp_with_mask(const void *addr, const void *dest, unsigned int mask) {

    if (memcmp(addr, dest, mask / 8) == 0) {
        unsigned int n = mask / 8;
        /*
         * Build the same "all 1 bits, shifted left" pattern as the
         * original `(-1) << (8 - (mask % 8))`, but via unsigned
         * arithmetic - left-shifting a negative value is undefined
         * behavior in C, even though it happens to work as intended
         * under most compilers' 2's complement extension.
         */
        unsigned int m = ~0U << (8 - (mask % 8));

        if (mask % 8 == 0 || (((const uint8_t *)addr)[n] & m) == (((const uint8_t *)dest)[n] & m)) {
            return 1;
        }
    }
    return 0;
}


/* 
 * convert prefix information to ascii string with length
 * thread safe and (almost) re-entrant implementation
 */
/**
 * Transpiles a binary network prefix layout into a human-readable string.
 *
 * Converts structural network prefix parameters back into standard ASCII text
 * string notation representations (e.g., "192.168.1.0/24").
 *
 * :param prefix: Pointer to the source network prefix block structure.
 * :param buff: Pointer to a destination character array buffer. Must be 
 *              adequately sized to handle string overflows.
 *              with_len (int): A boolean flag (1 or 0) toggling whether the subnet mask 
 *              suffix (like "/24") is appended to the string.
 * :returns: Pointer to the destination string buffer (`buff`) containing 
 *            the formatted ASCII data layout.
 */
static char *prefix_toa2x_ipv4(prefix_t *prefix, char *buff, int with_len) {
    //assert(prefix->bitlen <= IPV4_BITS);

    char *r = nullptr;
    r = (char *)inet_ntop(AF_INET, &prefix->add.sin, buff, INET_ADDRSTRLEN);

    if (r && with_len) {
        assert(prefix->bitlen <= IPV4_BITS);
        size_t used = (size_t)strlen(buff);
        size_t buflen = NET_STR_LEN - used;

        int ret = snprintf(buff + used, buflen, "/%d", prefix->bitlen);
        if (ret < 0 || ret > (int)buflen) {
            return nullptr;
        }
    }
    return buff;
}

static char *prefix_toa2x_ipv6(prefix_t *prefix, char *buff, int with_len) {
    char *r = nullptr;
    r = (char *)inet_ntop(AF_INET6, &prefix->add.sin6, buff, INET6_ADDRSTRLEN);

    if (r && with_len) {
        assert(prefix->bitlen <= IPV6_BITS);

        size_t used =  (size_t)strlen(buff);
        size_t bufflen = NET_STR_LEN - used;
        int ret = snprintf(buff + used, bufflen, "/%d", prefix->bitlen);
        if (ret < 0 || ret > (int)bufflen) {
            return nullptr;
        }
    }
    return buff;
}

char *prefix_toa2x(prefix_t *prefix, char *buff, int with_len) {

    if (!prefix) {
        return "(Null)";
    }

    assert(prefix->ref_count >= 0);

    if (!buff) {

        struct buffer {
            char buffs[MAX_THREADS][NET_STR_LEN];
            unsigned int i;
        };

        /*
         * static thread_local: 
         * each thread gets its own independent rotating set of 16 buffers. 
         * The original had a #if 0'd out THREAD_SPECIFIC_DATA branch and a 
         * plain `static` fallback that was NOT actually thread safe despite 
         * this function's comment.
         * thread_local (a C23 keyword) makes it now work and with
         * no behavior change for * single-threaded callers.
         */
        static thread_local struct buffer local_buff;

        buff = local_buff.buffs[local_buff.i++ % MAX_THREADS];
    }

    if (prefix->family == AF_INET) {
        return prefix_toa2x_ipv4(prefix, buff, with_len);
        /*
        assert(prefix->bitlen <= IPV4_BITS);

        char *r = nullptr;
        r = (char *)inet_ntop(AF_INET, &prefix->add.sin, buff, INET_ADDRSTRLEN);

        if (r && with_len) {
            assert(prefix->bitlen <= IPV4_BITS);

            size_t used = (size_t)strlen(buff);
            size_t buflen = NET_STR_LEN - used;

            int ret = snprintf(buff + used, buflen, "/%d", prefix->bitlen);
            if (ret < 0 || ret > (int)buflen) {
                return nullptr;
            }
        }
        return buff;
        */
    }

    if (prefix->family == AF_INET6) {
        return prefix_toa2x_ipv6(prefix, buff, with_len);
        /*
        char *r = nullptr;
        r = (char *)inet_ntop(AF_INET6, &prefix->add.sin6, buff, INET6_ADDRSTRLEN);

        if (r && with_len) {
            assert(prefix->bitlen <= IPV6_BITS);

            size_t used =  (size_t)strlen(buff);
            size_t bufflen = NET_STR_LEN - used;
            int ret = snprintf(buff + used, bufflen, "/%d", prefix->bitlen);
            if (ret < 0 || ret > (int)bufflen) {
                return nullptr;
            }
        }
        return buff;
        */
    }

    return nullptr;
}

/* prefix_toa2
 * convert prefix information to ascii string
 */
char *prefix_toa2(prefix_t *prefix, char *buff) {
    return prefix_toa2x(prefix, buff, 0);
}

/* 
 * prefix_toa
 */
char *prefix_toa(prefix_t *prefix) {
    return prefix_toa2(prefix, nullptr);
}

/**
 * Constructs a prefix node container from raw network parameters.
 *
 * This function translates raw binary representations of network addresses
 * into a standardized prefix block layout for the patricia tree.
 *
 * :param family: The network address family protocol
 *                (e.g., AF_INET or AF_INET6).
 * :param dest: Pointer to the raw binary network address array.
 * :param bitlen: The total routing bitmask bitlength.
 * :param prefix: Pointer to an allocated destination prefix
 *                structure where result will be saved.
 * :returns: Returns 1 on success, 0 if the bitlength exceeds limits, 
 *                   or -1 for unsupported address families.
 */
prefix_t *New_Prefix(int family, void *dest, int bitlen, prefix_t *prefix) {
    int dynamic_allocated = 0;
    int default_bitlen = 32;
    prefix_t *prefix_new = nullptr;

    if (!prefix) {
        prefix_new = calloc(1, sizeof(prefix_t));
        if (!prefix_new) {
            return nullptr;
        }
        prefix = prefix_new;
        dynamic_allocated++;
    }

    if (family == AF_INET6) {
        default_bitlen = IPV6_BITS;
        memcpy(&prefix->add.sin6, dest, 16);

    } else if (family == AF_INET) {
        memcpy(&prefix->add.sin, dest, 4);

    } else {
        if (prefix_new) {
            free((void *)prefix_new);
        }
        return nullptr;
    }

    prefix->bitlen = (uint8_t)((bitlen >= 0) ? (uint8_t)bitlen : (uint8_t)default_bitlen);
    prefix->family = (sa_family_t)family;
    prefix->ref_count = 0;

    if (dynamic_allocated) {
        prefix->ref_count++;
    }
    /* 
     * fprintf(stderr, "[C %s, %d]\n", prefix_toa (prefix), prefix->ref_count); 
     */
    return prefix;
}

/* } */


static int num_active_patricia = 0;      // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/* these routines support continuous mask only */

/**
 * Allocates and initializes a new Patricia Trie structure.
 *
 * This function builds the base routing tree tracking structure, establishing
 * the maximum depth limit of the radix bit-testing operations.
 *
 * :param maxbits: Maximum bitlength allowable for keys in this
 *                 trie (e.g., 32 for IPv4, 128 for IPv6).
 * :returns: Pointer to the newly allocated and initialized
 *           Patricia tree structure, or nullptr if memory allocation fails.
 */
patricia_tree_t *New_Patricia(int maxbits) {
    patricia_tree_t *patricia = calloc(1, sizeof(*patricia));

    if (!patricia) {
        return nullptr;
    }

    /*
     * Is this public - if not change to IPV6_BITS
     */
    assert(maxbits <= PATRICIA_MAXBITS);
    patricia->maxbits = (unsigned int)maxbits;
    patricia->head = nullptr;
    patricia->num_active_node = 0;
    patricia->frozen = 0;
    num_active_patricia++;
    return patricia;
}


/*
 * if func is supplied, it will be called as func(node->data)
 * before deleting the node
 */
/**
 * Deallocates all node elements inside the tree matrix while keeping the root intact.
 *
 * This function walks the entire tree layout structure using an iterative post-order traversal stack.
 * It detaches all child nodes and clears their internal payload properties. Unlike Destroy_Patricia,
 * this function leaves the top-level base tree pointer structure initialized and ready for reuse.
 *
 * :param patricia: Pointer to the base Patricia tree object to purge.
 * :param func: optional user data deallocation callback function pointer.
 *              If provided, the engine will systematically execute `func(node->data)`
 *              on every payload container encountered before destroying the node itself.
 *              Pass nullptr if your node data properties do not require custom memory managerment hooks.
 */
/*
 * NOLINTBEGIN(readability-function-cognitive-complexity)
 */
void Clear_Patricia(patricia_tree_t *patricia, void_fn_t func) {

    assert(patricia);

    if (patricia->head) {

        patricia_node_t *Xstack[PATRICIA_MAXBITS + 1];
        patricia_node_t **Xsp = Xstack;
        patricia_node_t *Xrn = patricia->head;

        while (Xrn) {
            patricia_node_t *l = Xrn->l;
            patricia_node_t *r = Xrn->r;

            if (Xrn->data) {
                if (func) {
                    func(Xrn->data);
                }

            } else {
                assert(!Xrn->data);
            }

            if (!patricia->frozen) {
                free(Xrn);
            }
            patricia->num_active_node--;

            if (l) {
                if (r) {
                    *Xsp++ = r;
                }
                Xrn = l;

            } else if (r) {
                Xrn = r;

            } else if (Xsp != Xstack) {
                Xrn = *(--Xsp);

            } else {
                Xrn = nullptr;
            }
        }
    }
    assert(patricia->num_active_node == 0);
    if (patricia->frozen && patricia->head) {
        free(patricia->head);
    }
}
/*
 * NOLINTEND(readability-function-cognitive-complexity)
 */


/**
 * Deallocates and wipes an entire Patricia Trie system memory layout.
 *
 * This function performs a recursive post-order tree-walk, freeing every node.
 * It permits an optional custom callback to securely purge node data payloads.
 *
 * :param patricia: Pointer to the Patricia tree system to wipe.
 * :func: An optional custom data-cleanup function pointer to
 *        execute on each node's user data value, or nullptr to skip data cleanup.
 */
void Destroy_Patricia(patricia_tree_t *patricia, void_fn_t func) {
    Clear_Patricia(patricia, func);
    free(patricia);
    num_active_patricia--;
}


/*
 * if func is supplied, it will be called as func(&node->prefix, node->data)
 */
void patricia_process(patricia_tree_t *patricia, void_fn_2_t func) {
    assert(func);

    if (!patricia->head) {
        return;
    }

    /*
     * Same iterative pre-order walk as Clear_Patricia(), inlined here
     * instead of going through the old PATRICIA_WALK/PATRICIA_WALK_END
     * macro pair (removed from patricia.h - this was their only caller).
     * Only visits nodes that carry data, matching the original macro's
     * `if (Xnode->data)` guard.
     */
    patricia_node_t *stack[PATRICIA_MAXBITS + 1];
    patricia_node_t **sp = stack;
    patricia_node_t *node = patricia->head;

    while (node) {
        if (node->data) {
            func(&node->prefix, node->data);
        }

        if (node->l) {
            if (node->r) {
                *sp++ = node->r;
            }
            node = node->l;

        } else if (node->r) {
            node = node->r;

        } else if (sp != stack) {
            node = *(--sp);

        } else {
            node = nullptr;
        }
    }
}


/**
 * Searches the Patricia Trie for an exact prefix match.
 *
 * This function performs a strict bitwise traversal down the radix tree matrix
 * to locate a node that matches the provided network prefix configuration exactly.
 *
 * :param patricia: Pointer to the base initialized Patricia Trie structure.
 * :param prefix: The specific IP prefix mask to query.
 * :returns: Pointer to the matching node structure if found, or
 *           nullptr if target prefix does not exist in the tree.
 */
patricia_node_t *patricia_search_exact(patricia_tree_t *patricia, prefix_t *prefix) {
    patricia_node_t *node = nullptr;
    uint8_t *addr = nullptr;
    unsigned int bitlen = 0;

    assert(patricia);
    assert(prefix);
    assert(prefix->bitlen <= patricia->maxbits);

    if (!patricia->head) {
        return nullptr;
    }

    node = patricia->head;
    addr = prefix_tochar(prefix);
    bitlen = prefix->bitlen;

    while (node->bit < bitlen) {

        if (bit_test(addr[node->bit >> 3U], 0x80U >> (node->bit & 0x07U))) {

#ifdef PATRICIA_DEBUG
            if (node->data) {
                deb_print("patricia_search_exact: take right %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);
            } else {
                dept_print("patricia_search_exact: take right at %d\n", node->bit);
            }
#endif /* PATRICIA_DEBUG */

            node = node->r;

        } else {

#ifdef PATRICIA_DEBUG
            if (node->data) {
                deb_print("patricia_search_exact: take left %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);
            } else {
                deb_print("patricia_search_exact: take left at %d\n", node->bit);
            }
#endif /* PATRICIA_DEBUG */

            node = node->l;
        }

        if (!node) {
            return nullptr;
        }
    }

    //deb_print("patricia_search_exact: stop at %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

    if (node->bit > bitlen || !node->data) {
        return nullptr;
    }

    assert(node->bit == bitlen);
    assert(node->bit == node->prefix.bitlen);
    if (comp_with_mask(prefix_tochar(&node->prefix), prefix_tochar(prefix), bitlen)) {

        //deb_print("patricia_search_exact: found %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

        return node;
    }
    return nullptr;
}


/* if inclusive != 0, "best" may be the given prefix itself */
patricia_node_t *patricia_search_best2(patricia_tree_t *patricia, prefix_t *prefix, int inclusive) {
    patricia_node_t *node = nullptr;
    patricia_node_t *stack[PATRICIA_MAXBITS + 1];
    uint8_t *addr = nullptr;
    unsigned int bitlen = 0;
    int cnt = 0;

    assert(patricia);
    assert(prefix);
    assert(prefix->bitlen <= patricia->maxbits);

    if (!patricia->head) {
        return nullptr;
    }

    node = patricia->head;
    addr = prefix_tochar(prefix);
    bitlen = prefix->bitlen;

    while (node->bit < bitlen) {

        if (node->data) {
            //deb_print("patricia_search_best: push %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);
            stack[cnt++] = node;
        }

        if (bit_test(addr[node->bit >> 3U], 0x80U >> (node->bit & 0x07U))) {

            //deb_print("patricia_search_best: take right %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            node = node->r;
        } else {

            //deb_print("patricia_search_best: take left %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            node = node->l;
        }

        if (!node) {
            break;
        }
    }

    if (inclusive && node && node->data && node->bit <= bitlen) {
        stack[cnt++] = node;
    }

#ifdef PATRICIA_DEBUG
    if (!node) {
        deb_print("patricia_search_best: stop at null\n");
    } else {
        deb_print("patricia_search_best: stop at %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);
    }
#endif /* PATRICIA_DEBUG */

    if (cnt <= 0) {
        return nullptr;
    }

    while (--cnt >= 0) {
        node = stack[cnt];

        //deb_print("patricia_search_best: pop %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

        if (comp_with_mask(prefix_tochar(&node->prefix),
                prefix_tochar(prefix),
                node->prefix.bitlen)) {

            //deb_print("patricia_search_best: found %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            return node;
        }
    }
    return nullptr;
}


/**
 * Locates the Longest Prefix Match (LPM) for a given network query.
 *
 * This function performs an un-mutating longest-match radix search. It maps
 * specific host IP queries back to their broadest covering subnet masks.
 *
 * :param patricia: Pointer to the base Patricia tree structure.
 * :prefix: Pointer to the target prefix/IP to look up.
 * :returns: Pointer to the most specific matching node 
 *           covering the target address, or nullptr if not found
 */
patricia_node_t *patricia_search_best(patricia_tree_t *patricia, prefix_t *prefix) {
    return patricia_search_best2(patricia, prefix, 1);
}


/**
 * Searches for a network prefix or inserts it if it is missing.
 *
 *  This is the primary mutating driver of the radix trie. It traverses the bit 
 *  index paths; if an exact match is missing, a new payload node is created.
 *
 * :param patricia: Pointer to the base Patricia tree structure.
 * :prefix: Pointer to the target network prefix layout.
 * :returns: Pointer to the newly created or pre-existing matching
 *           node within the tree matrix, or nullptr if allocation fails.
 */
/*
 * NOLINTBEGIN(readability-function-cognitive-complexity)
 */
patricia_node_t *patricia_lookup(patricia_tree_t *patricia, prefix_t *prefix) {
    patricia_node_t *node = nullptr;
    patricia_node_t *new_node = nullptr;
    patricia_node_t *parent = nullptr;
    patricia_node_t *glue = nullptr;

    uint8_t *addr = nullptr;
    uint8_t *test_addr = nullptr;
    unsigned int bitlen = 0;
    unsigned int check_bit = 0;
    unsigned int differ_bit = 0;

    assert(patricia);
    assert(prefix);
    assert(prefix->bitlen <= patricia->maxbits);

    if (!patricia->head) {

        node = calloc(1, sizeof(*node));
        if (!node) {
            return nullptr;
        }

        node->bit = prefix->bitlen;
        node->prefix = *prefix;
        node->parent = nullptr;
        node->l = node->r = nullptr;
        node->data = nullptr;
        patricia->head = node;


        //deb_print("patricia_lookup: new_node #0 %s/%d (head)\n", prefix_toa(prefix), prefix->bitlen);

        patricia->num_active_node++;
        return node;
    }

    addr = prefix_tochar(prefix);
    bitlen = prefix->bitlen;
    node = patricia->head;

    while (node->bit < bitlen || !node->data) {

        if (node->bit < patricia->maxbits && bit_test(addr[node->bit >> 3U], 0x80U >> (node->bit & 0x07U))) {

            if (!node->r) {
                break;
            }

            //deb_print("patricia_lookup: take right %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            node = node->r;

        } else {
            if (!node->l) {
                break;
            }

            //deb_print("patricia_lookup: take left %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            node = node->l;
        }

        assert(node);
    }

    assert(node->data);

    //deb_print("patricia_lookup: stop at %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);


    test_addr = prefix_tochar(&(node->prefix));
    /* find the first bit different */
    check_bit = (node->bit < bitlen) ? node->bit : bitlen;
    differ_bit = 0;

    for (unsigned int i = 0; i * 8 < check_bit; i++) {
        int r = addr[i] ^ test_addr[i];
        if (r == 0) {
            differ_bit = (i + 1) * 8;
            continue;
        }
        /* I know the better way, but for now */
        int j = 0;
        for (j = 0; j < 8; j++) {
            if (bit_test((unsigned int)r, (0x80U >> (unsigned int)j))) {
                break;
            }
        }
        /* must be found */
        assert(j < 8);
        differ_bit = (i * 8) + (unsigned int)j;
        break;
    }

    if (differ_bit > check_bit) {
        differ_bit = check_bit;
    }

    //deb_print("patricia_lookup: differ_bit %d\n", differ_bit);

    parent = node->parent;
    while (parent && parent->bit >= differ_bit) {
        node = parent;
        parent = node->parent;

        //deb_print("patricia_lookup: up to %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

    }

    if (differ_bit == bitlen && node->bit == bitlen) {
        if (node->data) {

            //deb_print("patricia_lookup: found %s/%d\n", prefix_toa(&node->prefix), node->prefix.bitlen);

            return node;
        }
        node->prefix = *prefix;

        //deb_print("patricia_lookup: new node #1 %s/%d (glue mod)\n", prefix_toa(prefix), prefix->bitlen);

        assert(!node->data);
        return node;
    }

    new_node = calloc(1, sizeof(*new_node));
    if (!new_node) {
        return nullptr;
    }
    new_node->bit = prefix->bitlen;
    new_node->prefix = *prefix;
    new_node->parent = nullptr;
    new_node->l = new_node->r = nullptr;
    new_node->data = nullptr;
    patricia->num_active_node++;

    if (node->bit == differ_bit) {
        new_node->parent = node;
        if (node->bit < patricia->maxbits && bit_test(addr[node->bit >> 3U], 0x80U >> (node->bit & 0x07U))) {
            assert(!node->r);
            node->r = new_node;
        } else {
            assert(!node->l);
            node->l = new_node;
        }

        //deb_print("patricia_lookup: new_node #2 %s/%d (child)\n", prefix_toa(prefix), prefix->bitlen);

        return new_node;
    }

    if (bitlen == differ_bit) {
        if (bitlen < patricia->maxbits && bit_test(test_addr[bitlen >> 3U], 0x80U >> (bitlen & 0x07U))) {
            new_node->r = node;
        } else {
            new_node->l = node;
        }

        new_node->parent = node->parent;

        if (!node->parent) {
            assert(patricia->head == node);
            patricia->head = new_node;

        } else if (node->parent->r == node) {
            node->parent->r = new_node;

        } else {
            node->parent->l = new_node;
        }

        node->parent = new_node;

        //deb_print("patricia_lookup: new_node #3 %s/%d (parent)\n", prefix_toa(prefix), prefix->bitlen);

    } else {
        glue = calloc(1, sizeof(*glue));
        if (!glue) {
            /*
             * new_node was already allocated and counted above but
             * never linked into the tree - undo both before bailing,
             * or we'd leak new_node and leave num_active_node wrong.
             */
            free(new_node);
            patricia->num_active_node--;
            return nullptr;
        }

        glue->bit = differ_bit;
        memset(&glue->prefix, 0, sizeof(prefix_t));
        glue->parent = node->parent;
        glue->data = nullptr;

        patricia->num_active_node++;
        if (differ_bit < patricia->maxbits && bit_test(addr[differ_bit >> 3U], 0x80U >> (differ_bit & 0x07U))) {
            glue->r = new_node;
            glue->l = node;

        } else {
            glue->r = node;
            glue->l = new_node;
        }

        new_node->parent = glue;

        if (!node->parent) {
            assert(patricia->head == node);
            patricia->head = glue;

        } else if (node->parent->r == node) {
            node->parent->r = glue;

        } else {
            node->parent->l = glue;
        }

        node->parent = glue;

        //deb_print("patricia_lookup: new_node #4 %s/%d (glue+node)\n", prefix_toa(prefix), prefix->bitlen);

    }
    return new_node;
}
/*
 * NOLINTEND(readability-function-cognitive-complexity)
 */


/**
 * Extricates an isolated target node container from the Patricia tree network.
 *
 * Removes a specific node structural point, rearranging internal branch pointer
 * paths to preserve radix trie sorting and lookup logic.
 *
 * :param patricia: Pointer to the parent Patricia tree structure.
 * :param node: Pointer to the specific node container to
 *              remove from the tree array layout.
 */
/*
 * NOLINTBEGIN(readability-function-cognitive-complexity)
 */
void patricia_remove(patricia_tree_t *patricia, patricia_node_t *node) {
    patricia_node_t *parent = nullptr;
    patricia_node_t *child = nullptr;;

    assert(patricia);
    assert(node);

    if (node->r && node->l) {

        //deb_print("patricia_remove: #0 %s/%d (r & l)\n", prefix_toa(&node->prefix), node->prefix.bitlen);

        /* Also I needed to clear data pointer -- masaki */
        node->data = nullptr;
        return;
    }

    if (!node->r && !node->l ) {

        //deb_print("patricia_remove: #1 %s/%d (!r & !l)\n", prefix_toa(&node->prefix), node->prefix.bitlen);

        parent = node->parent;
        free(node);
        patricia->num_active_node--;

        if (!parent) {
            assert(patricia->head == node);
            patricia->head = nullptr;
            return;
        }

        if (parent->r == node) {
            parent->r = nullptr;
            child = parent->l;
        } else {
            assert(parent->l == node);
            parent->l = nullptr;
            child = parent->r;
        }

        if (parent->data) {
            return;
        }

        /* we need to remove parent too */

        if (!parent->parent) {
            assert(patricia->head == parent);
            patricia->head = child;
        } else if (parent->parent->r == parent) {
            parent->parent->r = child;
        } else {
            assert(parent->parent->l == parent);
            parent->parent->l = child;
        }
        child->parent = parent->parent;
        free(parent);
        patricia->num_active_node--;
        return;
    }

    //deb_print("patricia_remove: #2 %s/%d (r ^ l)\n", prefix_toa(&node->prefix), node->prefix.bitlen);

    if (node->r) {
        child = node->r;

    } else {
        assert(node->l);
        child = node->l;
    }

    parent = node->parent;
    child->parent = parent;

    free(node);
    patricia->num_active_node--;

    if (!parent) {
        assert(patricia->head == node);
        patricia->head = child;
        return;
    }

    if (parent->r == node) {
        parent->r = child;

    } else {
        assert(parent->l == node);
        parent->l = child;
    }
}
/*
 * NOLINTEND(readability-function-cognitive-complexity)
 */
