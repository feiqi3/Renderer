#ifndef AST_H_
#define AST_H_
#pragma once
#include "common/CommonMath.h"
#include "common/Name.h"
#include "function/VMDef.h"
#include "function/VMInstruction.h"
#include <map>

#define AST_DETECT_ERROR_AND_SET(ret,node) \
if(ret == false){\
	lastErrorInfo = node->lastErrorInfo;\
	return false;\
}

#define AST_SET_NODE_INDEX(node) \
node->nodeIndexInTree = intermediate->treeNodeIndex++;

#define AST_SET_ERROR(err)\
{\
lastErrorInfo.error = err;\
lastErrorInfo.errorNodeIndex = nodeIndexInTree;\
}

namespace Render::VM {
	struct VMRunTarget;
}

namespace Render::VM::AST {
	enum class ASTError {
		NONE,
		VAR_NOT_FOUND,
		OP_NOT_MATCH
	};

	struct ASTErrorInfo {
		ASTError error = ASTError::NONE;
		int errorNodeIndex = -1;
	};

	struct StackFrame {
		uint32_t localVarSize = 0;
		uint32_t baseStackPtr = 0;
		std::vector<std::string> localVars;
	};

	struct ASTCompileIntermediate {
		//uint32_t stackBase[64];
		uint32_t curStackBase = 0;
		uint32_t curStackPtr = 0;
		uint32_t treeNodeIndex = 0;
		std::map<std::string, std::vector<uint32_t>> localVariables; //name, offset to bp
		std::map<std::string, uint32_t> outerParam; //name, index (place inside symbol table)
		std::map<u64, ConstantID> constantMap; //Constant value -> constant ID;
		std::vector<Instruction> instructionBuilder;
		std::vector<StackFrame> stackFramesStack;
		int stackFrameDepth = 0;
		void enterScope() {
			stackFrameDepth++;
			StackFrame newStackFrame{};
			newStackFrame.baseStackPtr = curStackPtr;
			newStackFrame.localVarSize = 0;
			stackFramesStack.push_back(newStackFrame);
			curStackBase = newStackFrame.baseStackPtr;
		}
		void leaveScope() {
			if (stackFramesStack.empty()) return;

			const auto& curStackFrame = stackFramesStack.back();

			for (const auto& localName : curStackFrame.localVars) {
				removeLocalVar(localName);
			}
			uint32_t popCount = curStackPtr - curStackFrame.baseStackPtr;


			if (popCount > 0) {
				addInstruction(Opcode::Pop, popCount);
			}
			curStackPtr = curStackFrame.baseStackPtr;
			stackFramesStack.pop_back();
			stackFrameDepth--;
			if (!stackFramesStack.empty())
			{
				curStackBase = stackFramesStack.back().baseStackPtr;
			}
			else 
			{
				curStackBase = 0;
			}
		}

		struct InstructionRef {
			ASTCompileIntermediate* intermediate;
			uint32_t insPos;
			void changeOprand(uint32_t v) {
				intermediate->instructionBuilder[insPos].opnd = v;
			}
		};

		uint32_t getNextInsLoc()const
		{
			return this->instructionBuilder.size();
		}

		InstructionRef addInstruction(Opcode op, uint32_t imm) {
			if (imm > INSTRUCTION_CONSTANT_MAX) {
				assert(false);
				*(int*)0;
			} //Max
			
			Instruction ins{};
			ins.op = op;
			ins.opnd = imm;

			instructionBuilder.push_back(ins);

			InstructionRef ref{};
			ref.intermediate = this;
			ref.insPos = instructionBuilder.size() - 1;
			return ref;
		}
		void removeLocalVar(const std::string& var) {
			auto itor = localVariables.find(var);
			if (itor == localVariables.end()) {
				return;
			}

			if (itor->second.size() == 1) {
				localVariables.erase(itor);
			}
			else {
				itor->second.pop_back();
			}
			stackFramesStack.back().localVarSize--;
		}

		void addLocalVar(const std::string& var) {
			auto itor = localVariables.find(var);
			if (itor == localVariables.end()) {
				localVariables.insert({ var,{curStackPtr} });
			}
			else {
				itor->second.push_back(curStackPtr);
			}


			if (!stackFramesStack.empty()) {
				stackFramesStack.back().localVars.push_back(var);
				stackFramesStack.back().localVarSize++;
			}
		}
		void stackPush() {
			curStackPtr++;
		}
		void stackPop() {
			assert(curStackPtr >= 1);
			curStackPtr--;
		}

