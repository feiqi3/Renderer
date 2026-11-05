#include "function/VM.h"
#include "function/AST.h"
#include <iostream>
#include <map>
#include <cassert>
#include <cmath>

using namespace Render::VM;

// ==========================================
// Test Harness for general VM execution and parameter management
// ==========================================
class VMTestHarness {
public:
	std::map<Render::Name, Operand> paramMap;

	void setParam(std::string name, Operand opnd) {
		paramMap[Render::Name(name)] = opnd;
	}

	Operand getParam(std::string name) {
		auto it = paramMap.find(Render::Name(name));
		if (it != paramMap.end()) {
			return it->second;
		}
		assert(false && "Param not found");
		return Operand{};
	}

	bool execute(StatementBlockNode* root, uint32_t stackSize = 256) {
		VMRunTarget* vmTarget = nullptr;
		compileAST(root, &vmTarget);
		if (!vmTarget) {
			return false;
		}

		VMFuncs func{};
		func.getFunc = [this](const Render::Name& name, uint32_t* outVal, ParamType& outType) {
			auto it = paramMap.find(name);
			if (it == paramMap.end()) {
				Operand opnd{};
				opnd.tag = ParamType::Int;
				opnd.val._i = 0;
				paramMap.insert({ name, opnd });
				*outVal = 0;
				outType = opnd.tag;
			}
			else {
				*outVal = it->second.val._u;
				outType = it->second.tag;
			}
			};

		func.setFunc = [this](const Render::Name& name, uint32_t val, ParamType type) {
			auto it = paramMap.find(name);
			if (it == paramMap.end()) {
				Operand opnd{};
				opnd.tag = type;
				opnd.val._u = val;
				paramMap.insert({ name, opnd });
			}
			else {
				it->second.tag = type;
				it->second.val._u = val;
			}
			};

		runVM(vmTarget, stackSize, func);
		destroyVMRunTarget(vmTarget);
		return true;
	}
};

// ==========================================
// Test 1: Compound arithmetic operations and operator precedence
// Target expression: _out = (a + 5) * (b - 2)
// Example: a = 10, b = 6 => (10 + 5) * (6 - 2) = 15 * 4 = 60
// ==========================================
void test_arithmetic_compound() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	ParamDeclareNode pA;   pA.targetParam = "a";
	ParamDeclareNode pB;   pB.targetParam = "b";
	root.statments.push_back(&pOut);
	root.statments.push_back(&pA);
	root.statments.push_back(&pB);

	// Left subtree: a + 5
	VariableNode varA; varA.targetVar = "a";
	LiteralNode lit5;  lit5.operand.tag = ParamType::Int; lit5.operand.val._i = 5;
	BinaryNode addNode;
	addNode.op = Opcode::Add;
	addNode.leftNode = &varA;
	addNode.rightNode = &lit5;

	// Right subtree: b - 2
	VariableNode varB; varB.targetVar = "b";
	LiteralNode lit2;  lit2.operand.tag = ParamType::Int; lit2.operand.val._i = 2;
	BinaryNode subNode;
	subNode.op = Opcode::Sub;
	subNode.leftNode = &varB;
	subNode.rightNode = &lit2;

	// Root mul: (a + 5) * (b - 2)
	BinaryNode mulNode;
	mulNode.op = Opcode::Mul;
	mulNode.leftNode = &addNode;
	mulNode.rightNode = &subNode;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &mulNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	Operand opA{}; opA.tag = ParamType::Int; opA.val._i = 10;
	Operand opB{}; opB.tag = ParamType::Int; opB.val._i = 6;
	harness.setParam("a", opA);
	harness.setParam("b", opB);

	harness.execute(&root);
	int res = harness.getParam("_out").asInt();
	assert(res == 60);
	std::cout << "[PASS] test_arithmetic_compound (Result: " << res << ")\n";
}

// ==========================================
// Test 2: Mixed floating-point operations and relational comparisons
// Target expression: _out = (x / 2.0f) >= 2.5f
// Example: x = 6.0f => 6.0 / 2.0 = 3.0 >= 2.5 => true
// ==========================================
void test_float_and_comparison() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	ParamDeclareNode pX;   pX.targetParam = "x";
	root.statments.push_back(&pOut);
	root.statments.push_back(&pX);

	VariableNode varX; varX.targetVar = "x";
	LiteralNode litDiv; litDiv.operand.tag = ParamType::Float; litDiv.operand.val._f = 2.0f;
	BinaryNode divNode;
	divNode.op = Opcode::Div;
	divNode.leftNode = &varX;
	divNode.rightNode = &litDiv;

	LiteralNode litCmp; litCmp.operand.tag = ParamType::Float; litCmp.operand.val._f = 2.5f;
	BinaryNode cmpNode;
	cmpNode.op = Opcode::GreaterEqual;
	cmpNode.leftNode = &divNode;
	cmpNode.rightNode = &litCmp;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &cmpNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	Operand opX{}; opX.tag = ParamType::Float; opX.val._f = 6.0f;
	harness.setParam("x", opX);

	harness.execute(&root);
	bool res = harness.getParam("_out").asBool();
	assert(res == true);
	std::cout << "[PASS] test_float_and_comparison (Result: " << (res ? "true" : "false") << ")\n";
}

