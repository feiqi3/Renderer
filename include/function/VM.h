#ifndef VM_H_
#define VM_H_
#include "common/CommonMath.h"
#include "common/Name.h"
#include "function/VMDef.h"
#include "function/VMInstruction.h"
#include <vector>
#include <functional>
#include <map>
namespace Render {
	namespace VM {
		using GetParamValueFunc = std::function<void(const Name& name, uint32_t* outVal, ParamType& outType)>;
		using SetParamValueFunc = std::function<void(const Name& name, uint32_t  val, ParamType type)>;

		struct VMFuncs {
			GetParamValueFunc getFunc;
			SetParamValueFunc setFunc;
		};

		void runVM(VMRunTarget* runTarget, uint32_t stackSize, VMFuncs inFunc);

	}
}
#endif//STATEMENT_H_