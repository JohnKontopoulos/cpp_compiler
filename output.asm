	.data
	.text
	.globl main

add:	# function add
	# prologue
	addiu $sp, $sp, -24	# allocate frame
	sw $ra, 20($sp)	# save $ra
	sw $fp, 16($sp)	# save $fp
	move $fp, $sp	# frame pointer
	# copy params to stack
	sw $a0, 0($sp)	# param 0
	sw $a1, 4($sp)	# param 1
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# result
	lw $t0, 8($sp)	# result
	move $v0, $t0	# return val
	j add_exit	# return
add_exit:	# epilogue
	lw $ra, 20($sp)	# restore $ra
	lw $fp, 16($sp)	# restore $fp
	addiu $sp, $sp, 24	# deallocate frame
	jr $ra	# return

main:	# main function
	addiu $sp, $sp, -16	# allocate frame
	sw $ra, 12($sp)	# save $ra
	li $t0, 5
	sw $t0, 0($sp)	# x
	li $t0, 3
	sw $t0, 4($sp)	# y
	li $t0, 0
	move $v0, $t0	# return val
	j main_exit	# return
main_exit:
	lw $ra, 12($sp)	# restore $ra
	addiu $sp, $sp, 16	# deallocate frame
	li $v0, 10	# exit
	syscall
