#
# SPDX-License-Identifier: (LGPL-3.0-or-later AND UMich-Merit)
#
# Copyright (c) 1997, 1998, 1999 Dave Plonka <plonka@doit.wisc.edu>
#
# $Id: COPYRIGHT,v 1.1.1.1 2013/08/15 18:46:09 labovit Exp $
#
# Copyright (c) 1999-2013
#
# The Regents of the University of Michigan ("The Regents") and Merit
# Network, Inc.
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Modifications Copyright (c) 2026 Gene C <arch@sapience.com>
#
# These modifications and additions are free software; you can redistribute them and/or
# modify them under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 3 of
# the License, or (at your option) any later version.
#
# cython: boundscheck=False
# cython: wraparound=False
# cython: initializedcheck=False
# patricia26.pyx - High-Speed String/CIDR Engine

from libc.string cimport strchr, memcpy
from libc.stdlib cimport atoi
from libc.stdint cimport uint8_t

from cpython.ref cimport Py_INCREF, Py_DECREF

cdef extern from "<sys/socket.h>" nogil:
    int AF_INET
    int AF_INET6

cdef extern from "Python.h":
    const char* PyUnicode_AsUTF8AndSize(object obj, Py_ssize_t* size) except NULL

cdef extern from "<arpa/inet.h>" nogil:
    int inet_pton(int af, const char* src, void* dst)
    unsigned int htonl(unsigned int hostlong)

cdef extern from "patricia.h":
    ctypedef unsigned short sa_family_t
    ctypedef struct prefix_t:
        sa_family_t family
        uint8_t bitlen
        uint8_t pad
        int ref_count
        void* add

    ctypedef struct patricia_node_t:
        prefix_t prefix
        void* data
        patricia_node_t* l
        patricia_node_t* r
        patricia_node_t* parent

    ctypedef struct patricia_tree_t:
        unsigned int maxbits
        int num_active_node
        int frozen
        patricia_node_t* head

    patricia_tree_t* New_Patricia(int maxbits) nogil
    void Destroy_Patricia(patricia_tree_t* patricia, void (*data_free)(void*) noexcept nogil) nogil
    patricia_node_t* patricia_lookup(patricia_tree_t* patricia, prefix_t* prefix) nogil
    patricia_node_t* patricia_search_best(patricia_tree_t* patricia, prefix_t* prefix) nogil
    patricia_node_t* patricia_search_exact(patricia_tree_t* patricia, prefix_t* prefix) nogil
    void patricia_remove(patricia_tree_t* patricia, patricia_node_t* node) nogil
    char* prefix_toa2x(prefix_t* prefix, char* buff, int with_len) nogil

cdef void dec_python_ref(void* data) noexcept with gil:
    if data != NULL:
        Py_DECREF(<object>data)

cdef extern from "<unistd.h>" nogil:
    long _SC_NPROCESSORS_ONLN
    long sysconf(int name)