// ==========================================
// Test 3: Bitwise operations and bit shifts
// Target expression: _out = (x << 2) | (y & 3)
// Example: x = 3 (0b0011 -> 0b1100 = 12), y = 7 (0b0111 & 0b0011 = 3) => 12 | 3 = 15
// ==========================================
void test_bitwise_operations() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	ParamDeclareNode pX;   pX.targetParam = "x";
	ParamDeclareNode pY;   pY.targetParam = "y";
	root.statments.push_back(&pOut);
	root.statments.push_back(&pX);
	root.statments.push_back(&pY);

	// x << 2
	VariableNode varX; varX.targetVar = "x";
	LiteralNode lit2; lit2.operand.tag = ParamType::Int; lit2.operand.val._i = 2;
	BinaryNode shlNode;
	shlNode.op = Opcode::Shl;
	shlNode.leftNode = &varX;
	shlNode.rightNode = &lit2;

	// y & 3
	VariableNode varY; varY.targetVar = "y";
	LiteralNode lit3; lit3.operand.tag = ParamType::Int; lit3.operand.val._i = 3;
	BinaryNode andNode;
	andNode.op = Opcode::BitAnd;
	andNode.leftNode = &varY;
	andNode.rightNode = &lit3;

	// (x << 2) | (y & 3)
	BinaryNode orNode;
	orNode.op = Opcode::BitOr;
	orNode.leftNode = &shlNode;
	orNode.rightNode = &andNode;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &orNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	Operand opX{}; opX.tag = ParamType::Int; opX.val._i = 3;
	Operand opY{}; opY.tag = ParamType::Int; opY.val._i = 7;
	harness.setParam("x", opX);
	harness.setParam("y", opY);

	harness.execute(&root);
	int res = harness.getParam("_out").asInt();
	assert(res == 15);
	std::cout << "[PASS] test_bitwise_operations (Result: " << res << ")\n";
}

// ==========================================
// Test 4: Nested If-Else branches and jump target backpatching
// Logic:
// if (score >= 60) {
//     if (score >= 90) { grade = 1; } // A
//     else { grade = 2; }             // B
// } else {
//     grade = 3;                     // Fail
// }
// Testing with score = 75 => Expected grade = 2
// ==========================================
void test_nested_if_else() {
	StatementBlockNode root;

	ParamDeclareNode pScore; pScore.targetParam = "score";
	ParamDeclareNode pGrade; pGrade.targetParam = "grade";
	root.statments.push_back(&pScore);
	root.statments.push_back(&pGrade);

	// Outer If
	IfNode outerIf;
	root.statments.push_back(&outerIf);

	// condition: score >= 60
	VariableNode varScoreOuter; varScoreOuter.targetVar = "score";
	LiteralNode lit60; lit60.operand.tag = ParamType::Int; lit60.operand.val._i = 60;
	BinaryNode condOuter;
	condOuter.op = Opcode::GreaterEqual;
	condOuter.leftNode = &varScoreOuter;
	condOuter.rightNode = &lit60;
	outerIf.conditionNode = &condOuter;

	// Outer True Block (Contains Inner If)
	StatementBlockNode outerTrueBlock;
	outerIf.leftNode = &outerTrueBlock;

	IfNode innerIf;
	outerTrueBlock.statments.push_back(&innerIf);

	// condition: score >= 90
	VariableNode varScoreInner; varScoreInner.targetVar = "score";
	LiteralNode lit90; lit90.operand.tag = ParamType::Int; lit90.operand.val._i = 90;
	BinaryNode condInner;
	condInner.op = Opcode::GreaterEqual;
	condInner.leftNode = &varScoreInner;
	condInner.rightNode = &lit90;
	innerIf.conditionNode = &condInner;

	// Inner True Block: grade = 1
	StatementBlockNode innerTrueBlock;
	innerIf.leftNode = &innerTrueBlock;
	AssignmentNode assignG1;
	assignG1.targetVar = "grade";
	LiteralNode litG1; litG1.operand.tag = ParamType::Int; litG1.operand.val._i = 1;
	assignG1.rightNode = &litG1;
	innerTrueBlock.statments.push_back(&assignG1);

	// Inner False Block: grade = 2
	StatementBlockNode innerFalseBlock;
	innerIf.rightNode = &innerFalseBlock;
	AssignmentNode assignG2;
	assignG2.targetVar = "grade";
	LiteralNode litG2; litG2.operand.tag = ParamType::Int; litG2.operand.val._i = 2;
	assignG2.rightNode = &litG2;
	innerFalseBlock.statments.push_back(&assignG2);

	// Outer False Block: grade = 3
	StatementBlockNode outerFalseBlock;
	outerIf.rightNode = &outerFalseBlock;
	AssignmentNode assignG3;
	assignG3.targetVar = "grade";
	LiteralNode litG3; litG3.operand.tag = ParamType::Int; litG3.operand.val._i = 3;
	assignG3.rightNode = &litG3;
	outerFalseBlock.statments.push_back(&assignG3);

	VMTestHarness harness;
	Operand opScore{}; opScore.tag = ParamType::Int; opScore.val._i = 75;
	harness.setParam("score", opScore);

	harness.execute(&root);
	int grade = harness.getParam("grade").asInt();
	assert(grade == 2);
	std::cout << "[PASS] test_nested_if_else (Result: " << grade << ")\n";
}

