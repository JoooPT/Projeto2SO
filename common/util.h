#ifndef UTIL_H
#define UTIL_H

/// Send message to a pipe
/// @param pipe
/// @param src
/// @param bytes
int send_msg(int pipe, void *src, size_t bytes);

/// Send read message from a pipe
/// @param pipe
/// @param dest
/// @param bytes
int get_msg(int pipe, void *dest, size_t bytes);

#endif // UTIL_H