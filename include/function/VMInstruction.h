#ifndef VM_INSTRUCTION_H_
#define VM_INSTRUCTION_H_
#include "common/CommonMath.h"
#include "common/Name.h"
#include "function/VMDef.h"
#include <vector>
#include <functional>
#include <map>
namespace Render {
	namespace VM {

		struct SymbolTable {
			//ConstantIndex -> ConstantValue
			std::vector<Operand> constantMap;
			//ParamIndex -> ParamName
			std::vector<Name> paramMap;
			//ParamName -> ParamIndex
			std::map<Name, ParamID> paramIndexMap;
		};

		struct VMRunTarget {
			SymbolTable symbols;
			InstructionList instructionList;
		};

		void destroyVMRunTarget(VMRunTarget* runTgt);
	}
}

#endif//VM_INSTRUCTION_H_