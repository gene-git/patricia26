# src/patricia26/patricia26.pyi
from typing import Any, Iterator, Iterable, IO

class Patricia26:
    def __init__(self) -> None:
        """Instantiates a pair of C-based Patricia network trees one for IPv4 and one for IPv6

        Each network item is a string form of an IP address or a CIDR Block.

        Args:
            None

        Raises:
            MemoryError: If unable to allocate the native C Patricia tree.

        Internally keeps 2 trees - 1 for ipv4 and 1 for ipv6
        """
        ...

    def __getitem__(self, prefix_str: str) -> Any:
        """Retrieves the data value from the longest matching prefix block (LPM).

        Args:
            prefix_str: The IP or CIDR string to query (e.g., "192.0.2.45").

        Returns:
            Any: The custom Python object metadata mapped to the matched network prefix.
            None: no matching prefix was found 

        """
        ...

    def __setitem__(self, prefix_str: str, value: Any) -> None:
        """Inserts value to the LPM of an IP or CIDR block string.

        Args:
            prefix_str: The IP subnet string (e.g., "192.0.2.0/24") used to lookup the LPM
            value: Custom Python object metadata to associate with this network.

        Raises:
            RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def insert(self, prefix_str: str, value: Any) -> None:
        """Inserts value to the LPM of an IP or CIDR block string.

        Saem as __setitem__.

        Args:
            prefix_str: The IP subnet string (e.g., "192.0.2.0/24") used to lookup the LPM
            value: Custom Python object metadata to associate with this network.

        Raises:
            RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def __contains__(self, prefix_str: str) -> bool:
        """Checks if prefix_str IP or CIDR is contained within any network prefix within the tree.

        Args:
            prefix_str: The target network block or IP string to evaluate.

        Returns:
            bool: True if the target falls within an existing prefix block, False otherwise.
        """
        ...

    def __iter__(self) -> Iterator[str]:
        """Provides a memory-flat depth-first trie branch pointer iterator loop.
           over all IPv4 and IPv6 prefixes

        Yields:
            str: The network prefix CIDR block strings stored in the tree.
        """
        ...

    def iter_v4(self) -> Iterator[str]:
        """Provides a memory-flat depth-first trie branch pointer iterator loop.
           over all IPv4 prefixes

        Yields:
            str: The network prefix CIDR block strings stored in the tree.
        """
        ...

    def iter_v6(self) -> Iterator[str]:
        """Provides a memory-flat depth-first trie branch pointer iterator loop.
           over all IPv6 prefixes

        Yields:
            str: The network prefix CIDR block strings stored in the tree.
        """
        ...


    def __delitem__(self,  prefix_str: str) -> None:
        """
        Removes prefix_str from the tree if it present.
        Args:
            prefix_str: The IP or CIDR string to delete (e.g., "192.0.2.0/24").

        Raises:
            RuntimeError: If attempting to modify a frozen Patricia26 tree.
        """
        ...

    def prefixes_v4(self) -> list[str]:
        """Returns a list of all IPv4 tree prefixes."""
        ...

    def prefixes_v4(self) -> list[str]:
        """Returns a list of all IPv6 tree prefixes."""
        ...

    def prefixes(self) -> list[str]:
        """Returns a flat list of all tree prefixes."""
        ...

    def keys(self) -> list[str]:
        """Alias for prefixes()."""
        ...

    def num_prefixes_v4(self) -> int:
        """Returns the total number of IPv4 prefixes in the tree.
        """
        ...

    def num_prefixes_v6(self) -> int:
        """Returns the total number of IPv6 prefixes in the tree.
        """
        ...

    def __len__(self) -> int:
        """Returns the total number of active entries inside the tree.

        Same as num_prefixes_v4() + num_prefixes_v6()
        """
        ...

    def freeze(self) -> None:
        """Locks both ipv4 and ipv6 trees prevent updates or changes."""
        ...

    def __init_subclass__(cls) -> None: ...

    def thaw(self) -> None:
        """Unlocks a frozen tree to re-allow modifications."""
        ...

    def lookup(self, ip_str: str) -> Any:
        """Return the value associated with LPM (longest prefix match) matching ip_str 

        LPM is the longest prefix match.

        Args:
            ip_str: An IP address or CIDR string (e.g., "192.0.2.45" or "10.0.0.0/24").

        Returns:
            Any | None: The custom Python object metadata mapped to the matched network.
                         or None if unmatched
        """
        ...

    def bulk_lookup(self, ip_strings: list[str]) -> list[Any]:
        """Similat to lookup() but takes a list list of IP or CIDR strings as input.

        Significantly fastger than looping on lookup() using optimied C code.

        Args:
            ip_strings: A list of target IP or CIDR strings.

        Returns:
            list[Any]: An array of matched metadata objects corresponding to the input list index.
            If an ip_string has no matching prefix, then None is returned for that element.
        """
        ...

    def get_prefix(self, prefix_str: str) -> str | None:
        """Finds and returns the containing network prefix CIDR string for an IP or CIDR.

        Args:
            prefix_str: The IP address string to check.

        Returns:
            str | None: The closest enclosing network block CIDR string, or None if unmatched.
        """
        ...

    def get_key(self, prefix_str: str) -> str | None:
        """Alias for get_prefix()"""
        ...

    def children(self, prefix_str: str) -> list[str]:
        """Extracts a list of all subnets stored that are more specific than the parent match.

        Args:
            prefix_str: The base network enclosure string block.

        Returns:
            list[str]: A list of more specific subnet keys residing beneath the target block.
        """
        ...

    def parent(self, prefix_str: str) -> str | None:
        """Find and return the direct parent network prefix enclosing this target.

        Args:
            prefix_str: The base network enclosure string block.

        Returns:
            str | None: The parent network CIDR string if found, or None if prefix 
                is not contained within a broader network in the tree.
        """
        ...

    def has_prefix(self, ip_str: str) -> bool:
        """Determines if an exact prefix match exists as a key in the tree.

        Args:
            ip_str: The exact network block string to evaluate (e.g., "192.168.0.0/16").

        Returns:
            bool: True if the exact prefix key exists with a payload, False otherwise.
        """
        ...

    def has_key(self, ip_str: str) -> bool:
        """Alias of has_prefix()"""
        ...

    def lookup_lpm(self, ip_str: str) -> tuple[str, Any]:
        """
        Similar to lookup() but returns a tuple of (lpm, value) 
        longest-prefix match lookup returning a tuple of (matching_cidr, value).

        Args:
            ip_str: The exact network block string to evaluate (e.g., "192.168.0.0/16").

        Returns:
            tuple of (lmp, value)
            If no matching prefix then (None, None).
        """
        ...

    def bulk_insert(self, items: Iterable[tuple[str, Any]]) -> None: 
        """
        Insert a list of tuples of (prefix, value) into the tree:

        Args:
            items: list of (prefix, tuples) to add to the tree
        """
        ...

    def dump_to_file(self, file_obj: IO[bytes]):
        """
        Write the tree to the file_object stream.

        Args:
            file_obj: File object opened in binary mode ('wb')
        """
        ...

    def load_from_file(self, file_obj: IO[bytes]):
        """
        Read the tree from the file object stream.

        Args:
            file_obj: File object opened in binary mode ('rb')
        """
        ...


