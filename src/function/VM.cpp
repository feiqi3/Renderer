#include "function/VM.h"
#include "function/VMInstruction.h"
namespace Render::VM {
	namespace {




		struct OperandStack {
			static const uint32_t STACK_SIZE = 64;

			Operand* stack;
			uint32_t stackPtr;
			uint32_t stackSize;
			ErrorCode lastError = ErrorCode::NONE;

			explicit OperandStack(uint32_t stackSize = STACK_SIZE)
				: stackSize(stackSize), stackPtr(0), lastError(ErrorCode::NONE) {
				stack = new Operand[stackSize];
			}

			~OperandStack() {
				delete[] stack;
				stack = nullptr;
			}
			OperandStack(const OperandStack&) = delete;
			OperandStack& operator=(const OperandStack&) = delete;
			void reset() {
				stackPtr = 0;
				lastError = ErrorCode::NONE;
			}
			bool push(const Operand& val) {
				if (stackPtr >= stackSize) {
					lastError = ErrorCode::STACK_OVERFLOW;
					return false;
				}
				stack[stackPtr++] = val;
				return true;
			}

			Operand pop() {
				if (stackPtr == 0) {
					lastError = ErrorCode::STACK_REACHBUTTOM;
					return Operand{};
				}
				return stack[--stackPtr];
			}

			Operand load(uint32_t offset) {
				if (offset >= stackPtr) {
					lastError = ErrorCode::STACK_REACHBUTTOM;
					return Operand{};
				}
				return stack[stackPtr - 1 - offset];
			}

			bool set(uint32_t offset, const Operand& val) {
				if (offset >= stackPtr) {
					lastError = ErrorCode::STACK_REACHBUTTOM;
					return false;
				}
				stack[stackPtr - 1 - offset] = val;
				return true;
			}

			bool has_error() const {
				return lastError != ErrorCode::NONE;
			}

		};


		struct InstructionRunner {
			InstructionRunner(OperandStack* stack, InstructionList* ins, SymbolTable* symbolTable, VMFuncs inFunc = {});
			void run();
			bool isRunning()const;

			void nop();
			void push(uint32_t val);
			void pushImm(uint32_t val);
			void pop(uint32_t val);
			void dup(uint32_t val);
			void load_local(uint32_t val);
			void store_local(uint32_t val);
			void load_param(uint32_t val);
			void store_param(uint32_t val);

			void add(uint32_t val);
			void sub(uint32_t val);
			void mul(uint32_t val);
			void div(uint32_t val);
			void mod(uint32_t val);
			void neg(uint32_t val);

			void bit_and(uint32_t val);
			void bit_or(uint32_t val);
			void bit_xor(uint32_t val);
			void bit_not(uint32_t val);
			void shl(uint32_t val);
			void shr(uint32_t val);

			void equal(uint32_t val);
			void not_equal(uint32_t val);
			void greater(uint32_t val);
			void greater_equal(uint32_t val);
			void less(uint32_t val);
			void less_equal(uint32_t val);

			void log_not(uint32_t val);
			void jump(uint32_t val);
			void jump_if_false(uint32_t val);
			void jump_if_true(uint32_t val);

			void halt();
			bool getInstruction(Instruction& out);
			void setInstructionPtr(uint32_t ptr);
			VMFuncs userFunc;
			OperandStack* stack;
			InstructionList* instructions;
			SymbolTable* table;
			uint32_t instructionsPtr;
			bool isVMRunning = false;
		};

		InstructionRunner::InstructionRunner(OperandStack* stack, InstructionList* ins, SymbolTable* symbolTable, VMFuncs inFunc)
			: stack(stack), userFunc(inFunc), instructions(ins), table(symbolTable), instructionsPtr(0) {
		}

		void InstructionRunner::setInstructionPtr(uint32_t ptr) {
			instructionsPtr = ptr;
		}

		bool InstructionRunner::getInstruction(Instruction& out) {
			if (instructionsPtr >= instructions->instructionNum) {
				return false;
			}
			out = instructions->list[instructionsPtr++];
			return true;
		}

		void InstructionRunner::nop() {}

		void InstructionRunner::push(uint32_t val) {
			assert(val < table->constantMap.size());
			stack->push(table->constantMap[val]);
		}

