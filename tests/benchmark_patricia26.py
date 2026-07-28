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
from patricia26 import Patricia26
from patricia26 import Patricia26Int

import ipaddress

def benchmark_bulk_lookup_ipa(pyt_int: Patricia26, iterations: int, check_me: str):

    if '/' in check_me:
        test_ips = iterations * [ipaddress.ip_network(check_me)]
    else:
        test_ips = iterations * [ipaddress.ip_address(check_me)]

    start_time = time.perf_counter()
    results = pyt_int.bulk_lookup(test_ips)
    end_time = time.perf_counter()

    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time

    #print(f"Bulk IPA Lookups / sec: {len(test_ips) / (end - start):,.2f}")

    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'Bulk'
    front = f'{mark:>8s} {ipt:>8s} {"val":>12s}'
    print(f'{front} {total_time:9.4f} {lookups_per_sec:13,.0f}')

def benchmark_lookup(pyt: Patricia26, iterations: int, check_me: str, bulk: bool):

    # Generate a large list of target IP or CIDR strings to simulate log chunks
    log_ips = [check_me] * iterations

    start_time = time.perf_counter()
    if bulk:
        results = pyt.bulk_lookup(log_ips)
    else:
        for _ in range(iterations):
            _ = pyt.lookup(check_me)
    end_time = time.perf_counter()

    # output info
    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time

    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'Bulk' if bulk else 'Loop'
    front = f'{mark:>8s} {ipt:>8s} {"val":>12s}'
    print(f'{front} {total_time:9.4f} {lookups_per_sec:13,.0f}')

def benchmark_lookup_lpm(pyt: Patricia26, iterations: int, check_me: str):

    log_ips = [check_me] * iterations

    start_time = time.perf_counter()
    for _ in range(iterations):
        _ = pyt.lookup_lpm(check_me)
    end_time = time.perf_counter()

    # output info
    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time

    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'Loop'
    front = f'{mark:>8s} {ipt:>8s} {"(lpm, val)":>12s}'
    print(f'{front} {total_time:9.4f} {lookups_per_sec:13,.0f}')

def benchmark_lookup_lpm2(pyt: Patricia26, iterations: int, check_me: str):

    log_ips = [check_me] * iterations

    start_time = time.perf_counter()
    for _ in range(iterations):
        lpm = pyt.get_prefix(check_me)
        _ = pyt.lookup(lpm)
    end_time = time.perf_counter()

    # output info
    total_time = end_time - start_time
    lookups_per_sec = iterations / total_time

    ipt = 'CIDR' if '/' in check_me else 'IP'
    mark = 'Loop2'
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

    pyt = Patricia26()
    pyt_int = Patricia26Int()
    for (net, val) in nets.items():
        pyt[net] = val
        pyt_int[ipaddress.ip_network(net)] = val

    if len(sys.argv) > 1:
        iterations = int(sys.argv[1])
    else:
        iterations = 200_000_000

    d8 = 8 * '-'
    d10 = 10 * '-'
    d12 = 12 * '-'
    d14 = 14 * '-'

    print('')
    print(f' {"Patricia26":>20s} ({iterations:,} iterations)')
    print(f'{"Method":>8s} {"Input":>8s} {"Lookup":>12s} {"Seconds":>10} {"Lookups / sec":>14s}')
    print(f'{d8:>8s} {d8:>8s} {d12:>12s} {d10:>10s} {d14:>14s}')

    benchmark_lookup(pyt, iterations, '192.0.2.45', True)
    benchmark_lookup(pyt, iterations, '192.0.2.128/25', True)

    benchmark_lookup(pyt, iterations, '192.0.2.45', False)
    benchmark_lookup(pyt, iterations, '192.0.2.128/25', False)

    benchmark_lookup_lpm(pyt, iterations, '192.0.2.128/25')
    benchmark_lookup_lpm(pyt, iterations, '192.0.2.45')

    benchmark_lookup_lpm2(pyt, iterations, '192.0.2.128/25')
    benchmark_lookup_lpm2(pyt, iterations, '192.0.2.45')

    print('')
    print(' bulk_lookup_ipa:')
    benchmark_bulk_lookup_ipa(pyt_int, iterations, '192.0.2.128/25')
    benchmark_bulk_lookup_ipa(pyt_int, iterations, '192.0.2.45')