// ==========================================
// Test 5: Modulo operations and arithmetic boundaries
// Target expression: _out = (a % b)
// Example: a = 17, b = 5 => 17 % 5 = 2
// ==========================================
void test_modulo_operation() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	ParamDeclareNode pDiv1; pDiv1.targetParam = "a";
	ParamDeclareNode pDiv2; pDiv2.targetParam = "b";
	root.statments.push_back(&pOut);
	root.statments.push_back(&pDiv1);
	root.statments.push_back(&pDiv2);

	VariableNode varA; varA.targetVar = "a";
	VariableNode varB; varB.targetVar = "b";
	BinaryNode modNode;
	modNode.op = Opcode::Mod;
	modNode.leftNode = &varA;
	modNode.rightNode = &varB;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &modNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	Operand opA{}; opA.tag = ParamType::Int; opA.val._i = 17;
	Operand opB{}; opB.tag = ParamType::Int; opB.val._i = 5;
	harness.setParam("a", opA);
	harness.setParam("b", opB);

	harness.execute(&root);
	int res = harness.getParam("_out").asInt();
	assert(res == 2);
	std::cout << "[PASS] test_modulo_operation (Result: " << res << ")\n";
}

// =========================================================================
// Category 1: Control Flow (Multi-Branch Cascading & Flow Convergence)
// =========================================================================

// Test 1.1: Multi-way branch chain (if - else if - else)
// Logic:
//   if (score >= 90)      { grade = 1; }
//   else if (score >= 70) { grade = 2; }
//   else if (score >= 50) { grade = 3; }
//   else                  { grade = 4; }
// Verifies: Jump address backpatching prevents falling through into subsequent branches.
void test_cascaded_if_else_chain() {
	StatementBlockNode root;

	ParamDeclareNode pScore; pScore.targetParam = "score";
	ParamDeclareNode pGrade; pGrade.targetParam = "grade";
	root.statments.push_back(&pScore);
	root.statments.push_back(&pGrade);

	// Level 1: if (score >= 90)
	IfNode ifL1;
	root.statments.push_back(&ifL1);

	VariableNode varScore1; varScore1.targetVar = "score";
	LiteralNode lit90; lit90.operand.tag = ParamType::Int; lit90.operand.val._i = 90;
	BinaryNode condL1;
	condL1.op = Opcode::GreaterEqual;
	condL1.leftNode = &varScore1;
	condL1.rightNode = &lit90;
	ifL1.conditionNode = &condL1;

	// Level 1 True Block: grade = 1
	StatementBlockNode thenL1;
	ifL1.leftNode = &thenL1;
	AssignmentNode assign1;
	assign1.targetVar = "grade";
	LiteralNode val1; val1.operand.tag = ParamType::Int; val1.operand.val._i = 1;
	assign1.rightNode = &val1;
	thenL1.statments.push_back(&assign1);

	// Level 1 False Block: Contains Level 2 If
	StatementBlockNode elseL1;
	ifL1.rightNode = &elseL1;

	// Level 2: if (score >= 70)
	IfNode ifL2;
	elseL1.statments.push_back(&ifL2);

	VariableNode varScore2; varScore2.targetVar = "score";
	LiteralNode lit70; lit70.operand.tag = ParamType::Int; lit70.operand.val._i = 70;
	BinaryNode condL2;
	condL2.op = Opcode::GreaterEqual;
	condL2.leftNode = &varScore2;
	condL2.rightNode = &lit70;
	ifL2.conditionNode = &condL2;

	// Level 2 True Block: grade = 2
	StatementBlockNode thenL2;
	ifL2.leftNode = &thenL2;
	AssignmentNode assign2;
	assign2.targetVar = "grade";
	LiteralNode val2; val2.operand.tag = ParamType::Int; val2.operand.val._i = 2;
	assign2.rightNode = &val2;
	thenL2.statments.push_back(&assign2);

	// Level 2 False Block: Contains Level 3 If
	StatementBlockNode elseL2;
	ifL2.rightNode = &elseL2;

	// Level 3: if (score >= 50)
	IfNode ifL3;
	elseL2.statments.push_back(&ifL3);

	VariableNode varScore3; varScore3.targetVar = "score";
	LiteralNode lit50; lit50.operand.tag = ParamType::Int; lit50.operand.val._i = 50;
	BinaryNode condL3;
	condL3.op = Opcode::GreaterEqual;
	condL3.leftNode = &varScore3;
	condL3.rightNode = &lit50;
	ifL3.conditionNode = &condL3;

	// Level 3 True Block: grade = 3
	StatementBlockNode thenL3;
	ifL3.leftNode = &thenL3;
	AssignmentNode assign3;
	assign3.targetVar = "grade";
	LiteralNode val3; val3.operand.tag = ParamType::Int; val3.operand.val._i = 3;
	assign3.rightNode = &val3;
	thenL3.statments.push_back(&assign3);

	// Level 3 False Block (Final Fallback): grade = 4
	StatementBlockNode elseL3;
	ifL3.rightNode = &elseL3;
	AssignmentNode assign4;
	assign4.targetVar = "grade";
	LiteralNode val4; val4.operand.tag = ParamType::Int; val4.operand.val._i = 4;
	assign4.rightNode = &val4;
	elseL3.statments.push_back(&assign4);

	// Subtest A: score = 75 (should hit grade = 2)
	{
		VMTestHarness harness;
		Operand op{}; op.tag = ParamType::Int; op.val._i = 75;
		harness.setParam("score", op);
		harness.execute(&root);
		assert(harness.getParam("grade").asInt() == 2);
	}

	// Subtest B: score = 30 (should hit grade = 4)
	{
		VMTestHarness harness;
		Operand op{}; op.tag = ParamType::Int; op.val._i = 30;
		harness.setParam("score", op);
		harness.execute(&root);
		assert(harness.getParam("grade").asInt() == 4);
	}

	std::cout << "[PASS] test_cascaded_if_else_chain\n";
}