		void InstructionRunner::pushImm(uint32_t val) {
			Operand opnd;
			opnd.tag = ParamType::Int;
			opnd.val._i = static_cast<int>(val);
			stack->push(opnd);
		}

		void InstructionRunner::pop(uint32_t val) {
			for (uint32_t i = 0; i < val; ++i) {
				stack->pop();
			}
		}

		void InstructionRunner::dup(uint32_t val) {
			auto opnd = stack->pop();
			if (stack->has_error()) return;
			for (uint32_t i = 0; i < val + 1; ++i) {
				stack->push(opnd);
			}
		}

		void InstructionRunner::load_local(uint32_t val) {
			auto opnd = stack->load(val);
			stack->push(opnd);
		}

		void InstructionRunner::store_local(uint32_t val) {
			auto opnd = stack->load(0);
			stack->set(val, opnd);
			stack->pop();
		}

		void InstructionRunner::load_param(uint32_t val) {
			assert(val < table->paramMap.size());
			const auto& name = table->paramMap[val];
			uint32_t getVal;
			ParamType type;
			userFunc.getFunc(name, &getVal, type);

			Operand opnd;
			opnd.tag = type;
			opnd.val._u = getVal;
			stack->push(opnd);
		}


		void InstructionRunner::store_param(uint32_t val)
		{
			assert(val < table->paramMap.size());
			const auto& name = table->paramMap[val];

			auto opnd = stack->pop();
			userFunc.setFunc(name, opnd.val._u, opnd.tag);

		}

