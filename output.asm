	.data
	.text
	.globl main
main:
	addiu $sp, $sp, -24	# allocate frame
	sw $ra, 20($sp)	# save $ra
	li $t0, 42
	sw $t0, 0($sp)	# x
	li $t0, 3
	mtc1 $t0, $f0	# int→float
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
	beq $t2, $zero, L0	# if false
	addiu $sp, $sp, -4
	sw $ra, 0($sp)
	lw $t0, 0($sp)	# x
	# cout int
	move $a0, $t0
	li $v0, 1	# print int syscall
	syscall
	li $a0, 10	# newline
	li $v0, 11
	syscall
	lw $ra, 0($sp)
	addiu $sp, $sp, 4
L0:
	li $t0, 0
	move $v0, $t0	# return value
	j main_exit	# return
main_exit:
	lw $ra, 20($sp)	# restore $ra
	addiu $sp, $sp, 24	# deallocate frame
	li $v0, 10	# exit
	syscall
