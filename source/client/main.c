//
// Created by Marincia Cătălin on 22.12.2020.
//

#include "Commands.h"

i32 main(i32 argc, char **argv) {
  CHECKCL(argc >=2, "You must provide an option, use help to see what is available")
  cmd_distributor(argc, argv);
  return 0;
}