		uint32_t getStack() {
			return curStackBase + curStackPtr;
		}

		bool citeLocalVariable(const std::string& varName, uint32_t& offsetFromCurSP) {
			auto itor = localVariables.find(varName);
			if (itor == localVariables.end()) {
				return false;
			}

			auto targetStackPTR = itor->second;
			offsetFromCurSP = curStackPtr - targetStackPTR.back();
			return true;
		}

		bool citeParam(const std::string& varName, ParamID& outParamID) {
			auto itor = outerParam.find(varName);
			if (itor == outerParam.end()) {
				return false;
			}
			outParamID = itor->second;
			return true;
		}

		uint32_t addConstant(VMRunTarget* runTarget, Operand constant) {

			u64 key = (static_cast<u64>(constant.tag) << 32) | static_cast<u64>(constant.val._u);

			auto itor = this->constantMap.find(key);
			if (itor != constantMap.end()) {
				return itor->second;
			}

			runTarget->symbols.constantMap.push_back(constant);
			ConstantID constantID = runTarget->symbols.constantMap.size() - 1;


			this->constantMap.insert({ key,constantID});
			return constantID;
		}

		InstructionList toInstructionList() {
			auto insNum = instructionBuilder.size();
			Instruction* outInsList = new Instruction[insNum];
			memcpy(outInsList, instructionBuilder.data(), sizeof(Instruction) * insNum);
			InstructionList outList;
			outList.list = outInsList;
			outList.instructionNum = insNum;
			return outList;
		}
	};

	struct ExprNodeBase {
		ExprNodeBase() :leftNode(nullptr), rightNode(nullptr), op(Opcode::Nop), lastErrorInfo({}), nodeIndexInTree(0) {}

