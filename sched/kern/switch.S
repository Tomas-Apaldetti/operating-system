/*
 * Your code here for context_switch
 *
 * It should restores the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
context_switch:
	addl $4, %esp				# Skip return direction, it doesn't matter
	movl (%esp), %esp			# Move Stack pointer to the direction of where the Struct Environment is
	popal						# Populate General Purposes Register
	popl %es					# Populate es register
	popl %ds					# Populate ds register
	addl $8, %esp 				# Skip errno and error code
	iret 
spin:
	jmp spin
