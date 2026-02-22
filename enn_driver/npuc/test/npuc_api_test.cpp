/* Driver for NPU Compiler */

#include <iostream>
#include <string>

#include "NPUcompiler.h"

using namespace std;
using namespace NPUC;

int main(int argc, char **argv)
{

  int ret = 0;

  // 1) set option: currently SOCType
  // TODO add additional options if needed.
  NPUCCompilerOptions options;
  // NOTICE : set appropriate SOC type
  options.setSOCType(SOCType::ST_MAKALU);

  // 2) create NPUCompiler instance with options
  NPUCompiler compiler(options);

  // SupportedOperation test example
  // 3) call isSupportedOperation() to check if an op is support by NPUC
  // NOTICE : set real kernel, padding, stride size, zeropoint for each op.
  SupportedOperationOptions opOptions;
  opOptions.setKernelSize(3)->setPaddingSize(2)->setStrideSize(2)->setZeroPoint(128);

  // currently CONV_2D is supported
  bool supported = compiler.isSupportedOperation(OpType::CONV_2D, opOptions);
  NPUC_LOG(INFO) << "Conv is supported: true == " << supported << std::endl;

  // NPUC compile test example
  // 4) call compile to get ncp binary
  NPUC_LOG(INFO) << "NPU compiler comile test starts." << ret << std::endl;
// 4-1) create NNNetwork by using the converter
// NOTICE : net can be set as real value returned by the converter in EDEN Driver
// 4-2) create ncpBuffer to get ncp binary
#ifdef OFFLINE_COMPILER
  std::shared_ptr<NPUC::NNNetwork> nnNet = std::make_shared<NPUC::NNNetwork>();
  std::shared_ptr<NPUC::NCPBuffer> ncpBuf = std::make_shared<NPUC::NCPBuffer>();
#else
  NPUC::NNNetwork *nnNet = new NPUC::NNNetwork();
  NPUC::NCPBuffer *ncpBuf = new NPUC::NCPBuffer();
#endif
  ret = compiler.compile(nnNet, ncpBuf);
  NPUC_LOG(INFO) << "NPU compiler comile test ends." << ret << std::endl;

  delete nnNet;
  delete ncpBuf;

  return ret;
}
