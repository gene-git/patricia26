
Python Particia26 Class: API Reference
=======================================

This section contains the high-level Python API wrapper for the Patricia Trie.
The package includes API stubs file allowing tools such as *mypy* to type check
code that uses Patricia26.

Here are some of the important methods:

* Patricia26()
  
  Instantiate class

* Store Value

  pyt[cidr] = value
  pyt.bulk_insert(cidr_values)

  cidr_values is a list of tuples[cidr, value]

* lookup() 

  returns the value associated with the input prefix or None if no match
  Can also use dictionary lookup:

  value = pyt.lookup(cidr)
  value = pyt[cidr]


* bulk_lookup() 

  similar to lookup() but takes a list of prefixes and returns a list of values.
  Faster than looping on lookup()
  
  values = pyt.bulk_lookup(cidrs)

* lookup_lpm() 

  similar to lookup() but returns a tuple of the (lpm, value).
  Faster than looping on get_prefix() plus lookup() by maybe 15% - 20%.

* get_prefix()

  Returns the LPM prefix associated with IP or CIDR string.

* prefixes(), prefixes_v4, prefixes_v6

  returns a list of all active prefixes in the tree. The _v4 / _v6 
  variants return list of IPv4 / IPv6 prefixes.

* freeze() / thaw() 

  lock/unlock the tree when modifying it.

* parent() 

  Returrns the prefix that is the immediate parent of the lpm.

* children() 

  returns a list of prefixes that are subnets of the lpm prefix

* num_prefixes_v4(), num_prefixes_v6()

  Returns the number of *active* IPv4 or IPv6 prefixes in the corresponding tree.
  Note that len(pyt) will return the total number of prefixes across both IPv4 and
  IPv6 trees. 

  Note that this count includes the active nodes which includes *glue* nodes 
  that patricia automatically inserts when needed, not just those explicitly added. 

* dump_to_file() / load_from_file()

  Writes or reads a class instance to or from a file stram object.

Supports the *contains* method. 

When checking if prefix is in the tree the *in* operator returns True if the prefix
is either an exact match of a tree prefix or is a subnet of one. In contrast, the 
has_prefix() method checks for an exact match to an existing prefix.

There are couple of pytricia compatibility methods that may be useful:

* keys() - an alias for prefixes().
* get_key() - an alias for get_prefix()

For example, Adding and removing key/values to/from the Patricia tree is done using:

.. code:: python

   pyt = Patricia26()
   key = '10.0.0.0/24'
   val = 'lan-A'
   pyt[key] = val
   del pyt[key]

The next section has the auto-generated API reference.

Python API
----------

.. automodule:: patricia26
   :members:
   :undoc-members:
   :show-inheritance:
