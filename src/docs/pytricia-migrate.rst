.. _pytricia_migration:

=======================
Migrating from PyTricia
=======================

The ``Partcia26`` API is a little different and this provides a quick overview how
to migrate from ``PyTricia``.

**Instantiate Class**

.. code-block:: python

    from pytricia import Pytricia
    pyt_ipv4 = Pytricia(32)
    pyt_ipv6 = Pytricia(128)


becomes:

.. code-block:: python

   from patricia26 import Patricia26
   pyt_ipv4 = Patricia26()
   pyt_ipv6 = Patricia26()


Note that there is no bit length argument because Patricia26 keeps separate IPv4 and IPv6 trees
internally.

**Store Value**

.. code-block:: python

   pyt[ip_or_cidr] = value

becomes:

.. code-block:: python

   pyt[ip_or_cidr] = value

   or
   pyt.insert(ip_or_cidr, value)

   or
   cidr_values = [(ip_or_cidr1, value1), (ip_or_cidr2, value2), ...]
   pyt.bulk_insert(cidr_values)

**Lookup Value**

.. code-block:: python

   val = pyt[ip]
   val = pyt.get(cidr)

becomes

.. code-block:: python

    val = pyt.lookup(ip_or_cidr)

    or
    val = pyt[ip_or_cidr]

    or
    vals = pyt.bulk_lookup(ip_or_cidr_list)

**Lookup LPM and Value**

.. code-block:: python

   lpm = pyt.get_key(ip_or_cidr)
   val = pyt[lpm]

becomes

.. code-block:: python

   (lpm, val) = pyt.lookup_lpm(ip_or_cidr)
    
   or
   lpm = pyt.get_prefix((ip_or_cidr)
   val = pyt.lookup(lpm)


