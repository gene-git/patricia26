#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Copyright (c) 2026 Gene C <arch@sapience.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
import sys
import time
from pytricia import PyTricia

def benchmark_lookup(pyt: PyTricia, iterations: int, check_me: str, use_get_key: bool):
    
    start_time = time.perf_counter()
    if use_get_key:
        for _ in range(iterations):
            _ = pyt.get(check_me)

    else:

        for _ in range(iterations):
            _ = pyt[check_me]
    end_time = time.perf_counter()
    
    # output info
    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time
    
    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'get' if use_get_key else '[key]' 
    front = f'{mark:>8s} {ipt:>8s} {"val":>12s}'
    print(f'{front} {total_time:9.4f} {lookups_per_sec:13,.0f}')

def benchmark_lookup_lpm(pyt: PyTricia, iterations: int, check_me: str):
    
    start_time = time.perf_counter()
    for _ in range(iterations):
        lpm = pyt.get_key(check_me)
        _ = pyt[lpm]
    end_time = time.perf_counter()
    
    # output info
    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time
    
    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'getk'
    front = f'{mark:>8s} {ipt:>8s} {"(lpm, val)":>12s}'
    print(f'{front} {total_time:9.4f} {lookups_per_sec:13,.0f}')

if __name__ == "__main__":
    #
    # Populate the tree
    #
    nets = {
        "192.168.0.0/16": "LAN",
        "10.0.0.0/8": "Corporate",
        "192.0.2.0/24": "Fake Hacker Block",
        }

    pyt = PyTricia()
    for (net, val) in nets.items():
        pyt[net] = val

    if len(sys.argv) > 1:
        iterations = int(sys.argv[1])
    else:
        iterations = 200_000_000

    print('')
    print(f' {"PyTricia":>20s} ({iterations:,} iterations)')
    d8 = 8 * '-'
    d10 = 10 * '-'
    d12 = 12 * '-'
    d14 = 14 * '-'

    print(f'{"Method":>8s} {"Input":>8s} {"Lookup":>12s} {"Seconds":>10} {"Lookups / sec":>14s}')
    print(f'{d8:>8s} {d8:>8s} {d12:>12s} {d10:>10s} {d14:>14s}')

    benchmark_lookup(pyt, iterations, '192.0.2.45', False)
    benchmark_lookup(pyt, iterations, '192.0.2.45', True)
    benchmark_lookup(pyt, iterations, '192.0.2.128/25', True)

    benchmark_lookup_lpm(pyt, iterations, '192.0.2.128/25')
    benchmark_lookup_lpm(pyt, iterations, '192.0.2.45')
