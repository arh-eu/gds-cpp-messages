#include <iostream>
#include <string>

#include "console_client.hpp"

int main(int argc, char* argv[]) {

  ArgParser parser(argc, argv);

  if(parser.parse())
  {
    if(parser.has_arg("hex"))
    {
      std::string original = parser.get_arg("hex");
      std::cout << "The hex value of '" << original <<"' is: 0x" << SimpleGDSClient::to_hex(original) << std::endl;
    }
    else
    {
      std::cout << "Argument parsing successful, starting GDS client.." << std::endl;
      try{
        SimpleGDSClient client(parser);
        client.run();
      }
      catch(const std::exception& e)
      {
        std::cout << e.what() << std::endl;
      }
    }

  }
  else
  { 
    std::cout <<"Invalid arguments passed!" << std::endl;
    if(parser.message().length())
      std::cout << parser.message() << std::endl;
  }

  return 0;
}