		virtual bool compile(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			//Post order
			bool ret = true;
			if(leftNode)
			{
				AST_SET_NODE_INDEX(leftNode);
				ret =ret && leftNode->compile(runTarget, intermediate);
				AST_DETECT_ERROR_AND_SET(ret, leftNode);
			}
			if (rightNode)
			{
				AST_SET_NODE_INDEX(rightNode);
				ret = ret && rightNode->compile(runTarget, intermediate);
				AST_DETECT_ERROR_AND_SET(ret, rightNode);
			}
			ret = ret && emit(runTarget, intermediate);
			return ret;
		}

		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			(void)runTarget;
			(void)intermediate;
			return true;
		}
		virtual ~ExprNodeBase() {}
		Opcode op;
		ExprNodeBase* leftNode;
		ExprNodeBase* rightNode;
		ASTErrorInfo  lastErrorInfo;
		int nodeIndexInTree;
	};

	struct BinaryNode : ExprNodeBase {
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override{
		
			switch (op)
			{
			case Render::VM::Opcode::Add:
			case Render::VM::Opcode::Sub:
			case Render::VM::Opcode::Mul:
			case Render::VM::Opcode::Div:
			case Render::VM::Opcode::Mod:
			case Render::VM::Opcode::BitAnd:
			case Render::VM::Opcode::BitOr:
			case Render::VM::Opcode::BitXor:
			case Render::VM::Opcode::Shl:
			case Render::VM::Opcode::Shr:
			case Render::VM::Opcode::Equal:
			case Render::VM::Opcode::NotEqual:
			case Render::VM::Opcode::Greater:
			case Render::VM::Opcode::GreaterEqual:
			case Render::VM::Opcode::Less:
			case Render::VM::Opcode::LessEqual:
			{
				//stack pop
				//stack pop 
				//stack push
				intermediate->addInstruction(op, 0);
				intermediate->stackPop();
				return true;
				break;
			}
			default:
				AST_SET_ERROR(ASTError::OP_NOT_MATCH)
				return false;
				break;
			}
			return true;

		}
	};

	struct UnaryNode : ExprNodeBase {
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			
			switch (op)
			{
			case Render::VM::Opcode::Neg:
			case Render::VM::Opcode::BitNot:
			case Render::VM::Opcode::LogNot: {
				//Stack pop
				//Stack push
				intermediate->addInstruction(op, 0);
				return true;
				break;
			}
			default:
				AST_SET_ERROR(ASTError::OP_NOT_MATCH)
				return false;
			}
			return true;
		}
	};

	struct IfNode : ExprNodeBase {

		bool compile(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			
			{
				AST_SET_NODE_INDEX(conditionNode);
				bool ret = conditionNode->compile(runTarget, intermediate);
				AST_DETECT_ERROR_AND_SET(ret, conditionNode);
			}
			ASTCompileIntermediate::InstructionRef insRefToElse = intermediate->addInstruction(Opcode::JumpIfFalse, 0);
			//Stack pop
			intermediate->stackPop();

			ASTCompileIntermediate::InstructionRef outOfIf;

			if (leftNode){
				intermediate->enterScope();
				AST_SET_NODE_INDEX(leftNode);
				bool ret = leftNode->compile(runTarget, intermediate);
				intermediate->leaveScope();
				AST_DETECT_ERROR_AND_SET(ret, leftNode);
			}

			if (rightNode) {
				outOfIf = intermediate->addInstruction(Opcode::Jump, 0);
			}

			auto nxtInsLoc = intermediate->getNextInsLoc();
			insRefToElse.changeOprand(nxtInsLoc);

			//else
			if (rightNode) {
				intermediate->enterScope();
				AST_SET_NODE_INDEX(rightNode);
				bool ret = rightNode->compile(runTarget, intermediate);
				intermediate->leaveScope();
				AST_DETECT_ERROR_AND_SET(ret, rightNode);
				outOfIf.changeOprand(intermediate->getNextInsLoc());
			}
			return true;
		}
		//left -> stmt if 
		//right -> stmt else
		ExprNodeBase* conditionNode = nullptr;
	};

	struct AssignmentNode : ExprNodeBase {
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			ParamID param;
			bool isParam = intermediate->citeParam(targetVar,param);
			if (isParam) {
				intermediate->addInstruction(Opcode::StoreParam, param);
				intermediate->stackPop();
				return true;
			}
			uint32_t offset = 0;
			bool isLocal = intermediate->citeLocalVariable(targetVar, offset);
			if (isLocal) {
				intermediate->addInstruction(Opcode::StoreLocal, offset);
				intermediate->stackPop();
				return true;
			}

			//Add a local variable 
			//Stack has been reserved by right expr
			intermediate->addLocalVar(targetVar);
			return true;
		}

		std::string targetVar;
	};

	struct LiteralNode : ExprNodeBase {
		Operand operand;

		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			
			
			if (operand.tag == ParamType::Int &&
				//Def of operand
				operand.val._i <= INSTRUCTION_CONSTANT_MAX && operand.val._i>= 0
			) {
				intermediate->addInstruction(Opcode::PushImm, operand.val._i);
			}
			else {
				ConstantID constant = intermediate->addConstant(runTarget, operand);
				intermediate->addInstruction(Opcode::Push, constant);
			}
			intermediate->stackPush();
			return true;
		}
	};

	struct VariableNode : ExprNodeBase {
		std::string targetVar;
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			ParamID param;
			bool isParam = intermediate->citeParam(targetVar, param);
			if (isParam) {
				intermediate->addInstruction(Opcode::LoadParam, param);
				intermediate->stackPush();
				return true;
			}
			uint32_t offset = 0;
			bool isLocal = intermediate->citeLocalVariable(targetVar, offset);
			if (isLocal) {
				intermediate->addInstruction(Opcode::LoadLocal, offset);
				intermediate->stackPush();
				return true;
			}

			//This Variable is not found????   
			AST_SET_ERROR(ASTError::VAR_NOT_FOUND);
			return false;
		}
	};


	struct ParamDeclareNode : ExprNodeBase {
		std::string targetParam;
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			ParamID paramId;

			auto itor = intermediate->outerParam.find(targetParam);
			if (itor != intermediate->outerParam.end()) {
				//multi-out param define
				//But its ok
				return true;
			}

			Name paramName(targetParam);
			paramId = runTarget->symbols.paramMap.size();
			runTarget->symbols.paramMap.push_back(paramName);
			runTarget->symbols.paramIndexMap.insert({ paramName, paramId });
			intermediate->outerParam.insert({targetParam,paramId});
			return true;
		}
	};

	struct StatementBlockNode : ExprNodeBase {
		ExprNodeBase* statement = nullptr;
		virtual bool emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			intermediate->enterScope();
			if (statement) {
				bool ret = statement->compile(runTarget, intermediate);
				AST_DETECT_ERROR_AND_SET(ret, statement);
			}
			intermediate->leaveScope();
			return true;
		}
	};

	void compileAST(ExprNodeBase* entry, VMRunTarget** outRunTarget);

}
#endif //AST_H_