	.data
	.text
	.globl main

main:	# main function
	addiu $sp, $sp, -24	# allocate frame
	sw $ra, 20($sp)	# save $ra
	li $t0, 5
	sw $t0, 0($sp)	# x
	li $t0, 5
	sw $t0, 4($sp)	# y
	li $t0, 5
	li $t1, 1
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# z
	li $t0, 0
	move $v0, $t0	# return val
	j main_exit	# return
main_exit:
	lw $ra, 20($sp)	# restore $ra
	addiu $sp, $sp, 24	# deallocate frame
	li $v0, 10	# exit
	syscall
