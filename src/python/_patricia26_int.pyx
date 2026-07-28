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
# patricia26_int.pyx - Pure Numerical / ipaddress Tracker
#
from libc.string cimport memcpy, memset
from libc.stdlib cimport atoi
from libc.stdint cimport uint8_t, uint32_t

from cpython.ref cimport Py_INCREF, Py_DECREF

cdef extern from "<sys/socket.h>" nogil:
    int AF_INET
    int AF_INET6

cdef extern from "<arpa/inet.h>" nogil:
    int inet_pton(int af, const char* src, void* dst)
    unsigned int htonl(unsigned int hostlong)

cdef extern from "<netinet/in.h>" nogil:
    struct in_addr:
        uint32_t s_addr
    struct in6_addr:
        uint8_t s6_addr[16]

cdef extern from "patricia.h":
    ctypedef unsigned short sa_family_t
    ctypedef union prefix_add_u:
        in_addr sin
        in6_addr sin6

    ctypedef struct prefix_t:
        sa_family_t family
        uint8_t bitlen
        uint8_t pad
        int ref_count
        prefix_add_u add

    ctypedef struct patricia_node_t:
        prefix_t prefix
        void* data
        patricia_node_t* l
        patricia_node_t* r
        patricia_node_t* parent

    ctypedef struct patricia_tree_t:
        int num_active_node
        int frozen
        patricia_node_t* head

    patricia_tree_t* New_Patricia(int maxbits)
    void Destroy_Patricia(patricia_tree_t* patricia, void (*data_free)(void*) noexcept)
    patricia_node_t* patricia_lookup(patricia_tree_t* patricia, prefix_t* prefix)
    patricia_node_t* patricia_search_best(patricia_tree_t* patricia, prefix_t* prefix)
    patricia_node_t* patricia_search_exact(patricia_tree_t* patricia, prefix_t* prefix)
    void patricia_remove(patricia_tree_t* patricia, patricia_node_t* node)
    char* prefix_toa2x(prefix_t* prefix, char* buff, int with_len)

cdef void dec_python_ref(void* data) noexcept with gil:
    if data != NULL:
        Py_DECREF(<object>data)