// Test 1.2: Diamond control flow convergence
// Logic:
//   local temp = in_val;
//   if (temp > 10) { temp = temp - 5; }
//   else           { temp = temp + 5; }
//   _out = temp * 2;
// Verifies: Stack alignment and local variable stability when both branches converge.
void test_diamond_flow_convergence() {
	StatementBlockNode root;

	ParamDeclareNode pIn;  pIn.targetParam = "in_val";
	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pIn);
	root.statments.push_back(&pOut);

	// local temp = in_val
	AssignmentNode assignTemp;
	assignTemp.targetVar = "temp";
	VariableNode varIn; varIn.targetVar = "in_val";
	assignTemp.rightNode = &varIn;
	root.statments.push_back(&assignTemp);

	// if (temp > 10)
	IfNode ifNode;
	root.statments.push_back(&ifNode);

	VariableNode varTempCond; varTempCond.targetVar = "temp";
	LiteralNode lit10; lit10.operand.tag = ParamType::Int; lit10.operand.val._i = 10;
	BinaryNode condNode;
	condNode.op = Opcode::Greater;
	condNode.leftNode = &varTempCond;
	condNode.rightNode = &lit10;
	ifNode.conditionNode = &condNode;

	// True branch: temp = temp - 5
	StatementBlockNode thenBlock;
	ifNode.leftNode = &thenBlock;
	AssignmentNode assignThen;
	assignThen.targetVar = "temp";
	VariableNode varTempThen; varTempThen.targetVar = "temp";
	LiteralNode lit5; lit5.operand.tag = ParamType::Int; lit5.operand.val._i = 5;
	BinaryNode subNode;
	subNode.op = Opcode::Sub;
	subNode.leftNode = &varTempThen;
	subNode.rightNode = &lit5;
	assignThen.rightNode = &subNode;
	thenBlock.statments.push_back(&assignThen);

	// False branch: temp = temp + 5
	StatementBlockNode elseBlock;
	ifNode.rightNode = &elseBlock;
	AssignmentNode assignElse;
	assignElse.targetVar = "temp";
	VariableNode varTempElse; varTempElse.targetVar = "temp";
	BinaryNode addNode;
	addNode.op = Opcode::Add;
	addNode.leftNode = &varTempElse;
	addNode.rightNode = &lit5;
	assignElse.rightNode = &addNode;
	elseBlock.statments.push_back(&assignElse);

	// Common tail statement: _out = temp * 2
	AssignmentNode assignOut;
	assignOut.targetVar = "_out";
	VariableNode varTempTail; varTempTail.targetVar = "temp";
	LiteralNode lit2; lit2.operand.tag = ParamType::Int; lit2.operand.val._i = 2;
	BinaryNode mulNode;
	mulNode.op = Opcode::Mul;
	mulNode.leftNode = &varTempTail;
	mulNode.rightNode = &lit2;
	assignOut.rightNode = &mulNode;
	root.statments.push_back(&assignOut);

	// Test Path A: in_val = 15 => temp = 10 => _out = 20
	{
		VMTestHarness harness;
		Operand op{}; op.tag = ParamType::Int; op.val._i = 15;
		harness.setParam("in_val", op);
		harness.execute(&root);
		assert(harness.getParam("_out").asInt() == 20);
	}

	// Test Path B: in_val = 4  => temp = 9  => _out = 18
	{
		VMTestHarness harness;
		Operand op{}; op.tag = ParamType::Int; op.val._i = 4;
		harness.setParam("in_val", op);
		harness.execute(&root);
		assert(harness.getParam("_out").asInt() == 18);
	}

	std::cout << "[PASS] test_diamond_flow_convergence\n";
}

// =========================================================================
// Category 2: Short-Circuiting & Side Effects
// =========================================================================

// Test 2.1: Guarded conditional bypassing side-effect statements
// Logic:
//   _out = 42; // Initial state
//   local guard = false;
//   local side_effect = 100;
//   if (guard == true) {
//       if (side_effect > 50) {
//           _out = 999; // Dangerous side-effect that must NOT execute
//       }
//   }
// Verifies: When outer guard is false, nested side-effects are completely bypassed.
void test_guarded_side_effect_bypass() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	// Initialize _out = 42
	AssignmentNode initOut;
	initOut.targetVar = "_out";
	LiteralNode val42; val42.operand.tag = ParamType::Int; val42.operand.val._i = 42;
	initOut.rightNode = &val42;
	root.statments.push_back(&initOut);

	// local guard = false
	AssignmentNode assignGuard;
	assignGuard.targetVar = "guard";
	LiteralNode litFalse; litFalse.operand.tag = ParamType::Bool; litFalse.operand.val._b = false;
	assignGuard.rightNode = &litFalse;
	root.statments.push_back(&assignGuard);

	// local side_effect = 100
	AssignmentNode assignSide;
	assignSide.targetVar = "side_effect";
	LiteralNode lit100; lit100.operand.tag = ParamType::Int; lit100.operand.val._i = 100;
	assignSide.rightNode = &lit100;
	root.statments.push_back(&assignSide);

	// Outer If: if (guard == true)
	IfNode outerIf;
	root.statments.push_back(&outerIf);

	VariableNode varGuard; varGuard.targetVar = "guard";
	LiteralNode litTrue; litTrue.operand.tag = ParamType::Bool; litTrue.operand.val._b = true;
	BinaryNode condOuter;
	condOuter.op = Opcode::Equal;
	condOuter.leftNode = &varGuard;
	condOuter.rightNode = &litTrue;
	outerIf.conditionNode = &condOuter;

	// Outer Then Block
	StatementBlockNode outerThen;
	outerIf.leftNode = &outerThen;

	// Inner If: if (side_effect > 50)
	IfNode innerIf;
	outerThen.statments.push_back(&innerIf);

	VariableNode varSide; varSide.targetVar = "side_effect";
	LiteralNode lit50; lit50.operand.tag = ParamType::Int; lit50.operand.val._i = 50;
	BinaryNode condInner;
	condInner.op = Opcode::Greater;
	condInner.leftNode = &varSide;
	condInner.rightNode = &lit50;
	innerIf.conditionNode = &condInner;

	// Inner Then: _out = 999
	StatementBlockNode innerThen;
	innerIf.leftNode = &innerThen;
	AssignmentNode mutateOut;
	mutateOut.targetVar = "_out";
	LiteralNode lit999; lit999.operand.tag = ParamType::Int; lit999.operand.val._i = 999;
	mutateOut.rightNode = &lit999;
	innerThen.statments.push_back(&mutateOut);

	VMTestHarness harness;
	harness.execute(&root);

	// _out must remain 42 because guard was false
	int result = harness.getParam("_out").asInt();
	assert(result == 42);
	std::cout << "[PASS] test_guarded_side_effect_bypass (Result: " << result << ")\n";
}

