	.data
FMT0:	.asciiz "%d%d"
FMT1:	.asciiz "%s\n"
FMT2:	.asciiz "%d\n"
FMT29:	.asciiz "%d\n"
FMT30:	.asciiz "%d\n"
	.text
	.globl main

add:	# function add
	# prologue
	addiu $sp, $sp, -40	# allocate frame
	sw $ra, 36($sp)	# save $ra
	sw $fp, 32($sp)	# save $fp
	move $fp, $sp	# frame pointer
	# copy params to stack
	sw $a0, 0($sp)	# param 0
	sw $a1, 4($sp)	# param 1
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# r
	lw $t0, 8($sp)	# r
	move $v0, $t0	# return val
	j add_exit	# return
add_exit:	# epilogue
	lw $ra, 36($sp)	# restore $ra
	lw $fp, 32($sp)	# restore $fp
	addiu $sp, $sp, 40	# deallocate frame
	jr $ra	# return

main:	# main function
	addiu $sp, $sp, -40	# allocate frame
	sw $ra, 36($sp)	# save $ra
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _scanf
	la $a0, FMT0	# scanf format
	addiu $a1, $sp, 0	# addr of a
	addiu $a2, $sp, 4	# addr of b
	jal _scanf	# call _scanf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _printf
	la $a0, FMT1	# printf format
	lw $t0, 0($gp)	# MSG
	move $a1, $t0	# printf arg 1
	jal _printf	# call _printf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	move $a0, $t2	# param 0
	jal add	# call
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	move $t0, $v0	# return val
	sw $t0, 8($sp)	# sum
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _printf
	la $a0, FMT2	# printf format
	lw $t0, 8($sp)	# sum
	move $a1, $t0	# printf arg 1
	jal _printf	# call _printf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	lw $t0, 0($sp)	# a
	mtc1 $t0, $f0	# int→float
	cvt.s.w $f0, $f0
	s.s $f0, 16($sp)	# f
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	mul $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	mul $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	mul $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	seq $t2, $t0, $t1
	beq $t2, $zero, L3	# if false
	li $t0, 1
	sw $t0, 8($sp)	# sum
L3:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	seq $t2, $t0, $t1
	beq $t2, $zero, L5	# if false
	li $t0, 2
	sw $t0, 8($sp)	# sum
L5:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	slt $t2, $t0, $t1
	beq $t2, $zero, L7	# if false
	li $t0, 3
	sw $t0, 8($sp)	# sum
L7:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	slt $t2, $t0, $t1
	beq $t2, $zero, L9	# if false
	li $t0, 4
	sw $t0, 8($sp)	# sum
L9:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	slt $t2, $t0, $t1
	beq $t2, $zero, L11	# if false
	li $t0, 5
	sw $t0, 8($sp)	# sum
L11:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	slt $t2, $t0, $t1
	beq $t2, $zero, L13	# if false
	li $t0, 6
	sw $t0, 8($sp)	# sum
L13:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	and $t2, $t0, $t1
	beq $t2, $zero, L15	# if false
	li $t0, 7
	sw $t0, 8($sp)	# sum
L15:
	lw $t0, 0($sp)	# a
	lw $t1, 4($sp)	# b
	or $t2, $t0, $t1
	beq $t2, $zero, L17	# if false
	li $t0, 8
	sw $t0, 8($sp)	# sum
L17:
	lw $t0, 0($sp)	# a
	seq $t1, $t0, $zero
	beq $t1, $zero, L19	# if false
	li $t0, 9
	sw $t0, 8($sp)	# sum
L19:
	lw $t0, 0($sp)	# a
	move $t1, $t0
	addi $t0, $t0, 1
	sw $t0, 0($sp)	# a
	lw $t0, 0($sp)	# a
	move $t1, $t0
	addi $t0, $t0, 1
	sw $t0, 0($sp)	# a
	lw $t0, 4($sp)	# b
	addi $t0, $t0, 1
	move $t1, $t0
	sw $t0, 4($sp)	# b
	lw $t0, 4($sp)	# b
	addi $t0, $t0, 1
	move $t1, $t0
	sw $t0, 4($sp)	# b
	li $t0, 0
	sw $t0, 12($sp)	# i
	li $t0, 0
	sw $t0, 8($sp)	# sum
L21:	# while
	li $t0, 0
	li $t1, 10
	slt $t2, $t0, $t1
	beq $t2, $zero, L22
	lw $t0, 8($sp)	# sum
	lw $t1, 12($sp)	# i
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 12($sp)	# i
	move $t1, $t0
	addi $t0, $t0, 1
	sw $t0, 12($sp)	# i
	j L21
L22:
	li $t0, 0
	sw $t0, 12($sp)	# i
L23:	# for
	lw $t0, 12($sp)	# i
	li $t1, 5
	slt $t2, $t0, $t1
	beq $t2, $zero, L24
	lw $t0, 8($sp)	# sum
	li $t1, 100
	slt $t2, $t0, $t1
	beq $t2, $zero, L25	# if false
	li $t0, 100
	sw $t0, 8($sp)	# sum
	j L26
L25:
	li $t0, 100
	li $t1, 1
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
L26:
	lw $t0, 12($sp)	# i
	move $t1, $t0
	addi $t0, $t0, 1
	sw $t0, 12($sp)	# i
	j L23
L24:
	lw $t0, 8($sp)	# sum
	li $t1, 100
	slt $t2, $t0, $t1
	beq $t2, $zero, L27	# if false
	li $t0, 100
	sw $t0, 8($sp)	# sum
	j L28
L27:
	li $t0, 100
	li $t1, 1
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
L28:
	li $t0, 42
	lw $t1, 21($sp)	# arr
	li $t2, 0
	li $t3, 4
	mul $t3, $t2, $t3
	add $t3, $t1, $t3
	sw $t0, 0($t3)
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _printf
	la $a0, FMT29	# printf format
	lw $t0, 21($sp)	# arr
	li $t1, 0
	li $t2, 4	# elem_size
	mul $t2, $t1, $t2
	add $t2, $t0, $t2
	lw $t3, 0($t2)	# array elem
	move $a1, $t3	# printf arg 1
	jal _printf	# call _printf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	li $t0, 2
	li $t1, 3
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 8($sp)	# sum
	li $t1, 0
	mul $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 8($sp)	# sum
	li $t1, 0
	add $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	lw $t0, 8($sp)	# sum
	li $t1, 1
	mul $t2, $t0, $t1
	sw $t2, 8($sp)	# sum
	li $t0, 42
	sw $t0, 0($sp)	# a
	li $t0, 42
	sw $t0, 4($sp)	# b
	addiu $sp, $sp, -4
	sw $ra, 0($sp)	# save $ra for _printf
	la $a0, FMT30	# printf format
	li $t0, 42
	move $a1, $t0	# printf arg 1
	jal _printf	# call _printf
	lw $ra, 0($sp)	# restore $ra
	addiu $sp, $sp, 4
	li $t0, 0
	beq $t0, $zero, L31	# if false
	li $t0, 999
	sw $t0, 8($sp)	# sum
L31:
L33:	# while
	li $t0, 0
	beq $t0, $zero, L34
	li $t0, 999
	sw $t0, 8($sp)	# sum
	j L33
L34:
	li $t0, 0
	move $v0, $t0	# return val
	j main_exit	# return
main_exit:
	lw $ra, 36($sp)	# restore $ra
	addiu $sp, $sp, 40	# deallocate frame
	li $v0, 10	# exit syscall
	syscall
