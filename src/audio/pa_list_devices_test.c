#include "acutest.h"

#include "pa_list_devices.c"

static void test_get_input_devices() {
  char** devices = NULL;
  int devices_length = 0;
  get_input_devices(&devices, &devices_length);
  fprintf(stderr, "Pulse Audio input devices found: \n");
  if (devices_length == 0) {
    fprintf(stderr, "None\n");
  }
  else {
    for (int i = 0; i < devices_length; ++i) {
      fprintf(stderr, "%s\n", devices[i]);
    }
  }
  string_list_free(devices, devices_length);
}

TEST_LIST = {
  {"get_input_devices", test_get_input_devices},
  {NULL, NULL},
};