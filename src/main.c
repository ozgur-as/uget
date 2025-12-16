#include "config.h"
#include "core.h"
#include <stdio.h>

int main(int argc, char **argv) {
  // For now, this is a minimal CLI stub to verify the build.
  // In the future, this should parse arguments and call uget_core functions.
  (void)argc;
  (void)argv;
  printf("uget (cli) built with CMake\n");
  return core_dummy();
}
