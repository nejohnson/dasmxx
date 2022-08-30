/*
 * Copyright (C) 2022 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "dasmxx.h"
#include "optab.h"

/* Note: u'nSP is not able to address individual bytes at all.
 * So dasm_word_width_bytes is set to 2, and only 16-bit words can be addressed.
 */
DASM_PROFILE( "dasmunsp", "SunPlus µnSP", 4, 5, 0, 2, 2 )

const char* const regname[] = { "SP", "R1", "R2", "R3", "R4", "BP", "SR", "PC" };

#define OPB (opc & 7)
#define OPN ((opc >> 3) & 7)
#define OP1 ((opc >> 6) & 7)
#define OPA ((opc >> 9) & 7)
#define IMM6 (opc & 0x3F)

OPERAND_FUNC(none)
{
}

OPERAND_FUNC(int)
{
	const char* const intname[] = { "OFF", "IRQ", "FIQ", "IRQ,FIQ" };
	operand( "%s", intname[OPB]);
}

OPERAND_FUNC(call)
{
	int target = (IMM6 << 16) | nextw(f, addr);
	char buf[32];
	operand( "%s", xref_genwordaddr(buf, "%08x", target));
		;
}

OPERAND_FUNC(jmp)
{
	int dir = OP1;
	int off = IMM6;
	char buf[32];
	if (dir == 1) {
		operand("%s", xref_genwordaddr(buf, "%04x", *addr / 2 - off));
	} else if (dir == 0) {
		operand("%s", xref_genwordaddr(buf, "%04x", *addr / 2 + off));
	} else {
		operand("?? unknown jump direction %d", dir);
	}
}

OPERAND_FUNC(ljmp)
{
	char buf[32];
	int word = nextw(f, addr);
	operand("%s", xref_genwordaddr(buf, "%08x", word | (*addr / 2 & 0xFFFF0000)));
}

OPERAND_FUNC(pushset)
{
	// OPA OPN Regs
	// 1     1 R1
	// 2     2 R1-R2
	// 2     1 R2
	// Rh to Rh-N, OPA encodes Rh
	if ((OPA + 1) >= OPN)
		operand("%s-%s", regname[OPA + 1 - OPN] , regname[OPA]);
	else
		operand("INVALID");
}

OPERAND_FUNC(popset)
{
	// Rl to Rl+N, OPA encodes Rl-1
	if ((OPA + 1) > 7 || (OPA + OPN) > 7)
		operand("INVALID");
	else
		operand("%s-%s", regname[OPA + 1] , regname[OPA + OPN]);
}

OPERAND_FUNC(stack)
{
    operand( "[%s]", regname[OPB]);
}

OPERAND_FUNC(op1)
{
    operand( "%s", regname[OPA]);
}

bool op3 = false;

OPERAND_FUNC(op2)
{
	switch (OP1) {
		case 0:
			operand("[BP+%x]", IMM6);
			break;
		case 1:
			operand("#%x", IMM6);
			break;
		case 3:
		{
			int opn = OPN;
			int rs = OPB;
			int word;
			if (opn & 4)
				operand("D:");
			switch (opn & 3) {
				case 0:
					operand("[%s]", regname[rs]);
					break;
				case 1:
					operand("[%s--]", regname[rs]);
					break;
				case 2:
					operand("[%s++]", regname[rs]);
					break;
				default:
					operand("?? unknown OP3 opn %d", opn);
					break;
			}
			break;
		}
		case 4:
		{
			int opn = OPN;
			int word;
			switch (opn) {
				case 0:
					operand("%s", regname[OPB]);
					break;
				case 1:
					if (op3) {
						operand("%s, ", regname[OPB]);
					}
					word = nextw(f, addr);
					operand("#%x", word);
					break;
				case 2:
				case 3: // only for ST
				{
					word = nextw(f, addr);
					char buf[32];
					operand("[%s]", xref_genwordaddr(buf, "%04x", word));
					break;
				}
				default:
					operand("?? unknown OP4 opn %d", opn);
					break;
			}
			break;
		}
		case 5:
		{
			int opn = OPN;
			if (opn >= 4)
				operand("%s LSR %d", regname[OPB], opn - 3);
			else
				operand("%s LSL %d", regname[OPB], opn + 1);
			break;
		}
		default:
			operand("?? unknown op1 %d", OP1);
			break;
	}
}

OPERAND_FUNC(op3)
{
	op3 = true;
	operand_op2(f, addr, opc, xtype);
	op3 = false;
}

OPERAND_FUNC(mul)
{
	operand("%s, %s", regname[OPA], regname[OPB]);
}

TWO_OPERAND(op1, op2)
TWO_OPERAND(op1, op3)
TWO_OPERAND(pushset, stack)
TWO_OPERAND(popset, stack)

optab_t base_optab[] = {

	// Jumps
	MASK( "JCC", jmp,     0xFF80, 0x0E00, X_JMP)
	MASK( "JNZ", jmp,     0xFF80, 0x4E00, X_JMP)
	MASK( "JZ",  jmp,     0xFF80, 0x5E00, X_JMP)
	MASK( "JA",  jmp,     0xFF80, 0x9E00, X_JMP)
	MASK( "JG",  jmp,     0xFF80, 0xBE00, X_JMP)
	MASK( "JMP", jmp,     0xFF80, 0xEE00, X_JMP)

	INSN( "RETF", none, 0x9A90, X_NONE)
	INSN( "RETI", none, 0x9A98, X_NONE)
	MASK( "POP",  popset_stack, 0xF1C0, 0x9080, X_NONE)
	MASK( "PUSH", pushset_stack, 0xF1C0, 0xD080, X_NONE)
	INSN( "LJMP", ljmp, 0x9F0F, X_JMP)

	// ALU ops
	MASK( "ADD",  op1_op3, 0xF000, 0x0000, X_NONE)
	MASK( "ADC",  op1_op3, 0xF000, 0x1000, X_NONE)
	MASK( "SUB",  op1_op3, 0xF000, 0x2000, X_NONE)
	MASK( "SBC",  op1_op3, 0xF000, 0x3000, X_NONE)
	MASK( "CMP",  op1_op3, 0xF000, 0x4000, X_NONE) // TODO should be only op3 for 3-operand variants?
	MASK( "NEG",  op1_op2, 0xF000, 0x6000, X_NONE)
	MASK( "XOR",  op1_op3, 0xF000, 0x8000, X_NONE)
	MASK( "LD",   op1_op2, 0xF000, 0x9000, X_NONE)
	MASK( "OR",   op1_op3, 0xF000, 0xA000, X_NONE)
	MASK( "AND",  op1_op3, 0xF000, 0xB000, X_NONE)
	MASK( "TEST", op3,     0xF000, 0xC000, X_NONE)
	MASK( "ST",   op1_op2, 0xF000, 0xD000, X_NONE)

	// Specials
	MASK( "INT",  int,  0xF1FC, 0xF140, X_NONE)
	MASK( "CALL", call, 0xF1C0, 0xF040, X_CALL)
	MASK( "MULS", mul,  0xF1C8, 0xF108, X_NONE)
	END
};
