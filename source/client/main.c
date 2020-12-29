//
// Created by Marincia Cătălin on 22.12.2020.
//

#include "CommandHandlers.h"
i32 main(i32 argc, char **argv) {
//  if(remove(".staged/test")!=-1)
  CHECKCL(argc >=2, "You must provide an option, use help to see what is available")
  parse_command_line(argc, argv);
  return 0;
}
