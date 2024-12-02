start:
add $3,$12,$21
sub $14,$25,$10
and $16,$9,$4
or $22,$5,$30

main:
addi $28,$4,45
andi $24,$19,99

branch:
beq $14,$7,start
bne $16,$9,start

label:
sll $7,$6,5

j branch