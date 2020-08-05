#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "console_client.hpp"

int main(int argc, char* argv[]) {

  ArgParser parser(argc, argv);
  parser.parse();

  if(parser.is_ok())
  {
    if(parser.has_arg("hex"))
    {
      std::string original = parser.get_arg("hex");
      std::stringstream file_list_stream(original);
      std::string tmp;
      std::vector<std::string> filenames;
      constexpr static char delimiter = ';';
      while(std::getline(file_list_stream, tmp, delimiter))
      {
        filenames.emplace_back(tmp);
      }

      for(const auto& name : filenames)
      {
        std::cout << "The hex value of '" << name <<"' is: 0x" << SimpleGDSClient::to_hex(name) << std::endl;
      }
    }
    else if(parser.has_arg("help"))
    {
      std::cout << parser.message() << std::endl;
    }
    else if(parser.has_commands())
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