cdef class Patricia26Int:
    cdef patricia_tree_t* _tree_v4
    cdef patricia_tree_t* _tree_v6

    def __cinit__(self):
        self._tree_v4 = New_Patricia(32)
        self._tree_v6 = New_Patricia(128)
        if self._tree_v4 == NULL or self._tree_v6 == NULL:
            raise MemoryError("Unable to allocate native dual integer tree blocks.")

    def __dealloc__(self):
        if self._tree_v4 != NULL:
            Destroy_Patricia(self._tree_v4, dec_python_ref)

        if self._tree_v6 != NULL:
            Destroy_Patricia(self._tree_v6, dec_python_ref)

    cdef object _prefix_to_ipa(self, prefix_t* prefix):
        import ipaddress
        cdef char buf[64]
        cdef char* c_str = prefix_toa2x(prefix, buf, 1)

        if c_str == NULL:
            return None
        return ipaddress.ip_network(c_str.decode('utf-8'), strict=False)

    cdef inline void _store_v6_to_prefix(self, object raw_ip_int, prefix_t* prefix) noexcept:
        py_w0 = (raw_ip_int >> 96) & 0xFFFFFFFF
        py_w1 = (raw_ip_int >> 64) & 0xFFFFFFFF
        py_w2 = (raw_ip_int >> 32) & 0xFFFFFFFF
        py_w3 = raw_ip_int & 0xFFFFFFFF

        cdef unsigned int w0 = htonl(<unsigned int>py_w0)
        cdef unsigned int w1 = htonl(<unsigned int>py_w1)
        cdef unsigned int w2 = htonl(<unsigned int>py_w2)
        cdef unsigned int w3 = htonl(<unsigned int>py_w3)

        # CLEAN & SAFE: Write directly to the typed structure field names!
        cdef char* dest = <char*>&prefix.add.sin6

        memcpy(dest, &w0, 4)
        memcpy(dest + 4, &w1, 4)
        memcpy(dest + 8, &w2, 4)
        memcpy(dest + 12, &w3, 4)

    cdef inline patricia_node_t* _parse_and_find(self, object key, bint exact) noexcept:
        """Unified native object router path wrapper. Exception free."""
        cdef prefix_t prefix
        cdef int family = 2
        cdef object raw_ip_int
        cdef unsigned int v4_val

        # Clear memory safely to ensure the padding byte is always zero
        memset(&prefix, 0, sizeof(prefix_t))

        if hasattr(key, "version"):
            family = 10 if key.version == 6 else 2
            if hasattr(key, "prefixlen"):
                prefix.bitlen = <uint8_t>key.prefixlen
                raw_ip_int = key.network_address._ip
            else:
                prefix.bitlen = 128 if family == 10 else 32
                raw_ip_int = key._ip
        elif isinstance(key, int):
            family = 10 if key > 0xFFFFFFFFUL else 2
            prefix.bitlen = 128 if family == 10 else 32
            raw_ip_int = key
        else:
            return NULL

        prefix.family = <unsigned short>family

        if family == 2:
            v4_val = htonl(<unsigned int>raw_ip_int)
            # CLEAN & SAFE: Standard structure member referencing
            memcpy(&prefix.add.sin, &v4_val, 4)

            if exact or prefix.bitlen < 32:
                return patricia_search_exact(self._tree_v4, &prefix)
            return patricia_search_best(self._tree_v4, &prefix)
        else:
            self._store_v6_to_prefix(raw_ip_int, &prefix)
            if exact or prefix.bitlen < 128:
                return patricia_search_exact(self._tree_v6, &prefix)
            return patricia_search_best(self._tree_v6, &prefix)


    def __getitem__(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)
        if node == NULL or node.data == NULL:
            return None
        return <object>node.data

    def __contains__(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)
        return node != NULL and node.data != NULL

    def lookup(self, object key):
        """Returns matched node data payload safely without throwing lookup exceptions."""
        return self[key]

    def get_prefix(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)

        if node == NULL or node.data == NULL:
            return None
        return self._prefix_to_ipa(&node.prefix)

    def has_prefix(self, str prefix_str):
        """Returns True ONLY if the exact CIDR prefix length exists with active user data."""
        # Check node and node.data to protect against internal structural placeholders
        cdef patricia_node_t* node = self._parse_and_find(prefix_str, True)
        return node != NULL and node.data != NULL

    def parent(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, True)

        if node == NULL:
            return None

        cdef patricia_node_t* curr = node.parent

        while curr != NULL:
            if curr.data != NULL:
                return self._prefix_to_ipa(&curr.prefix)
            curr = curr.parent
        return None

    def children(self, object prefix_key):
        """Returns a list of all active subnet IP objects enclosed directly under this prefix block."""
        cdef patricia_node_t* root_node
        cdef patricia_node_t* curr
        cdef patricia_node_t* stack[129]
        cdef list child_prefixes = []
        cdef int sp = 0

        # Find the starting root block exactly using your internal parser
        root_node = self._parse_and_find(prefix_key, True)
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
                if curr.data != NULL:
                    child_prefixes.append(self._prefix_to_ipa(&curr.prefix))

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

    def lookup_lpm(self, object key):
        cdef patricia_node_t* node = self._parse_and_find(key, False)

        if node == NULL or node.data == NULL:
            return (None, None)
        return (self._prefix_to_ipa(&node.prefix), <object>node.data)

    def lookup_lpm_ipa(self, object key):
        return self.lookup_lpm(key)

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

    def bulk_lookup(self, list ipa_list):
        """Hyper-optimized native object batch query supporting mixed hosts and subnets.

        Natively isolates address family scopes instantly, allowing allocation-free
        handling of combined Address and Network objects within uniform family streams.
        """
        cdef Py_ssize_t list_len = len(ipa_list)
        if list_len == 0:
            return []

        cdef prefix_t prefix
        cdef patricia_node_t* node
        cdef Py_ssize_t idx
        cdef object obj
        cdef unsigned int v4_val

        results = [None] * list_len

        # Peek index 0 to determine the unmixed family path (IPv4 or IPv6)
        first_obj = ipa_list[0]
        cdef bint is_v6 = first_obj.version == 6
        prefix.family = 10 if is_v6 else 2

        # Cache the exact underlying class types of bare host objects to bypass slow property checks
        import ipaddress
        cdef object v4_addr_type = ipaddress.IPv4Address
        cdef object v6_addr_type = ipaddress.IPv6Address

        if is_v6:
            # === PATH A: MIXED IPv6 LOOKUPS (ADDRESSES & NETWORKS) ===
            for idx in range(list_len):
                obj = ipa_list[idx]

                # Lightning-fast pointer class comparison
                if type(obj) is v6_addr_type:
                    prefix.bitlen = 128
                    self._store_v6_to_prefix(obj._ip, &prefix)
                else:
                    prefix.bitlen = <unsigned int>obj.prefixlen
                    self._store_v6_to_prefix(obj.network_address._ip, &prefix)

                node = patricia_search_best(self._tree_v6, &prefix)
                if node != NULL and node.data != NULL:
                    results[idx] = <object>node.data
        else:
            # === PATH B: MIXED IPv4 LOOKUPS (ADDRESSES & NETWORKS) ===
            for idx in range(list_len):
                obj = ipa_list[idx]

                if type(obj) is v4_addr_type:
                    prefix.bitlen = 32
                    v4_val = htonl(<unsigned int>obj._ip)
                else:
                    prefix.bitlen = <unsigned int>obj.prefixlen
                    v4_val = htonl(<unsigned int>obj.network_address._ip)

                memcpy(&prefix.add, &v4_val, 4)
                node = patricia_search_best(self._tree_v4, &prefix)
                if node != NULL and node.data != NULL:
                    results[idx] = <object>node.data

        return results

    #
    # --- Standard Object Dictionary API Hooks ---
    #

    def __setitem__(self, object key, object value):
        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26Int tree.")

        cdef prefix_t prefix
        cdef patricia_node_t* node
        cdef int family = 2
        cdef object raw_ip_int

        if hasattr(key, "version"):
            family = 10 if key.version == 6 else 2
            if hasattr(key, "prefixlen"):
                prefix.bitlen = <unsigned int>key.prefixlen
                raw_ip_int = key.network_address._ip
            else:
                prefix.bitlen = 128 if family == 10 else 32
                raw_ip_int = key._ip
        elif isinstance(key, int):
            family = 10 if key > 0xFFFFFFFF else 2
            prefix.bitlen = 128 if family == 10 else 32
            raw_ip_int = key
        else:
            raise TypeError("Unsupported target key type constraint profile")

        prefix.family = family
        if family == 2:
            v4_val = htonl(<unsigned int>raw_ip_int)
            memcpy(&prefix.add, &v4_val, 4)
            node = patricia_lookup(self._tree_v4, &prefix)
        else:
            self._store_v6_to_prefix(raw_ip_int, &prefix)
            node = patricia_lookup(self._tree_v6, &prefix)

        if node == NULL:
            raise RuntimeError("Unable to expand trie slot mappings.")
        if node.data != NULL:
            Py_DECREF(<object>node.data)
        Py_INCREF(value)
        node.data = <void*>value

    def __delitem__(self, object key):
        # 1. Move all declarations to the absolute top of the function
        cdef patricia_node_t* node
        cdef patricia_tree_t* target_tree

        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26Int tree.")

        node = self._parse_and_find(key, True)

        # 2. Wrap all deletion and tree assignment logic safely inside the non-NULL block
        if node != NULL and node.data != NULL:
            Py_DECREF(<object>node.data)
            node.data = NULL

            # Use the faster C-level family check directly from the node itself
            target_tree = self._tree_v6 if node.prefix.family == 10 else self._tree_v4
            patricia_remove(target_tree, node)

    def prefixes_v4(self):
        return list(self.iter_v4())

    def prefixes_v6(self):
        return list(self.iter_v6())

    def prefixes(self):
        """Combines all tracking layouts and matches into a unified list container."""
        return self.prefixes_v4() + self.prefixes_v6()

    def insert(self, object prefix_str, object val):
        """Inserts value to the LPM of an IP or CIDR block string."""
        self.__setitem__(prefix_str, val)

    def bulk_insert(self, object items):
        """Bulk inserts pairs of (prefix, value) efficiently.

        Expects an iterable of 2-tuples: Iterable[tuple[object, object]]
        """
        cdef patricia_node_t* node
        cdef object key
        cdef object val

        if self._tree_v4.frozen:
            raise RuntimeError("Cannot modify a frozen Patricia26Int tree.")

        for item in items:
            key, val = item
            node = self._parse_and_find(key, False)

            if node != NULL:
                if node.data != NULL:
                    Py_DECREF(<object>node.data)

                Py_INCREF(val)
                node.data = <void*>val

    def iter_v4(self):
        """Yields all active v4 prefixes as IP objects using a zero-allocation stack."""
        cdef patricia_node_t* stack[129]
        cdef int sp = 0
        cdef patricia_node_t* curr

        if self._tree_v4 != NULL and self._tree_v4.head != NULL:
            curr = self._tree_v4.head
            while curr != NULL:
                if curr.data != NULL:
                    yield self._prefix_to_ipa(&curr.prefix)

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
        """Yields all active v6 prefixes as IP objects using a zero-allocation stack."""
        cdef patricia_node_t* stack[129]
        cdef int sp = 0
        cdef patricia_node_t* curr

        if self._tree_v6 != NULL and self._tree_v6.head != NULL:
            curr = self._tree_v6.head

            while curr != NULL:
                if curr.data != NULL:
                    yield self._prefix_to_ipa(&curr.prefix)

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

    def dump_to_file(self, object file_obj):
        """
        Walks the active C-trie layout, harvests only logical pairs,
        and writes a clean, platform-independent data stream to a file.
        """
        cdef object ipa_obj
        cdef list state_v4 = []
        cdef list state_v6 = []

        # Gather active IPv4 nodes
        for ipa_obj in self.iter_v4():
            node = self._parse_and_find(ipa_obj, True)
            if node != NULL and node.data != NULL:
                state_v4.append((ipa_obj, <object>node.data))

        # Gather active IPv6 nodes
        for ipa_obj in self.iter_v6():
            node = self._parse_and_find(ipa_obj, True)
            if node != NULL and node.data != NULL:
                state_v6.append((ipa_obj, <object>node.data))

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
