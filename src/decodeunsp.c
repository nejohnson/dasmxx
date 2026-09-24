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
DASM_PROFILE( "dasmunsp", "SunPlus µnSP", 4, 8, 0, 2, 2 )

const char* const regname[] = { "SP", "R1", "R2", "R3", "R4", "BP", "SR", "PC" };

#define OPB (opc & 7)
#define OPN ((opc >> 3) & 7)
#define OP1 ((opc >> 6) & 7)
#define OPA ((opc >> 9) & 7)
#define IMM6 (opc & 0x3F)

int dasm_cfg_supported( void )
{
	return 1;
}

static int read_opcode_word( ADDR addr, UWORD *out )
{
	return dasm_input_read_word_at( addr, out );
}

static int cfg_jmp_target( ADDR opaddr, UWORD op, ADDR *target )
{
	int dir = (op >> 6) & 7;
	int off = op & 0x3F;
	ADDR next_word_addr = opaddr / dasm_word_width_bytes + 1;

	if (dir == 1)
		*target = (next_word_addr - off) * dasm_word_width_bytes;
	else if (dir == 0)
		*target = (next_word_addr + off) * dasm_word_width_bytes;
	else
		return 0;

	return 1;
}

static int cfg_long_target( ADDR opaddr, ADDR *target )
{
	UWORD word;
	ADDR next_word_addr = opaddr / dasm_word_width_bytes + 1;

	if (!read_opcode_word(opaddr + 2, &word))
		return 0;

	*target = (word | (next_word_addr & 0xFFFF0000)) * dasm_word_width_bytes;
	return 1;
}

static int cfg_call_target( ADDR opaddr, UWORD op, ADDR *target )
{
	UWORD word;

	if (!read_opcode_word(opaddr + 2, &word))
		return 0;

	*target = (((op & 0x3F) << 16) | word) * dasm_word_width_bytes;
	return 1;
}

void dasm_post_insn( void )
{
	UWORD op0;
	ADDR target;

	if (!read_opcode_word(g_insn_addr, &op0))
		return;

	if ((op0 & 0xFF80) == 0xEE00)
	{
		dasm_cfg_set_flow(CFG_FLOW_JUMP);
		if (cfg_jmp_target(g_insn_addr, op0, &target))
			dasm_cfg_add_target(target);
	}
	else if ((op0 & 0xFF80) == 0x0E00
	         || (op0 & 0xFF80) == 0x4E00
	         || (op0 & 0xFF80) == 0x5E00
	         || (op0 & 0xFF80) == 0x7E00
	         || (op0 & 0xFF80) == 0x9E00
	         || (op0 & 0xFF80) == 0xBE00)
	{
		dasm_cfg_set_flow(CFG_FLOW_COND_JUMP);
		if (cfg_jmp_target(g_insn_addr, op0, &target))
			dasm_cfg_add_target(target);
	}
	else if (op0 == 0x9A90 || op0 == 0x9A98)
	{
		dasm_cfg_set_flow(CFG_FLOW_RETURN);
	}
	else if (op0 == 0x9F0F)
	{
		dasm_cfg_set_flow(CFG_FLOW_JUMP);
		if (cfg_long_target(g_insn_addr, &target))
			dasm_cfg_add_target(target);
	}
	else if ((op0 & 0xF1C0) == 0xF040)
	{
		dasm_cfg_set_flow(CFG_FLOW_CALL);
		if (cfg_call_target(g_insn_addr, op0, &target))
			dasm_cfg_add_target(target);
	}
	else if ((op0 & 0xFFC0) == 0xFE80)
	{
		dasm_cfg_set_flow(CFG_FLOW_JUMP);
		if (cfg_call_target(g_insn_addr, op0, &target))
			dasm_cfg_add_target(target);
	}
}

OPERAND_FUNC(none)
{
}

OPERAND_FUNC(int)
{
	const char* const intname[] = { "OFF", "IRQ", "FIQ", "IRQ,FIQ" };
	operand( "%s", intname[OPB]);
}

OPERAND_FUNC(fir)
{
	const char* const intname[] = { "ON", "OFF" };
	operand( "%s", intname[OPB & 1]);
}

OPERAND_FUNC(call)
{
	int target = (IMM6 << 16) | nextw(f, addr);
	char buf[32];
	operand( "%s", xref_genwordaddr(buf, "%08x", target));
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
					operand("[++%s]", regname[rs]);
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
					operand("%s ASR %d", regname[OPB], opn - 3);
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
		case 6:
		{
			int opn = OPN;
			if (opn >= 4)
				operand("%s ROR %d", regname[OPB], opn - 3);
			else
				operand("%s ROL %d", regname[OPB], opn + 1);
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

OPERAND_FUNC(mac)
{
	int opn = OPN;
	int op1 = OP1;
	opn += (op1 & 1) << 3;
	operand("%s, %s, %d", regname[OPA], regname[OPB], opn);
}

TWO_OPERAND(op1, op2)
TWO_OPERAND(op1, op3)
TWO_OPERAND(pushset, stack)
TWO_OPERAND(popset, stack)

OPERAND_FUNC(cmp)
{
	if (OP1 == 4 && OPN == 1)
		operand_op3(f, addr, opc, xtype);
	else
		operand_op1_op2(f, addr, opc, xtype);
}

optab_t base_optab[] = {

	// Jumps
	MASK( "JCC", jmp,     0xFF80, 0x0E00, X_JMP)
	MASK( "JNZ", jmp,     0xFF80, 0x4E00, X_JMP)
	MASK( "JZ",  jmp,     0xFF80, 0x5E00, X_JMP)
	MASK( "JMI", jmp,     0xFF80, 0x7E00, X_JMP)
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
	MASK( "CMP",  cmp,     0xF000, 0x4000, X_NONE)
	MASK( "NEG",  op1_op2, 0xF000, 0x6000, X_NONE)
	MASK( "XOR",  op1_op3, 0xF000, 0x8000, X_NONE)
	MASK( "LD",   op1_op2, 0xF000, 0x9000, X_NONE)
	MASK( "OR",   op1_op3, 0xF000, 0xA000, X_NONE)
	MASK( "AND",  op1_op3, 0xF000, 0xB000, X_NONE)
	MASK( "TEST", op3,     0xF000, 0xC000, X_NONE)
	MASK( "ST",   op1_op2, 0xF000, 0xD000, X_NONE)

	// Specials
	MASK( "INT",  int,  0xF1FC, 0xF140, X_NONE)
	MASK( "FIR_MOV",  fir,  0xF1FE, 0xF144, X_NONE)
	MASK( "CALL", call, 0xF1C0, 0xF040, X_CALL)
	MASK( "GOTO", call, 0xFFC0, 0xFE80, X_CALL)
	MASK( "MULU", mul,  0xF1F8, 0xF008, X_NONE)
	MASK( "MULS", mul,  0xF1F8, 0xF108, X_NONE)
	MASK( "MACU", mac,  0xF180, 0xF080, X_NONE)
	MASK( "MACS", mac,  0xF180, 0xF180, X_NONE)
	END
};
