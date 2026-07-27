/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * Copyright (c) 2026 Your Name <your.email@example.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
*/

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "patricia.h"

/* internal helper, not part of the public header, declared here just for the test */
//int my_inet_pton(int af, const char *src, void *dst);

int main(void) {
    patricia_tree_t *tree = New_Patricia(32);
    if (!tree) { printf("FAIL: New_Patricia\n"); return 1; }

    struct in_addr a1, a2, a3;
    inet_pton(AF_INET, "10.0.0.0", &a1);
    inet_pton(AF_INET, "10.0.0.0", &a2);
    inet_pton(AF_INET, "10.0.0.5", &a3);

    prefix_t p1 = {0}, p2 = {0}, p3 = {0};
    New_Prefix(AF_INET, &a1, 8, &p1);   /* 10.0.0.0/8  */
    New_Prefix(AF_INET, &a2, 24, &p2);  /* 10.0.0.0/24 */
    New_Prefix(AF_INET, &a3, 32, &p3);  /* 10.0.0.5/32 - search key */

    patricia_node_t *n1 = patricia_lookup(tree, &p1);
    if (!n1) { printf("FAIL: insert n1\n"); return 1; }
    n1->data = (void *)"n1-/8";

    patricia_node_t *n2 = patricia_lookup(tree, &p2);
    if (!n2) { printf("FAIL: insert n2\n"); return 1; }
    n2->data = (void *)"n2-/24";

    patricia_node_t *best = patricia_search_best(tree, &p3);
    if (!best) { printf("FAIL: search_best returned null\n"); return 1; }
    printf("search_best(10.0.0.5/32) -> %s (expect n2-/24, most specific)\n", (char *)best->data);
    if (strcmp((char *)best->data, "n2-/24") != 0) {
        printf("FAIL: expected most-specific /24 match\n");
        return 1;
    }

    patricia_node_t *exact = patricia_search_exact(tree, &p1);
    if (!exact || strcmp((char *)exact->data, "n1-/8") != 0) {
        printf("FAIL: search_exact for /8\n");
        return 1;
    }
    printf("search_exact(10.0.0.0/8) -> %s (expect n1-/8)\n", (char *)exact->data);

    char buf[64];
    char *s = prefix_toa2x(&p1, buf, 1);
    printf("prefix_toa2x(p1) -> %s (expect 10.0.0.0/8)\n", s);
    if (strcmp(s, "10.0.0.0/8") != 0) { printf("FAIL: prefix_toa2x\n"); return 1; }

    unsigned char parsed[4];
    //int r = my_inet_pton(AF_INET, "192.168.1.42", parsed);
    int r = inet_pton(AF_INET, "192.168.1.42", parsed);
    printf("my_inet_pton(192.168.1.42) -> ret=%d bytes=%d.%d.%d.%d\n",
            r, parsed[0], parsed[1], parsed[2], parsed[3]);
    if (r != 1 || parsed[0]!=192 || parsed[1]!=168 || parsed[2]!=1 || parsed[3]!=42) {
        printf("FAIL: my_inet_pton\n");
        return 1;
    }

    Destroy_Patricia(tree, NULL);
    printf("ALL OK\n");
    return 0;
}
