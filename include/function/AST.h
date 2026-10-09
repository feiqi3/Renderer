#ifndef AST_H_
#define AST_H_
#include "common/CommonMath.h"
#include "common/Name.h"
#include "function/VMDef.h"
#include "function/VMInstruction.h"
#include <map>
namespace Render::VM {
	struct VMRunTarget;
}

namespace Render::VM::AST {
	enum class ASTError {
		NONE,
		VAR_NOT_FOUND
	};

	struct ASTCompileIntermediate {
		//uint32_t stackBase[64];
		uint32_t curStackBase;
		uint32_t curStackPtr;
		std::map<std::string, uint32_t> localVariables; //name, offset to bp
		std::map<std::string, uint32_t> outerParam; //name, index (place inside symbol table)
		std::map<ConstantID, uint32_t> constantMap; //Constant value -> constant ID;
		std::vector<Instruction> instructionBuilder;
		ASTError lastError = ASTError::NONE;
		struct InstrctionRef {
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

		InstrctionRef addInstruction(Opcode op, uint32_t imm) {
			if (imm > INSTRUCTION_CONSTANT_MAX) {
				assert(false);
				*(int*)0;
			} //Max
			
			Instruction ins{};
			ins.op = op;
			ins.opnd = imm;

			instructionBuilder.push_back(ins);

			InstrctionRef ref{};
			ref.intermediate = this;
			ref.insPos = instructionBuilder.size() - 1;
			return ref;
		}

		void addLocalVar(const std::string& var) {
			localVariables.insert({ var,curStackPtr });
		}
		void stackPush() {
			curStackPtr++;
		}
		void stackPop() {
			assert(curStackPtr - 1 >= 0);
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
			offsetFromCurSP = curStackPtr - targetStackPTR;
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
			auto itor = this->constantMap.find(constant.val._u);
			if (itor != constantMap.end()) {
				return itor->second;
			}

			Operand opnd;
			runTarget->symbols.constantMap.push_back(opnd);
			ConstantID constantID = runTarget->symbols.constantMap.size() - 1;
			//Whattttttt?     
			this->constantMap.insert({ constant.val._u,constantID});
			return constantID;
		}
	};

	struct ExprNodeBase {
		ExprNodeBase():leftNode(nullptr),rightNode(nullptr), op(Opcode::Nop){}

		virtual void compile(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			//Post order

			if(leftNode)
				leftNode->compile(runTarget, intermediate);
			if (rightNode) 
				rightNode->compile(runTarget, intermediate);

			emit(runTarget, intermediate);
		}

		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			(void)runTarget;
			(void)intermediate;
		}
		~ExprNodeBase() {}

		Opcode op;
		ExprNodeBase* leftNode;
		ExprNodeBase* rightNode;
	};

	struct BinaryNode : ExprNodeBase {
		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override{
		
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
			case Render::VM::Opcode::BitNot:
			case Render::VM::Opcode::Shl:
			case Render::VM::Opcode::Shr:
			case Render::VM::Opcode::Equal:
			case Render::VM::Opcode::NotEqual:
			case Render::VM::Opcode::Greater:
			case Render::VM::Opcode::GreaterEqual:
			case Render::VM::Opcode::Less:
			case Render::VM::Opcode::LessEqual:
			{
				intermediate->addInstruction(op, 0);
				intermediate->stackPop();
				break;
			}
			default:
				assert(false);
				break;
			}
		
		}
	};

	struct UnaryNode : ExprNodeBase {
		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			
			switch (op)
			{
			case Render::VM::Opcode::Neg:
			case Render::VM::Opcode::BitNot:
			case Render::VM::Opcode::LogNot: {
				intermediate->addInstruction(op, 0);
				break;
			}
			default:
				assert(false);
				break;
			}
		}
	};

	struct IfNode : ExprNodeBase {

		void compile(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) {
			
			conditionNode->compile(runTarget, intermediate);
			
			ASTCompileIntermediate::InstrctionRef insRefToElse = intermediate->addInstruction(Opcode::JumpIfFalse, 0);
			intermediate->stackPop();
			ASTCompileIntermediate::InstrctionRef outOfIf;

			if (leftNode){
				leftNode->compile(runTarget, intermediate);

			}

			if (rightNode) {
				outOfIf = intermediate->addInstruction(Opcode::Jump, 0);
			}

			auto nxtInsLoc = intermediate->getNextInsLoc();
			insRefToElse.changeOprand(nxtInsLoc);

			//else
			if (rightNode) {

				rightNode->compile(runTarget, intermediate);
				outOfIf.changeOprand(intermediate->getNextInsLoc());
			}

		}
		//left -> stmt if 
		//right -> stmt else
		ExprNodeBase* conditionNode = nullptr;
	};

	struct AssignmentNode : ExprNodeBase {
		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			ParamID param;
			bool isParam = intermediate->citeParam(targetVar,param);
			if (isParam) {
				intermediate->addInstruction(Opcode::StoreParam, param);
				intermediate->stackPop();
				return;
			}
			uint32_t offset = 0;
			bool isLocal = intermediate->citeLocalVariable(targetVar, offset);
			if (isLocal) {
				intermediate->addInstruction(Opcode::StoreLocal, offset);
				intermediate->stackPop();
				return;
			}

			//Add a local variable 
			intermediate->addLocalVar(targetVar);
			intermediate->addInstruction(Opcode::StoreLocal, offset);
			intermediate->stackPush();
		}

		std::string targetVar;
	};

	struct LiteralNode : ExprNodeBase {
		Operand operand;

		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			
			
			if (operand.tag == ParamType::Int &&
				//Def of operand
				operand.val._i <= INSTRUCTION_CONSTANT_MAX
			) {
				intermediate->addInstruction(Opcode::PushImm, operand.val._i);
			}
			else {
				ConstantID constant = intermediate->addConstant(runTarget, operand);
				intermediate->addInstruction(Opcode::Push, constant);
			}
			intermediate->stackPush();

		}
	};

	struct VariableNode : ExprNodeBase {
		std::string targetVar;
		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			ParamID param;
			bool isParam = intermediate->citeParam(targetVar, param);
			if (isParam) {
				intermediate->addInstruction(Opcode::LoadParam, param);
				intermediate->stackPush();
				return;
			}
			uint32_t offset = 0;
			bool isLocal = intermediate->citeLocalVariable(targetVar, offset);
			if (isLocal) {
				intermediate->addInstruction(Opcode::LoadLocal, offset);
				intermediate->stackPush();
				return;
			}

			//This Variable is not found????   
			intermediate->lastError = ASTError::VAR_NOT_FOUND;
			
		}
	};

	struct StmtNode : ExprNodeBase {
		virtual void emit(VMRunTarget* runTarget, ASTCompileIntermediate* intermediate) override {
			//DO NOTHING
		}
	};
}
#endif //AST_H_