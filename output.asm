	.data
FMT0:	.asciiz "%d"
FMT1:	.asciiz "%d\n"
	.text
	.globl main

main:	# main function
	addiu $sp, $sp, -16	# allocate frame
	sw $ra, 12($sp)	# save $ra
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _scanf
	la $a0, FMT0	# scanf format
	addiu $a1, $sp, 0	# addr of x
	jal _scanf	# call _scanf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _printf
	la $a0, FMT1	# printf format
	lw $t0, 0($sp)	# x
	move $a1, $t0	# printf arg 1
	jal _printf	# call _printf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	li $t0, 0
	move $v0, $t0	# return val
	j main_exit	# return
main_exit:
	lw $ra, 12($sp)	# restore $ra
	addiu $sp, $sp, 16	# deallocate frame
	li $v0, 10	# exit
	syscall
