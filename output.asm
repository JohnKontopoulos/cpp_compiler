	.data
	.text
	.globl main
main:
	addiu $sp, $sp, -24	# stack frame
	sw $ra, 20($sp)	# save $ra
	li $t0, 42
	sw $t0, 0($sp)	# x
	li $t0, 3
	mtc1 $t0, $f0
	cvt.s.w $f0, $f0
	s.s $f0, 4($sp)	# y
	li $t0, 97	# 'a'
	sw $t0, 8($sp)	# c
	lw $t0, 0($sp)	# x
	move $t1, $t0
	addi $t0, $t0, 1
	sw $t0, 0($sp)	# x
	lw $t0, 0($sp)	# x
	li $t1, 0
	slt $t2, $t0, $t1
	beq $t2, $zero, L0
	lw $t0, 0($sp)	# x
	move $a0, $t0
	li $v0, 1
	syscall
L0:
	li $t0, 0
	move $v0, $t0
	j main_exit
main_exit:
	lw $ra, 20($sp)	# restore $ra
	addiu $sp, $sp, 24
	li $v0, 10
	syscall
