#include "function/AST.h"

namespace Render::VM::AST {
	void compileAST(ExprNodeBase* entry, VMRunTarget** outRunTarget)
	{
		VMRunTarget* runTarget = new VMRunTarget;
		ASTCompileIntermediate* intermediate = new ASTCompileIntermediate;
		entry->compile(runTarget, intermediate);
		intermediate->addInstruction(Opcode::Halt, 0);
		runTarget->instructionList = intermediate->toInstructionList();
		*outRunTarget = runTarget;
		delete intermediate;
	}
}