		void InstructionRunner::add(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.tag = ParamType::Float;
				newOpnd.val._f = opnd2.asFloat() + opnd1.asFloat();
			}
			else {
				newOpnd.tag = ParamType::Int;
				newOpnd.val._i = opnd2.asInt() + opnd1.asInt();
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::sub(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.tag = ParamType::Float;
				newOpnd.val._f = opnd2.asFloat() - opnd1.asFloat();
			}
			else {
				newOpnd.tag = ParamType::Int;
				newOpnd.val._i = opnd2.asInt() - opnd1.asInt();
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::mul(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.tag = ParamType::Float;
				newOpnd.val._f = opnd2.asFloat() * opnd1.asFloat();
			}
			else {
				newOpnd.tag = ParamType::Int;
				newOpnd.val._i = opnd2.asInt() * opnd1.asInt();
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::div(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.tag = ParamType::Float;
				newOpnd.val._f = opnd2.asFloat() / opnd1.asFloat();
			}
			else {
				newOpnd.tag = ParamType::Int;
				int divisor = opnd1.asInt();
				newOpnd.val._i = (divisor != 0) ? (opnd2.asInt() / divisor) : 0;
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::mod(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			int divisor = opnd1.asInt();
			newOpnd.val._i = (divisor != 0) ? (opnd2.asInt() % divisor) : 0;
			stack->push(newOpnd);
		}

		void InstructionRunner::neg(uint32_t val) {
			auto opnd1 = stack->pop();
			if (opnd1.tag == ParamType::Float) {
				opnd1.val._f = -opnd1.val._f;
			}
			else {
				opnd1.tag = ParamType::Int;
				opnd1.val._i = -opnd1.asInt();
			}
			stack->push(opnd1);
		}

		void InstructionRunner::bit_and(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = opnd2.asInt() & opnd1.asInt();
			stack->push(newOpnd);
		}

		void InstructionRunner::bit_or(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = opnd2.asInt() | opnd1.asInt();
			stack->push(newOpnd);
		}

		void InstructionRunner::bit_xor(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = opnd2.asInt() ^ opnd1.asInt();
			stack->push(newOpnd);
		}

		void InstructionRunner::bit_not(uint32_t val) {
			auto opnd1 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = ~opnd1.asInt();
			stack->push(newOpnd);
		}

		void InstructionRunner::shl(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = opnd2.asInt() << opnd1.asInt();
			stack->push(newOpnd);
		}

		void InstructionRunner::shr(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Int;
			newOpnd.val._i = opnd2.asInt() >> opnd1.asInt();
			stack->push(newOpnd);
		}


		void InstructionRunner::equal(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() == opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() == opnd1.asInt());
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::not_equal(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() != opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() != opnd1.asInt());
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::greater(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() > opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() > opnd1.asInt());
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::greater_equal(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() >= opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() >= opnd1.asInt());
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::less(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() < opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() < opnd1.asInt());
			}
			stack->push(newOpnd);
		}

		void InstructionRunner::less_equal(uint32_t val) {
			auto opnd1 = stack->pop();
			auto opnd2 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;

			if (opnd1.tag == ParamType::Float || opnd2.tag == ParamType::Float) {
				newOpnd.val._b = (opnd2.asFloat() <= opnd1.asFloat());
			}
			else {
				newOpnd.val._b = (opnd2.asInt() <= opnd1.asInt());
			}
			stack->push(newOpnd);
		}


		void InstructionRunner::log_not(uint32_t val) {
			auto opnd1 = stack->pop();
			Operand newOpnd;
			newOpnd.tag = ParamType::Bool;
			newOpnd.val._b = !opnd1.asBool();
			stack->push(newOpnd);
		}

		void InstructionRunner::jump(uint32_t val) {
			setInstructionPtr(val);
		}

		void InstructionRunner::jump_if_false(uint32_t val) {
			auto opnd1 = stack->pop();
			if (!opnd1.asBool()) {
				setInstructionPtr(val);
			}
		}

		void InstructionRunner::jump_if_true(uint32_t val) {
			auto opnd1 = stack->pop();
			if (opnd1.asBool()) {
				setInstructionPtr(val);
			}
		}

		void InstructionRunner::halt() {
			isVMRunning = false;
		}

		bool InstructionRunner::isRunning() const
		{
			return isVMRunning;
		}


		void InstructionRunner::run() {
			Instruction inst;
			isVMRunning = true;

			while (isRunning() && getInstruction(inst)) {
				switch (inst.op) {
				case Opcode::Nop:          nop(); break;
				case Opcode::Push:         push(inst.opnd); break;
				case Opcode::PushImm:      pushImm(inst.opnd); break;
				case Opcode::Pop:          pop(inst.opnd); break;
				case Opcode::Dup:          dup(inst.opnd); break;
				case Opcode::LoadLocal:    load_local(inst.opnd); break;
				case Opcode::StoreLocal:   store_local(inst.opnd); break;
				case Opcode::LoadParam:    load_param(inst.opnd); break;
				case Opcode::StoreParam:   store_param(inst.opnd); break;

				case Opcode::Add:          add(inst.opnd); break;
				case Opcode::Sub:          sub(inst.opnd); break;
				case Opcode::Mul:          mul(inst.opnd); break;
				case Opcode::Div:          div(inst.opnd); break;
				case Opcode::Mod:          mod(inst.opnd); break;
				case Opcode::Neg:          neg(inst.opnd); break;

				case Opcode::BitAnd:       bit_and(inst.opnd); break;
				case Opcode::BitOr:        bit_or(inst.opnd); break;
				case Opcode::BitXor:       bit_xor(inst.opnd); break;
				case Opcode::BitNot:       bit_not(inst.opnd); break;
				case Opcode::Shl:          shl(inst.opnd); break;
				case Opcode::Shr:          shr(inst.opnd); break;

				case Opcode::Equal:        equal(inst.opnd); break;
				case Opcode::NotEqual:     not_equal(inst.opnd); break;
				case Opcode::Greater:      greater(inst.opnd); break;
				case Opcode::GreaterEqual: greater_equal(inst.opnd); break;
				case Opcode::Less:         less(inst.opnd); break;
				case Opcode::LessEqual:    less_equal(inst.opnd); break;

				case Opcode::LogNot:       log_not(inst.opnd); break;
				case Opcode::Jump:         jump(inst.opnd); break;
				case Opcode::JumpIfFalse:  jump_if_false(inst.opnd); break;
				case Opcode::JumpIfTrue:   jump_if_true(inst.opnd); break;

				case Opcode::Halt:
					halt();
					break;

				default:
					isVMRunning = false;
					break;
				}

				if (stack->has_error()) {
					isVMRunning = false;
					break;
				}
			}


		}

		void runVM(VMRunTarget* runTarget, uint32_t stackSize, VMFuncs inFunc)
		{
			OperandStack stack(stackSize);
			InstructionRunner runner(&stack, &runTarget->instructionList, &runTarget->symbols, inFunc);
			runner.run();
		}

	}

