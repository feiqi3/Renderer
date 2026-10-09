#ifndef VM_DEF_H_
#define VM_DEF_H_
#pragma once
namespace Render::VM {
	using ConstantID = uint32_t;
	using ParamID = uint32_t;
	enum class Opcode : uint8_t {
		Nop = 0,
		PushImm,
		Push,
		Pop,
		Dup,
		LoadLocal,      //Get Stack value at (offset from stack top)
		StoreLocal,     //Set Stack value at (offset from stack top)
		LoadParam,
		StoreParam,

		Add,
		Sub,
		Mul,
		Div,
		Mod,
		Neg,

		BitAnd,
		BitOr,
		BitXor,
		BitNot,
		Shl,
		Shr,

		Equal,
		NotEqual,
		Greater,
		GreaterEqual,
		Less,
		LessEqual,

		// Logic control
		LogNot,
		Jump,
		JumpIfFalse,
		JumpIfTrue,

		Halt
	};

	enum class ParamType
	{
		Bool,
		Int,
		Float,
	};


	union OperandVal {
		float	_f;
		int		_i;
		bool	_b;
		u32		_u;
	};

	struct Operand {
		OperandVal val;
		ParamType  tag;

		inline float asFloat() const {
			if (tag == ParamType::Float)return val._f;
			if (tag == ParamType::Bool)return (float)val._b;
			if (tag == ParamType::Int)return (float)val._i;
			return val._f;
		}

		inline int asInt()const {
			if (tag == ParamType::Float)return (int)val._f;
			if (tag == ParamType::Bool)return (int)val._b;
			if (tag == ParamType::Int)return val._i;
			return val._i;
		}

		inline bool asBool()const {
			if (tag == ParamType::Float)return val._f == 0. ? false : true;
			if (tag == ParamType::Bool)return val._b;
			if (tag == ParamType::Int)return (bool)val._i;
			return val._b;
		}
	};

	struct Instruction {
		Opcode op : 8;
		uint32_t opnd : 24;

	};
	const int32_t INSTRUCTION_CONSTANT_MAX = (1 << 25) - 1;

	struct InstructionList {
		Instruction* list = nullptr;
		uint32_t instructionNum = 0;
	};

	enum class ErrorCode {
		NONE,
		STACK_OVERFLOW,
		STACK_REACHBUTTOM,
	};
}
#endif // VM_DEF_H_