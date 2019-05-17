
#include <stddef.h>

#include "MatlabInterface.hpp"

#include <iostream>

namespace util {

mat::Engine* MatlabInterface::ep = NULL;
char MatlabInterface::output[32768];
std::vector<std::string> MatlabInterface::paths;

std::string MatlabInterface::meval(const std::string& cmd) {
  if(!useMatlabEngine())
    return "MatlabInterface::NoMatlabEngineRunning\n";

  mat::engEvalString((mat::Engine*)ep,cmd.c_str());
  std::string rv = std::string(output);
  rv.erase(0,2);
  output[0]=0;

  std::cout << std::flush;

  return rv;
}

void MatlabInterface::startMatlabEngine() {
  if(ep)
    return;

  if(!ep)
    ep = mat::engOpen(nullptr);
  
  if(!ep) {
    std::cerr << "MatlabInterface: WARNING Matlab not started with windows hack!" << std::endl;
    return;
  }

  mat::engOutputBuffer(ep,output,sizeof(output));

  std::cout << "MatlabInterface: Matlab Engine started." << std::endl;

  for(auto it=paths.begin(); it!=paths.end(); ++it)
    meval((std::string("addpath ")+*it)+";");

  // enable multi-threaded computations
  mat::engEvalString(ep, "feature('NumThreads',feature('NumCores'))"); 
}

void MatlabInterface::closeMatlabEngine() {
  if(ep) {
    engClose((mat::Engine*)ep);
    ep = NULL;
    std::cout << "MatlabInterface: Matlab Engine closed." << std::endl;
  }
}

bool MatlabInterface::useMatlabEngine() {
  if(!ep) {
    startMatlabEngine();
  }

  if(ep)
    return true;

  return false;
}

} //namespace util