// Test 2.2: Relational comparisons evaluated into booleans and combined
// Logic:
//   local cond1 = (a > b);   // 15 > 10 => true
//   local cond2 = (c == d);  // 5 == 5  => true
//   if (cond1 == cond2) { _out = 1; }
//   else                { _out = 0; }
// Verifies: Interoperability of boolean typed local variables on the stack with Opcode::Equal.
void test_compound_boolean_evaluation() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	// local a = 15, b = 10
	AssignmentNode assignA; assignA.targetVar = "a";
	LiteralNode litA; litA.operand.tag = ParamType::Int; litA.operand.val._i = 15;
	assignA.rightNode = &litA;
	root.statments.push_back(&assignA);

	AssignmentNode assignB; assignB.targetVar = "b";
	LiteralNode litB; litB.operand.tag = ParamType::Int; litB.operand.val._i = 10;
	assignB.rightNode = &litB;
	root.statments.push_back(&assignB);

	// local cond1 = (a > b)
	AssignmentNode assignCond1; assignCond1.targetVar = "cond1";
	VariableNode varA; varA.targetVar = "a";
	VariableNode varB; varB.targetVar = "b";
	BinaryNode cmp1;
	cmp1.op = Opcode::Greater;
	cmp1.leftNode = &varA;
	cmp1.rightNode = &varB;
	assignCond1.rightNode = &cmp1;
	root.statments.push_back(&assignCond1);

	// local cond2 = (5 == 5)
	AssignmentNode assignCond2; assignCond2.targetVar = "cond2";
	LiteralNode litC; litC.operand.tag = ParamType::Int; litC.operand.val._i = 5;
	LiteralNode litD; litD.operand.tag = ParamType::Int; litD.operand.val._i = 5;
	BinaryNode cmp2;
	cmp2.op = Opcode::Equal;
	cmp2.leftNode = &litC;
	cmp2.rightNode = &litD;
	assignCond2.rightNode = &cmp2;
	root.statments.push_back(&assignCond2);

	// if (cond1 == cond2)
	IfNode ifNode;
	root.statments.push_back(&ifNode);

	VariableNode varC1; varC1.targetVar = "cond1";
	VariableNode varC2; varC2.targetVar = "cond2";
	BinaryNode boolMatch;
	boolMatch.op = Opcode::Equal;
	boolMatch.leftNode = &varC1;
	boolMatch.rightNode = &varC2;
	ifNode.conditionNode = &boolMatch;

	// True: _out = 1
	StatementBlockNode thenBlock;
	ifNode.leftNode = &thenBlock;
	AssignmentNode assignTrue;
	assignTrue.targetVar = "_out";
	LiteralNode val1; val1.operand.tag = ParamType::Int; val1.operand.val._i = 1;
	assignTrue.rightNode = &val1;
	thenBlock.statments.push_back(&assignTrue);

	// False: _out = 0
	StatementBlockNode elseBlock;
	ifNode.rightNode = &elseBlock;
	AssignmentNode assignFalse;
	assignFalse.targetVar = "_out";
	LiteralNode val0; val0.operand.tag = ParamType::Int; val0.operand.val._i = 0;
	assignFalse.rightNode = &val0;
	elseBlock.statments.push_back(&assignFalse);

	VMTestHarness harness;
	harness.execute(&root);
	assert(harness.getParam("_out").asInt() == 1);
	std::cout << "[PASS] test_compound_boolean_evaluation\n";
}

// =========================================================================
// Category 3: Local Variable Lifetimes & Stack Offset Management
// =========================================================================

