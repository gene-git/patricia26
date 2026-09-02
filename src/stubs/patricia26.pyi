# src/patricia26/patricia26.pyi
from typing import Any, Iterator, Iterable, IO

class Patricia26:
    def __init__(self) -> None:
        """
        Instantiates a pair of C-based Patricia network trees one for IPv4 and one for IPv6

        Internally keeps 2 trees - 1 for ipv4 and 1 for ipv6
        Each network item is a string form of an IP address or a CIDR Block.

        :raises MemoryError: If unable to allocate the native C Patricia tree.

        """
        ...

    def __getitem__(self, prefix_str: str) -> Any:
        """
        Retrieves the data value from the longest matching prefix block (LPM).

        :param prefix_str: The IP or CIDR string to query (e.g., "192.0.2.45").
        :returns: The custom Python object metadata mapped to the matched network prefix.
                  or None if no matching prefix was found 
        """
        ...

    def __setitem__(self, prefix_str: str, value: Any) -> None:
        """
        Inserts value to the LPM of an IP or CIDR block string.

        :param prefix_str: The IP subnet string (e.g., "192.0.2.0/24") used to lookup the LPM
        :param value: Custom Python object metadata to associate with this network.
        :raises RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def insert(self, prefix_str: str, value: Any) -> None:
        """
        Inserts value to the LPM of an IP or CIDR block string.

        Saem as __setitem__.

        :param prefix_str: The IP subnet string (e.g., "192.0.2.0/24") used to lookup the LPM
        :param value: Custom Python object metadata to associate with this network.
        :raises RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def __contains__(self, prefix_str: str) -> bool:
        """
        Checks if prefix_str IP or CIDR is contained within any network prefix within the tree.

        :param prefix_str: The target network block or IP string to evaluate.
        :returns: True if the target falls within an existing prefix block, False otherwise.
        """
        ...

    def __iter__(self) -> Iterator[str]:
        """
        Provides an iterator over all IPv4 and IPv6 prefixes
        :yields: The next network prefix CIDR block strings stored in the tree.
        """
        ...

    def iter_v4(self) -> Iterator[str]:
        """
        Provides an iterator over all IPv4 prefixes

        :yields: The next network prefix CIDR block strings stored in the tree.
        """
        ...

    def iter_v6(self) -> Iterator[str]:
        """
        Provides an iterator over all IPv6 prefixes

        Yields: The next network prefix CIDR block strings stored in the tree.
        """
        ...


    def __delitem__(self,  prefix_str: str) -> None:
        """
        Removes prefix_str from the tree (if it present).

        :param prefix_str: The IP or CIDR string to delete (e.g., "192.0.2.0/24").
        :raises RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def prefixes_v4(self) -> list[str]:
        """
        Returns a list of all IPv4 tree prefixes.
        :returns: List of all IPv4 prefixes in the tree.
        """
        ...

    def prefixes_v6(self) -> list[str]:
        """
        Returns a list of all IPv6 tree prefixes
        :returns: List of all IPv6 prefixes in the tree.
        """
        ...

    def prefixes(self) -> list[str]:
        """
        Returns a list of all prefixes
        :returns: List of all IPv4 and IPv6 prefixes in the tree.
        """
        ...

    def keys(self) -> list[str]:
        """
        Alias for prefixes()
        """
        ...

    def num_prefixes_v4(self) -> int:
        """
        Returns the total number of IPv4 prefixes in the tree.
        :returns: Number of IPv4 prefixes.
        """
        ...

    def num_prefixes_v6(self) -> int:
        """
        Returns the total number of IPv6 prefixes in the tree.
        :returns: Number of IPv6 prefixes.
        """
        ...

    def __len__(self) -> int:
        """
        Returns the total number of prefixes in the tree.

        Same as num_prefixes_v4() + num_prefixes_v6()
        :returns: Number of prefixes in the tree (both IPv4 and IPv6)
        """
        ...

    def freeze(self) -> None:
        """
        Locks both ipv4 and ipv6 trees prevent updates or changes
        """
        ...

    def __init_subclass__(cls) -> None: ...

    def thaw(self) -> None:
        """
        Unlocks a frozen tree to re-allow modifications
        """
        ...

    def lookup(self, ip_str: str) -> Any:
        """
        Return the value associated with LPM (longest prefix match) matching ip_str 

        LPM is the longest prefix match.

        :param ip_str: An IP address or CIDR string (e.g., "192.0.2.45" or "10.0.0.0/24").
        :returns: The custom Python object metadata mapped to the matched network.
                  or None if unmatched
        """
        ...

    def bulk_lookup(self, ip_strings: list[str]) -> list[Any]:
        """
        Similar to lookup() but takes a list list of IP or CIDR strings as input.

        Significantly faster than looping on lookup() as it uses optimied C code.

        :param ip_strings: A list of target IP or CIDR strings.
        :returns: A list of values that match each corresponding ip_string in input
                  If an ip_string has no matching prefix, then None is returned for that element.
        """
        ...

    def get_prefix(self, prefix_str: str) -> str | None:
        """
        Finds and returns the prefix (CIDR) string for an IP or CIDR.

        :param prefix_str: The IP/CIDR string to check.
        :returns: The closest enclosing network block CIDR string, or None if unmatched.
        """
        ...

    def get_key(self, prefix_str: str) -> str | None:
        """Alias for get_prefix()"""
        ...

    def children(self, prefix_str: str) -> list[str]:
        """
        Extracts a list of all child subnets (those with smaller prefix than the parent)

        :param prefix_str: The prefix to lookup
        :returns: A list of more specific subnets that sit beneath the LPM(prefix_str).
        """
        ...

    def parent(self, prefix_str: str) -> str | None:
        """
        Find and return the parent prefix that encloses the input prefix.

        :param prefix_str: The prefix to lookup the parent for.
        :returns: The parent network prefix if found, or None if prefix 
                  is not contained within any subnet in tree
        """
        ...

    def has_prefix(self, ip_str: str) -> bool:
        """
        Determines if an exact prefix match exists in the tree.

        :param ip_str: The prefix to check (e.g., "192.168.0.0/16").
        :returns: True if the prefix exists (non-glue)
        """
        ...

    def has_key(self, ip_str: str) -> bool:
        """Alias of has_prefix()"""
        ...

    def lookup_lpm(self, ip_str: str) -> tuple[str, Any]:
        """
        Similar to lookup() but returns a tuple of (lpm, value) 
        longest-prefix match lookup returning a tuple of (matching_cidr, value).

        :param ip_str: The prefix string to lookup (e.g., "192.168.0.0/16").
        :returns: tuple of (lmp, value) or (None, None) if not found.
        """
        ...

    def bulk_insert(self, items: Iterable[tuple[str, Any]]) -> None: 
        """
        Insert a list of tuples of (prefix, value) into the tree:

        :param items: list of (prefix, tuples) to add to the tree
        """
        ...

    def dump_to_file(self, file_obj: IO[bytes]):
        """
        Write the tree to the file_object stream.

        :param file_obj: File object that was opened in binary mode ('wb')
        """
        ...

    def load_from_file(self, file_obj: IO[bytes]):
        """
        Read the tree from the file object stream.

        :param file_obj: File object opened in binary mode ('rb')
        """
        ...


