	.data
	.text
	.globl main

main:	# main function
	addiu $sp, $sp, -16	# allocate frame
	sw $ra, 12($sp)	# save $ra
	li $t0, 5
	sw $t0, 0($sp)	# x
	lw $t0, 0($sp)	# x
	li $t1, 1
	mul $t2, $t0, $t1
	sw $t2, 4($sp)	# y
	li $t0, 0
	beq $t0, $zero, L0	# if false
	li $t0, 99
	sw $t0, 4($sp)	# y
L0:
	li $t0, 0
	move $v0, $t0	# return val
	j main_exit	# return
main_exit:
	lw $ra, 12($sp)	# restore $ra
	addiu $sp, $sp, 16	# deallocate frame
	li $v0, 10	# exit
	syscall
