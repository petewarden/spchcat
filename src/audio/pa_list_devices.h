#ifndef INCLUDE_PA_LIST_DEVICES_H
#define INCLUDE_PA_LIST_DEVICES_H

// Returns a list of the names of all input devices found on this system by
// Pulse Audio.
void get_input_devices(char*** devices, int* devices_length);

#endif  // INCLUDE_PA_LIST_DEVICES_H