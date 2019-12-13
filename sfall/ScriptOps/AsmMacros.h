#pragma once

/*
	MACROS for operators assembly code

	Notes:
	- DO NOT add any comments within macros
	- every macro should contain __asm {} block
	- every assembly line should start with __asm and should NOT have semicolon in the end!
	- use this macros outside of other __asm {} blocks (obviously)
*/

// rscript - 32-bit register where script pointer will be put (it is used for several related functions)
#define _OP_BEGIN(rscript) __asm \
{							\
	__asm pushad			\
	__asm mov rscript, eax	\
}

#define _OP_END  __asm \
{							\
	__asm popad				\
	__asm retn				\
}

/*
	Gets next argument from stack and puts its type to "rtype" and value to "rval"
	arguments come in reverse order (from last to first)
	all arguments should be valid r32 registers
*/
#define _GET_ARG_R32(rscript, rtype, rval) __asm \
{	\
	__asm mov eax, rscript					\
	__asm call interpretPopShort_			\
	__asm mov rtype, eax					\
	__asm mov eax, rscript					\
	__asm call interpretPopLong_			\
	__asm mov rval, eax \
}

/*
	Gets argument from stack to eax and puts its type to edx register
	eax register must contain the script_ptr
	jlabel - name of the jump label in case the value type is not INT
	return: eax - arg value
*/
#define _GET_ARG_INT(jlabel) __asm {		\
	__asm mov  edx, eax						\
	__asm call interpretPopShort_			\
	__asm xchg eax, edx						\
	__asm call interpretPopLong_			\
	__asm cmp  dx, VAR_TYPE_INT				\
	__asm jnz  jlabel						\
}

/*
	checks argument to be integer, and jumps to GOTOFAIL if it's not
*/
#define _CHECK_ARG_INT(r16type, GOTOFAIL) __asm { \
	__asm cmp r16type, VAR_TYPE_INT		\
	__asm jne GOTOFAIL }

/*
	checks argument to be float, and jumps to GOTOFAIL if it's not
*/
#define _CHECK_ARG_FLOAT(r16type, GOTOFAIL) __asm { \
	__asm cmp r16type, VAR_TYPE_FLOAT		\
	__asm jne GOTOFAIL }


/*
	checks argument (which may be any type) if it's a string and retrieves it (overwrites value in rval)
	num - any number, but it must be unique (used for label names)
	r16type - 16bit register where value type is stored
	rval - r32 where value is stored
*/
#define _CHECK_PARSE_STR(num, rscript, r16type, rval) __asm { \
	__asm cmp r16type, VAR_TYPE_STR2	\
	__asm jz skipgetstr##num			\
	__asm cmp r16type, VAR_TYPE_STR		\
	__asm jnz notstring##num			\
__asm skipgetstr##num:					\
	__asm mov edx, e##r16type			\
	__asm mov ebx, rval					\
	__asm mov eax, rscript				\
	__asm call interpretGetString_		\
	__asm mov rval, eax	} \
notstring##num:

#define _PARSE_STR_ARG(num, rscript, r16type, rval, GOTOFAIL) __asm { \
	__asm cmp r16type, VAR_TYPE_STR2	\
	__asm jz skipgetstr##num			\
	__asm cmp r16type, VAR_TYPE_STR		\
	__asm jnz GOTOFAIL					\
	__asm skipgetstr##num:				\
	__asm mov edx, e##r16type			\
	__asm mov ebx, rval					\
	__asm mov eax, rscript				\
	__asm call interpretGetString_		\
	__asm mov rval, eax	}


// must be immediately after C function call
#define _RET_VAL_INT(rscript) __asm {		\
	__asm mov edx, eax					\
	__asm mov eax, rscript				\
	__asm call interpretPushLong_		\
	__asm mov edx, VAR_TYPE_INT			\
	__asm mov eax, rscript				\
	__asm call interpretPushShort_		\
}

// return value stored in EAX as float
#define _RET_VAL_FLOAT(rscript) __asm {		\
	__asm mov edx, eax					\
	__asm mov eax, rscript				\
	__asm call interpretPushLong_		\
	__asm mov edx, VAR_TYPE_FLOAT		\
	__asm mov eax, rscript				\
	__asm call interpretPushShort_		\
}

/*
	Returns the value to the script
	eax and ebx register must contain the script_ptr
	edx register must contain the returned value
*/
#define _RET_VAL_INT2 __asm {			\
	__asm call interpretPushLong_		\
	__asm mov  edx, VAR_TYPE_INT		\
	__asm mov  eax, ebx					\
	__asm call interpretPushShort_		\
}

#define _J_RET_VAL_TYPE(type) __asm {	\
	__asm call interpretPushLong_		\
	__asm mov  edx, type				\
	__asm mov  eax, ebx					\
	__asm jmp  interpretPushShort_		\
}

/*
	handle return value which may be of any type
	num - any unique number
	type - register or other expression (like memory address) where value type is stored (usually [esp])
*/
#define _RET_VAL_POSSIBLY_STR(num, rscript, type) __asm {  \
	__asm mov ecx, eax					\
	__asm mov edx, type					\
	__asm cmp edx, VAR_TYPE_STR			\
	__asm jne resultnotstr##num			\
	__asm mov edx, eax					\
	__asm mov eax, rscript				\
	__asm call interpretAddString_		\
	__asm mov ecx, eax					\
__asm resultnotstr##num:				\
	__asm mov edx, ecx					\
	__asm mov eax, rscript				\
	__asm call interpretPushLong_		\
	__asm mov edx, type					\
	__asm mov eax, rscript				\
	__asm call interpretPushShort_		\
}

/*
	better way of handling new opcodes:
	- no ASM code required
	- all type checks should be done in the wrapped C-function
	- use opArgs, opArgTypes to access arguments
	- use opRet, opRetType to set return value

	func - C-function to call (should not have arguments)
	argnum - number of opcode arguments
	isExpression - 1 if opcode has return value, 0 otherwise
*/
#define _WRAP_OPCODE(func, argnum, isExpression) __asm { \
	__asm pushad					\
	__asm push isExpression			\
	__asm push argnum				\
	__asm push func					\
	__asm push eax					\
	__asm lea ecx, opHandler		\
	__asm call OpcodeHandler::handleOpcode	\
	__asm popad						\
	__asm retn						\
}