// Test 3.1: Multiple local variable slots and interleaved updates
// Logic:
//   local a = 10;
//   local b = 20;
//   local c = 30;
//   a = a + b; // a = 30
//   b = c - b; // b = 30 - 20 = 10
//   c = a * b; // c = 30 * 10 = 300
//   _out = c;
// Verifies: LoadLocal and StoreLocal correctly calculate distinct stack slot offsets.
void test_multiple_local_variable_slots() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	// a = 10
	AssignmentNode assignA; assignA.targetVar = "a";
	LiteralNode lit10; lit10.operand.tag = ParamType::Int; lit10.operand.val._i = 10;
	assignA.rightNode = &lit10;
	root.statments.push_back(&assignA);

	// b = 20
	AssignmentNode assignB; assignB.targetVar = "b";
	LiteralNode lit20; lit20.operand.tag = ParamType::Int; lit20.operand.val._i = 20;
	assignB.rightNode = &lit20;
	root.statments.push_back(&assignB);

	// c = 30
	AssignmentNode assignC; assignC.targetVar = "c";
	LiteralNode lit30; lit30.operand.tag = ParamType::Int; lit30.operand.val._i = 30;
	assignC.rightNode = &lit30;
	root.statments.push_back(&assignC);

	// a = a + b
	AssignmentNode updateA; updateA.targetVar = "a";
	VariableNode varA1; varA1.targetVar = "a";
	VariableNode varB1; varB1.targetVar = "b";
	BinaryNode addAB;
	addAB.op = Opcode::Add;
	addAB.leftNode = &varA1;
	addAB.rightNode = &varB1;
	updateA.rightNode = &addAB;
	root.statments.push_back(&updateA);

	// b = c - b
	AssignmentNode updateB; updateB.targetVar = "b";
	VariableNode varC1; varC1.targetVar = "c";
	VariableNode varB2; varB2.targetVar = "b";
	BinaryNode subCB;
	subCB.op = Opcode::Sub;
	subCB.leftNode = &varC1;
	subCB.rightNode = &varB2;
	updateB.rightNode = &subCB;
	root.statments.push_back(&updateB);

	// c = a * b
	AssignmentNode updateC; updateC.targetVar = "c";
	VariableNode varA2; varA2.targetVar = "a";
	VariableNode varB3; varB3.targetVar = "b";
	BinaryNode mulAB;
	mulAB.op = Opcode::Mul;
	mulAB.leftNode = &varA2;
	mulAB.rightNode = &varB3;
	updateC.rightNode = &mulAB;
	root.statments.push_back(&updateC);

	// _out = c
	AssignmentNode assignOut; assignOut.targetVar = "_out";
	VariableNode varC2; varC2.targetVar = "c";
	assignOut.rightNode = &varC2;
	root.statments.push_back(&assignOut);

	VMTestHarness harness;
	harness.execute(&root);
	int res = harness.getParam("_out").asInt();
	assert(res == 300);
	std::cout << "[PASS] test_multiple_local_variable_slots (Result: " << res << ")\n";
}

// Test 3.2: Interaction between parameters and local variables
// Logic:
//   local factor = 3;
//   local acc = in_x * factor; // acc = 5 * 3 = 15
//   local offset = in_y + 2;   // offset = 4 + 2 = 6
//   acc = acc - offset;        // acc = 15 - 6 = 9
//   _out = acc;
// Verifies: LoadParam/StoreParam work seamlessly alongside LoadLocal/StoreLocal.
void test_param_and_local_interaction() {
	StatementBlockNode root;

	ParamDeclareNode pX;   pX.targetParam = "in_x";
	ParamDeclareNode pY;   pY.targetParam = "in_y";
	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pX);
	root.statments.push_back(&pY);
	root.statments.push_back(&pOut);

	// local factor = 3
	AssignmentNode assignFactor; assignFactor.targetVar = "factor";
	LiteralNode lit3; lit3.operand.tag = ParamType::Int; lit3.operand.val._i = 3;
	assignFactor.rightNode = &lit3;
	root.statments.push_back(&assignFactor);

	// local acc = in_x * factor
	AssignmentNode assignAcc; assignAcc.targetVar = "acc";
	VariableNode varX; varX.targetVar = "in_x";
	VariableNode varFactor; varFactor.targetVar = "factor";
	BinaryNode mulNode;
	mulNode.op = Opcode::Mul;
	mulNode.leftNode = &varX;
	mulNode.rightNode = &varFactor;
	assignAcc.rightNode = &mulNode;
	root.statments.push_back(&assignAcc);

	// local offset = in_y + 2
	AssignmentNode assignOffset; assignOffset.targetVar = "offset";
	VariableNode varY; varY.targetVar = "in_y";
	LiteralNode lit2; lit2.operand.tag = ParamType::Int; lit2.operand.val._i = 2;
	BinaryNode addNode;
	addNode.op = Opcode::Add;
	addNode.leftNode = &varY;
	addNode.rightNode = &lit2;
	assignOffset.rightNode = &addNode;
	root.statments.push_back(&assignOffset);

	// acc = acc - offset
	AssignmentNode updateAcc; updateAcc.targetVar = "acc";
	VariableNode varAcc1; varAcc1.targetVar = "acc";
	VariableNode varOffset; varOffset.targetVar = "offset";
	BinaryNode subNode;
	subNode.op = Opcode::Sub;
	subNode.leftNode = &varAcc1;
	subNode.rightNode = &varOffset;
	updateAcc.rightNode = &subNode;
	root.statments.push_back(&updateAcc);

	// _out = acc
	AssignmentNode assignOut; assignOut.targetVar = "_out";
	VariableNode varAcc2; varAcc2.targetVar = "acc";
	assignOut.rightNode = &varAcc2;
	root.statments.push_back(&assignOut);

	VMTestHarness harness;
	Operand opX{}; opX.tag = ParamType::Int; opX.val._i = 5;
	Operand opY{}; opY.tag = ParamType::Int; opY.val._i = 4;
	harness.setParam("in_x", opX);
	harness.setParam("in_y", opY);

	harness.execute(&root);
	int res = harness.getParam("_out").asInt();
	assert(res == 9);
	std::cout << "[PASS] test_param_and_local_interaction (Result: " << res << ")\n";
}

