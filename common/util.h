#ifndef UTIL_H
#define UTIL_H

// Helper function to send messages
// Retries to send whatever was not sent in the beginning  
/// @param fd 
/// @param buff 
void send_msg(int fd, Buffer buff)

#endif // UTIL_H