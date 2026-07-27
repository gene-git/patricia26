/*
 * patricia-export.h
 */
#ifndef PATRICIA_EXPORT_H
#define PATRICIA_EXPORT_H

#if defined(__GNUC__) || defined(__clang__)
#define PATRICIA_EXPORT __attribute__((visibility("default")))
#else
#define PATRICIA_EXPORT
#endif

#endif