// =========================================================================
// Category 4: Deep AST & Stack Pressure
// =========================================================================

// Test 4.1: Deeply nested right-associative expression tree
// Target expression: 1 + (1 + (1 + ... + (1 + 1)))  [Depth = 20]
// Expected result: 20
// Verifies: Right-skewed trees push operands sequentially before evaluating,
// testing that the operand stack correctly grows to depth 20 without overflow.
void test_deeply_nested_expression_tree() {
	StatementBlockNode root;

	ParamDeclareNode pOut;
	pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	const int DEPTH = 20;
	std::vector<LiteralNode> literals(DEPTH);
	std::vector<BinaryNode> binaryNodes(DEPTH - 1);

	// Initialize all literal nodes with value 1
	for (int i = 0; i < DEPTH; ++i) {
		literals[i].operand.tag = ParamType::Int;
		literals[i].operand.val._i = 1;
	}

	// Build right-leaning tree: bin[i].left = lit[i], bin[i].right = bin[i+1]
	binaryNodes.back().op = Opcode::Add;
	binaryNodes.back().leftNode = &literals[DEPTH - 2];
	binaryNodes.back().rightNode = &literals[DEPTH - 1];

	for (int i = DEPTH - 3; i >= 0; --i) {
		binaryNodes[i].op = Opcode::Add;
		binaryNodes[i].leftNode = &literals[i];
		binaryNodes[i].rightNode = &binaryNodes[i + 1];
	}

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &binaryNodes[0];
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	harness.execute(&root);

	int result = harness.getParam("_out").asInt();
	assert(result == 20);
	std::cout << "[PASS] test_deeply_nested_expression_tree (Depth: " << DEPTH << ", Result: " << result << ")\n";
}

// =========================================================================
// Category 5: Type Promotion & Complex Bitwise Edge Cases
// =========================================================================

// Test 5.1: Mixed Int-Float alternating chain promotion
// Target expression: _out = ((a + b) * c) - (d / e)
// Inputs: a = 5 (Int), b = 2.5f (Float), c = 2 (Int), d = 8.0f (Float), e = 4 (Int)
// Evaluation:
//   (5 + 2.5f)  => 7.5f  [Promoted to Float]
//   7.5f * 2    => 15.0f [Promoted to Float]
//   8.0f / 4    => 2.0f  [Promoted to Float]
//   15.0f - 2.0f=> 13.0f [Result tag: Float]
// Verifies: Correct promotion to ParamType::Float across cascading mixed binary operators.
void test_type_promotion_chain() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	ParamDeclareNode pA;   pA.targetParam = "a";
	ParamDeclareNode pB;   pB.targetParam = "b";
	ParamDeclareNode pC;   pC.targetParam = "c";
	ParamDeclareNode pD;   pD.targetParam = "d";
	ParamDeclareNode pE;   pE.targetParam = "e";
	root.statments.push_back(&pOut);
	root.statments.push_back(&pA);
	root.statments.push_back(&pB);
	root.statments.push_back(&pC);
	root.statments.push_back(&pD);
	root.statments.push_back(&pE);

	// Node: a + b
	VariableNode varA; varA.targetVar = "a";
	VariableNode varB; varB.targetVar = "b";
	BinaryNode addAB;
	addAB.op = Opcode::Add;
	addAB.leftNode = &varA;
	addAB.rightNode = &varB;

	// Node: (a + b) * c
	VariableNode varC; varC.targetVar = "c";
	BinaryNode mulABC;
	mulABC.op = Opcode::Mul;
	mulABC.leftNode = &addAB;
	mulABC.rightNode = &varC;

	// Node: d / e
	VariableNode varD; varD.targetVar = "d";
	VariableNode varE; varE.targetVar = "e";
	BinaryNode divDE;
	divDE.op = Opcode::Div;
	divDE.leftNode = &varD;
	divDE.rightNode = &varE;

	// Node: ((a + b) * c) - (d / e)
	BinaryNode subTotal;
	subTotal.op = Opcode::Sub;
	subTotal.leftNode = &mulABC;
	subTotal.rightNode = &divDE;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &subTotal;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	Operand opA{}; opA.tag = ParamType::Int;   opA.val._i = 5;
	Operand opB{}; opB.tag = ParamType::Float; opB.val._f = 2.5f;
	Operand opC{}; opC.tag = ParamType::Int;   opC.val._i = 2;
	Operand opD{}; opD.tag = ParamType::Float; opD.val._f = 8.0f;
	Operand opE{}; opE.tag = ParamType::Int;   opE.val._i = 4;
	harness.setParam("a", opA);
	harness.setParam("b", opB);
	harness.setParam("c", opC);
	harness.setParam("d", opD);
	harness.setParam("e", opE);

	harness.execute(&root);

	Operand out = harness.getParam("_out");
	assert(out.tag == ParamType::Float);
	assert(std::abs(out.asFloat() - 13.0f) < 1e-4f);
	std::cout << "[PASS] test_type_promotion_chain (Result: " << out.asFloat() << ")\n";
}