cdef class Patricia26:
    cdef patricia_tree_t* _tree_v4
    cdef patricia_tree_t* _tree_v6

    def __cinit__(self):
        self._tree_v4 = New_Patricia(32)
        self._tree_v6 = New_Patricia(128)
        if self._tree_v4 == NULL or self._tree_v6 == NULL:
            raise MemoryError("Unable to instantiate internal dual-tree storage matrices.")

    def __dealloc__(self):
        if self._tree_v4 != NULL:
            Destroy_Patricia(self._tree_v4, dec_python_ref)

        if self._tree_v6 != NULL:
            Destroy_Patricia(self._tree_v6, dec_python_ref)

    def num_prefixes_v4(self):
        return self._tree_v4.num_active_node

    def num_prefixes_v6(self):
        return self._tree_v6.num_active_node

    def __len__(self):
        return self._tree_v4.num_active_node + self._tree_v6.num_active_node

    def freeze(self):
        """Freezes both underlying C routing trees using direct integer flags."""
        if self._tree_v4 != NULL:
            self._tree_v4.frozen = <int>1
        if self._tree_v6 != NULL:
            self._tree_v6.frozen = <int>1

    def thaw(self):
        """Thaws both underlying C routing trees to permit modification loops."""
        if self._tree_v4 != NULL:
            self._tree_v4.frozen = <int>0
        if self._tree_v6 != NULL:
            self._tree_v6.frozen = <int>0

    cdef inline str _prefix_to_str(self, prefix_t* prefix):
        cdef char buf[64]
        cdef char* c_str = prefix_toa2x(prefix, buf, 1)

        if c_str == NULL:
            return ""

        return c_str.decode('utf-8')

    cdef inline patricia_node_t* _parse_and_find(self, object key, bint exact) noexcept:
        cdef prefix_t prefix
        cdef Py_ssize_t string_size

        # Standardize input to a string
        if not isinstance(key, str):
            key = str(key)

        cdef const char* p = PyUnicode_AsUTF8AndSize(key, &string_size)
        if string_size == 0 or string_size >= 64:
            return NULL

        # Address family 
        cdef bint is_v6 = (strchr(p, b':') != NULL)
        cdef int max_bits = 128 if is_v6 else 32
        cdef int bitlen = max_bits

        # Safe stack memory replication and null-termination
        cdef char c_buf[64]
        memcpy(c_buf, p, string_size)
        c_buf[string_size] = 0

        # Extract and validate CIDR bit length segment
        cdef char* slash_pos = strchr(c_buf, b'/')
        cdef char* mask_str

        if slash_pos != NULL:
            slash_pos[0] = 0  # Truncate string to isolate raw IP address
            mask_str = slash_pos + 1

            # Guard : Trap empty trailing slash strings like "10.0.0.0/"
            if mask_str[0] == 0:
                return NULL

            # validation: Ensure every character following the slash is a pure numeric digit
            # (prevents malicious or malformed alpha-strings from defaulting to 0)
            while mask_str[0] != 0:
                if not (b'0' <= <unsigned char>mask_str[0] <= b'9'):
                    return NULL
                mask_str += 1

            # convert validated numeric text block to an integer
            bitlen = atoi(slash_pos + 1)

        # network translation and mask checks
        if bitlen > max_bits:
            return NULL

        if is_v6:
            if inet_pton(AF_INET6, c_buf, <void*>&prefix.add) <= 0:
                return NULL
            prefix.family = AF_INET6
            prefix.bitlen = bitlen
            if exact or bitlen < 128:
                return patricia_search_exact(self._tree_v6, &prefix)
            return patricia_search_best(self._tree_v6, &prefix)
        else:
            if inet_pton(AF_INET, c_buf, <void*>&prefix.add) <= 0:
                return NULL
            prefix.family = AF_INET
            prefix.bitlen = bitlen
            if exact or bitlen < 32:
                return patricia_search_exact(self._tree_v4, &prefix)
            return patricia_search_best(self._tree_v4, &prefix)

    def __getitem__(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)

        if node == NULL or node.data == NULL:
            return None
        return <object>node.data

    def __contains__(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)
        return node != NULL and node.data != NULL

    def has_prefix(self, str prefix_str):
        """Returns True ONLY if the exact CIDR prefix length exists with active user data."""
        # Check node and node.data to protect against internal structural placeholders
        cdef patricia_node_t* node = self._parse_and_find(prefix_str, True)
        return node != NULL and node.data != NULL

    def has_key(self, str prefix_str):
        return self.has_prefix(prefix_str)

    def get_key(self, str prefix_str):
        return self.get_prefix(prefix_str)

    def lookup(self, str ip_str):
        """Returns the object value mapped to the best matching subnet without throwing standard KeyErrors."""
        return self[ip_str]

    def get_prefix(self, str prefix_str):
        """Returns the exact best matching routing key string found in the trie matrix."""
        cdef patricia_node_t* node = self._parse_and_find(prefix_str, False)
        if node == NULL or node.data == NULL:
            return None
        return self._prefix_to_str(&node.prefix)

    def parent(self, str prefix_str):
        """Finds the immediate operational parent block container encompassing this prefix."""
        cdef patricia_node_t* node = self._parse_and_find(prefix_str, True)
        cdef patricia_node_t* curr

        if node == NULL:
            return None

        curr = node.parent
        while curr != NULL:
            if curr.data != NULL:
                return self._prefix_to_str(&curr.prefix)
            curr = curr.parent
        return None

    def children(self, str prefix_str):
        """Returns a list of all active subnets enclosed directly under this prefix block."""
        cdef patricia_node_t* root_node
        cdef patricia_node_t* curr
        cdef patricia_node_t* stack[129]
        cdef list child_prefixes = []
        cdef int sp = 0

        # Find the starting root block exactly
        root_node = self._parse_and_find(prefix_str, True)
        if root_node == NULL:
            return []

        # If the target root node has no sub-branches, it has no children
        if root_node.l == NULL and root_node.r == NULL:
            return []

        # Seed the stack using only the immediate children of the root node
        if root_node.l != NULL:
            stack[sp] = root_node.l
            sp += 1

        if root_node.r != NULL:
            stack[sp] = root_node.r
            sp += 1

        # Process the nested sub-tree branches using pointer descent
        while sp > 0:
            sp -= 1
            curr = stack[sp]
            while curr != NULL:
                # If we encounter an active tracking entry, collect it
                if curr.data != NULL:
                    child_prefixes.append(self._prefix_to_str(&curr.prefix))

                # Desent down the left path, caching the right branch if present
                if curr.l != NULL:
                    if curr.r != NULL:
                        stack[sp] = curr.r
                        sp += 1
                    curr = curr.l
                elif curr.r != NULL:
                    curr = curr.r

                elif sp > 0:
                    sp -= 1
                    curr = stack[sp]

                else:
                    curr = NULL

        return child_prefixes

    def __setitem__(self, str key, object value):
        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26 tree.")

        cdef prefix_t prefix
        cdef patricia_node_t* node
        cdef Py_ssize_t string_size
        cdef const char* p = PyUnicode_AsUTF8AndSize(key, &string_size)

        if string_size == 0 or string_size >= 64:
            raise ValueError(key)

        cdef bint is_v6 = (strchr(p, b':') != NULL)
        cdef int max_bits = 128 if is_v6 else 32
        cdef int bitlen = max_bits

        cdef char c_buf[64]
        memcpy(c_buf, p, string_size)
        c_buf[string_size] = 0

        cdef char* slash_pos = strchr(c_buf, b'/')
        cdef char* mask_str

        if slash_pos != NULL:
            slash_pos[0] = 0
            mask_str = slash_pos + 1

            if mask_str[0] == 0:
                raise ValueError(key)

            while mask_str[0] != 0:
                if not (b'0' <= <unsigned char>mask_str[0] <= b'9'):
                    raise ValueError(key)
                mask_str += 1

            bitlen = atoi(slash_pos + 1)

        if bitlen > max_bits:
            raise ValueError(key)

        if is_v6:
            if inet_pton(AF_INET6, c_buf, <void*>&prefix.add) <= 0:
                raise ValueError(key)
            prefix.family = AF_INET6
            prefix.bitlen = bitlen
            node = patricia_lookup(self._tree_v6, &prefix)
        else:
            if inet_pton(AF_INET, c_buf, <void*>&prefix.add) <= 0:
                raise ValueError(key)
            prefix.family = AF_INET
            prefix.bitlen = bitlen
            node = patricia_lookup(self._tree_v4, &prefix)

        if node == NULL:
            raise RuntimeError("Unable to expand trie slot mappings.")

        if node.data != NULL:
            Py_DECREF(<object>node.data)
        Py_INCREF(value)
        node.data = <void*>value

    def iter_v4(self):
        """Yields all active v4 prefixes using a zero-allocation stack."""
        cdef patricia_node_t* stack[129]
        cdef int sp = 0
        cdef patricia_node_t* curr

        if self._tree_v4 != NULL and self._tree_v4.head != NULL:
            curr = self._tree_v4.head

            while curr != NULL:
                if curr.data != NULL:
                    yield self._prefix_to_str(&curr.prefix)

                # Push right child to stack if left child exists to follow it first
                if curr.l != NULL:
                    if curr.r != NULL:
                        stack[sp] = curr.r
                        sp += 1
                    curr = curr.l

                elif curr.r != NULL:
                    curr = curr.r

                elif sp > 0:
                    sp -= 1
                    curr = stack[sp]

                else:
                    curr = NULL

    def iter_v6(self):
        """Yields all active v6 prefixes using a zero-allocation stack."""
        cdef patricia_node_t* stack[129]
        cdef int sp = 0
        cdef patricia_node_t* curr

        if self._tree_v6 != NULL and self._tree_v6.head != NULL:
            curr = self._tree_v6.head

            while curr != NULL:
                if curr.data != NULL:
                    yield self._prefix_to_str(&curr.prefix)

                if curr.l != NULL:
                    if curr.r != NULL:
                        stack[sp] = curr.r
                        sp += 1
                    curr = curr.l

                elif curr.r != NULL:
                    curr = curr.r

                elif sp > 0:
                    sp -= 1
                    curr = stack[sp]

                else:
                    curr = NULL

    def __iter__(self):
        """Yields all active v4 prefixes followed by all active v6 prefixes."""
        yield from self.iter_v4()
        yield from self.iter_v6()

    def __delitem__(self, object key):
        # 1. Place all C-level declarations at the absolute top of the function
        cdef patricia_node_t* node
        cdef patricia_tree_t* target_tree

        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26 tree.")

        # 2. Force an exact-match search look up
        node = self._parse_and_find(key, True)

        # 3. Wrap ALL usage of the node pointer inside the non-NULL safety block
        if node != NULL and node.data != NULL:
            Py_DECREF(<object>node.data)
            node.data = NULL

            # Safe branch lookup: node is guaranteed to exist here
            target_tree = self._tree_v6 if node.prefix.family == 10 else self._tree_v4
            patricia_remove(target_tree, node)

    def lookup_lpm(self, str ip_str):
        """Returns single-pass tuple of (matched_cidr_str, node_data)."""
        cdef prefix_t prefix
        cdef patricia_node_t* node = NULL
        cdef Py_ssize_t string_size = 0
        cdef const char* p = NULL
        cdef bint is_v6 = False
        cdef int max_bits = 0
        cdef int bitlen = 0
        cdef char* slash_pos = NULL
        cdef char* mask_str = NULL
        cdef char c_buf[64]

        try:
            p = PyUnicode_AsUTF8AndSize(ip_str, &string_size)
            if string_size == 0 or string_size >= 64:
                return (None, None)

            is_v6 = (strchr(p, b':') != NULL)
            max_bits = 128 if is_v6 else 32
            bitlen = max_bits

            memcpy(c_buf, p, string_size)
            c_buf[string_size] = 0

            slash_pos = strchr(c_buf, b'/')
            if slash_pos != NULL:
                slash_pos[0] = 0
                mask_str = slash_pos + 1

                if mask_str[0] == 0:
                    return (None, None)

                while mask_str[0] != 0:
                    if not (b'0' <= <unsigned char>mask_str[0] <= b'9'):
                        return (None, None)
                    mask_str += 1

                bitlen = atoi(slash_pos + 1)

            if bitlen > max_bits:
                return (None, None)

            if is_v6:
                if inet_pton(AF_INET6, c_buf, <void*>&prefix.add) <= 0:
                    return (None, None)
                prefix.family = AF_INET6
                prefix.bitlen = bitlen
                node = patricia_search_best(self._tree_v6, &prefix)
            else:
                if inet_pton(AF_INET, c_buf, <void*>&prefix.add) <= 0:
                    return (None, None)
                prefix.family = AF_INET
                prefix.bitlen = bitlen
                node = patricia_search_best(self._tree_v4, &prefix)

            if node == NULL or node.data == NULL:
                return (None, None)

            return (self._prefix_to_str(&node.prefix), <object>node.data)

        except Exception:
            return (None, None)

    def bulk_lookup(self, list ip_list):
        """Hyper-optimized batch query executing branchless, unmixed single-pass loops."""
        cdef Py_ssize_t list_len = len(ip_list)
        if list_len == 0:
            return []

        cdef prefix_t prefix
        cdef patricia_node_t* node
        cdef Py_ssize_t idx, string_size
        cdef const char* p
        cdef unsigned int v4_val
        cdef int bitlen
        cdef const char* slash_pos
        cdef char c_buf[64]

        # Allocate final list directly in a single pass
        results = []

        # Peek index 0 safely to establish the batch route
        p = PyUnicode_AsUTF8AndSize(ip_list[0], &string_size) if list_len > 0 else NULL
        cdef bint is_v6_batch = (p != NULL and strchr(p, b':') != NULL)

        if not is_v6_batch:
            # === HYPER-PERFORMANCE SINGLE-PASS IPv4 FAST-PATH ===
            for idx in range(list_len):
                p = PyUnicode_AsUTF8AndSize(ip_list[idx], &string_size)
                if string_size == 0 or string_size >= 64:
                    results.append(None)
                    continue

                # Run the inline C helper (replaces original raw loops)
                if not _parse_ipv4_inline(p, &v4_val, &bitlen):
                    results.append(None)
                    continue

                prefix.family = AF_INET
                prefix.bitlen = bitlen
                memcpy(&prefix.add, &v4_val, 4)

                node = patricia_search_best(self._tree_v4, &prefix)
                if node != NULL and node.data != NULL:
                    results.append(<object>node.data)
                else:
                    results.append(None)
        else:
            # === HYPER-PERFORMANCE SINGLE-PASS IPv6 FAST-PATH ===
            for idx in range(list_len):
                p = PyUnicode_AsUTF8AndSize(ip_list[idx], &string_size)
                if string_size == 0 or string_size >= 64:
                    results.append(None)
                    continue

                memcpy(c_buf, p, string_size)
                c_buf[string_size] = 0

                bitlen = 128
                slash_pos = strchr(c_buf, b'/')
                if slash_pos != NULL:
                    (<char*>c_buf)[slash_pos - c_buf] = 0
                    bitlen = atoi(slash_pos + 1)
                    if bitlen > 128:
                        results.append(None)
                        continue

                prefix.family = AF_INET6
                prefix.bitlen = bitlen

                if inet_pton(AF_INET6, c_buf, <void*>&prefix.add) <= 0:
                    results.append(None)
                    continue

                node = patricia_search_best(self._tree_v6, &prefix)
                if node != NULL and node.data != NULL:
                    results.append(<object>node.data)
                else:
                    results.append(None)

        return results

    def prefixes_v4(self):
        return list(self.iter_v4())

    def prefixes_v6(self):
        return list(self.iter_v6())

    def prefixes(self):
        """Returns a unified list combining all active v4 and v6 subnet tracking string keys."""
        return self.prefixes_v4() + self.prefixes_v6()

    def insert(self, object prefix_str, object val):
        """Inserts value to the LPM of an IP or CIDR block string."""
        self.__setitem__(prefix_str, val)

    def bulk_insert(self, object items):
        """Bulk inserts pairs of (prefix, value) efficiently."""
        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26 tree.")

        for item in items:
            prefix_str, val = item
            self.__setitem__(prefix_str, val)

    def dump_to_file(self, object file_obj):
        """
        Walks the active C-trie layout, harvests only logical pairs,
        and writes a clean, platform-independent data stream to a file.
        """
        cdef str prefix_str
        cdef patricia_node_t* node
        cdef list state_v4 = []
        cdef list state_v6 = []

        # gather active IPv4 nodes
        for prefix_str in self.iter_v4():
            node = self._parse_and_find(prefix_str, True)
            if node != NULL and node.data != NULL:
                state_v4.append((prefix_str, <object>node.data))

        # gather active IPv6 nodes
        for prefix_str in self.iter_v6():
            node = self._parse_and_find(prefix_str, True)
            if node != NULL and node.data != NULL:
                state_v6.append((prefix_str, <object>node.data))

        import pickle
        payload = {
            'state_v4': state_v4,
            'state_v6': state_v6,
            'is_frozen': self._tree_v4.frozen if self._tree_v4 != NULL else False
        }
        pickle.dump(payload, file_obj, protocol=pickle.HIGHEST_PROTOCOL)

    def load_from_file(self, object file_obj):
        """
        Reads a raw data stream from a file, thaws the trie,
        and dynamically rebuilds the underlying C routing matrix.
        """
        import pickle
        payload = pickle.load(file_obj)

        if not isinstance(payload, dict):
            raise ValueError("Invalid serialization payload structure.")

        cdef list state_v4 = payload.get('state_v4', [])
        cdef list state_v6 = payload.get('state_v6', [])
        cdef bint is_frozen = payload.get('is_frozen', False)

        # MANDATORY: Ensure writes are permitted during recovery loops
        self.thaw()

        for prefix_str, val in state_v4:
            self[prefix_str] = val

        for prefix_str, val in state_v6:
            self[prefix_str] = val

        # Re-apply frozen state only if the serialized data dictated it
        if is_frozen:
            self.freeze()

cdef inline bint _parse_ipv4_inline(const char* p, unsigned int* out_v4, int* out_bits) noexcept nogil:
    cdef unsigned int v4_val = 0
    cdef unsigned int part = 0
    cdef int dots = 0
    cdef int bitlen = 32
    cdef Py_ssize_t i = 0

    while p[i] != 0 and p[i] != b'/':
        if p[i] == b'.':
            v4_val = (v4_val << 8) | part
            part = 0
            dots += 1
        else:
            part = part * 10 + <unsigned int>(p[i] - 48)
        i += 1

    v4_val = (v4_val << 8) | part

    if dots != 3:
        return False

    if p[i] == b'/':
        i += 1
        bitlen = 0
        while p[i] != 0:
            bitlen = bitlen * 10 + <unsigned int>(p[i] - 48)
            i += 1
        if bitlen > 32:
            return False

    out_v4[0] = htonl(v4_val)
    out_bits[0] = bitlen
    return True
