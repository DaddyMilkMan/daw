#include <fstream>
#include <iostream>
#include <string>


void logToFile(const std::string &msg) {
  std::ofstream outfile;
  outfile.open("C:\\zenith\\daw\\debug_log.txt",
               std::ios_base::app); // Append mode
  outfile << msg << std::endl;
}