// Test 5.2: Complex bitwise masking and shifting
// Target expression: _out = ((val << 4) ^ mask) >> 2
// Inputs: val = 15 (0x0F), mask = 170 (0xAA)
// Evaluation:
//   15 << 4   = 240 (0xF0)
//   240 ^ 170 = 90  (0x5A)
//   90 >> 2   = 22  (0x16)
// Verifies: Proper operand pop order (opnd2 OP opnd1) for Shl, Shr, and BitXor.
void test_bitwise_masking_and_shifting() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	// val << 4
	LiteralNode litVal;  litVal.operand.tag = ParamType::Int; litVal.operand.val._i = 15;
	LiteralNode litShl4; litShl4.operand.tag = ParamType::Int; litShl4.operand.val._i = 4;
	BinaryNode shlNode;
	shlNode.op = Opcode::Shl;
	shlNode.leftNode = &litVal;
	shlNode.rightNode = &litShl4;

	// (val << 4) ^ mask
	LiteralNode litMask; litMask.operand.tag = ParamType::Int; litMask.operand.val._i = 170;
	BinaryNode xorNode;
	xorNode.op = Opcode::BitXor;
	xorNode.leftNode = &shlNode;
	xorNode.rightNode = &litMask;

	// ((val << 4) ^ mask) >> 2
	LiteralNode litShr2; litShr2.operand.tag = ParamType::Int; litShr2.operand.val._i = 2;
	BinaryNode shrNode;
	shrNode.op = Opcode::Shr;
	shrNode.leftNode = &xorNode;
	shrNode.rightNode = &litShr2;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &shrNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	harness.execute(&root);

	int result = harness.getParam("_out").asInt();
	assert(result == 22);
	std::cout << "[PASS] test_bitwise_masking_and_shifting (Result: " << result << ")\n";
}

// =========================================================================
// Category 6: Robustness & Boundary Handling
// =========================================================================

// Test 6.1: Division and modulo by zero protection
// Target expression: _out = (a / 0) + (b % 0)
// Inputs: a = 100, b = 25
// Expected behavior: VM implementation guards against hardware div-by-zero
// by returning 0, ensuring VM does not crash and execution completes smoothly.
void test_divide_and_mod_by_zero() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	LiteralNode lit100; lit100.operand.tag = ParamType::Int; lit100.operand.val._i = 100;
	LiteralNode lit25;  lit25.operand.tag = ParamType::Int;  lit25.operand.val._i = 25;
	LiteralNode litZero; litZero.operand.tag = ParamType::Int; litZero.operand.val._i = 0;

	// a / 0
	BinaryNode divNode;
	divNode.op = Opcode::Div;
	divNode.leftNode = &lit100;
	divNode.rightNode = &litZero;

	// b % 0
	BinaryNode modNode;
	modNode.op = Opcode::Mod;
	modNode.leftNode = &lit25;
	modNode.rightNode = &litZero;

	// (a / 0) + (b % 0)
	BinaryNode addNode;
	addNode.op = Opcode::Add;
	addNode.leftNode = &divNode;
	addNode.rightNode = &modNode;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &addNode;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	harness.execute(&root);

	int result = harness.getParam("_out").asInt();
	assert(result == 0);
	std::cout << "[PASS] test_divide_and_mod_by_zero (Safe result: " << result << ")\n";
}

// Test 6.2: Chained relational and inequality comparisons
// Target expression: _out = (a != b) == (c == d)
// Evaluation:
//   (10 != 20) => true
//   (5 == 5)   => true
//   true == true => true (stored as 1 / true)
// Verifies: Equality comparison when both operands are boolean values from relational operators.
void test_chained_comparisons() {
	StatementBlockNode root;

	ParamDeclareNode pOut; pOut.targetParam = "_out";
	root.statments.push_back(&pOut);

	// a != b (10 != 20 => true)
	LiteralNode litA; litA.operand.tag = ParamType::Int; litA.operand.val._i = 10;
	LiteralNode litB; litB.operand.tag = ParamType::Int; litB.operand.val._i = 20;
	BinaryNode neqNode;
	neqNode.op = Opcode::NotEqual;
	neqNode.leftNode = &litA;
	neqNode.rightNode = &litB;

	// c == d (5 == 5 => true)
	LiteralNode litC; litC.operand.tag = ParamType::Int; litC.operand.val._i = 5;
	LiteralNode litD; litD.operand.tag = ParamType::Int; litD.operand.val._i = 5;
	BinaryNode eqNode;
	eqNode.op = Opcode::Equal;
	eqNode.leftNode = &litC;
	eqNode.rightNode = &litD;

	// (a != b) == (c == d)
	BinaryNode cmpMatch;
	cmpMatch.op = Opcode::Equal;
	cmpMatch.leftNode = &neqNode;
	cmpMatch.rightNode = &eqNode;

	AssignmentNode assignNode;
	assignNode.targetVar = "_out";
	assignNode.rightNode = &cmpMatch;
	root.statments.push_back(&assignNode);

	VMTestHarness harness;
	harness.execute(&root);

	bool result = harness.getParam("_out").asBool();
	assert(result == true);
	std::cout << "[PASS] test_chained_comparisons (Result: " << (result ? "true" : "false") << ")\n";
}

// ==========================================
// Run all tests
// ==========================================
int main() {
	try {
		test_arithmetic_compound();
		test_float_and_comparison();
		test_bitwise_operations();
		test_nested_if_else();
		test_modulo_operation();
		test_cascaded_if_else_chain();
		test_diamond_flow_convergence();
		test_guarded_side_effect_bypass();
		test_compound_boolean_evaluation();
		test_multiple_local_variable_slots();
		test_param_and_local_interaction();
		test_deeply_nested_expression_tree();
		test_type_promotion_chain();
		test_bitwise_masking_and_shifting();
		test_divide_and_mod_by_zero();
		test_chained_comparisons();
		std::cout << "\nAll VM AST test cases passed successfully!\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Test failed with exception: " << e.what() << "\n";
		return 1;
	}
	return 0;
}