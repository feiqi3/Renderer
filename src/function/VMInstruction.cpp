#include "function/VMInstruction.h"

namespace Render::VM {
	void destroyVMRunTarget(VMRunTarget* runTgt)
	{
		delete[] runTgt->instructionList.list;
		runTgt->instructionList.list = nullptr;
		delete runTgt;
	}
}
