; Print.s
; Student names: change this to your names or look very silly
; Last modification date: change this to the last modification date or look very silly
; Runs on LM4F120 or TM4C123
; EE319K lab 7 device driver for any LCD
;
; As part of Lab 7, students need to implement these LCD_OutDec and LCD_OutFix
; This driver assumes two low-level LCD functions
; ST7735_OutChar   outputs a single 8-bit ASCII character
; ST7735_OutString outputs a null-terminated string 

num EQU 0
count EQU 4
linkin EQU 8
div EQU 12

    IMPORT   ST7735_OutChar
    IMPORT   ST7735_OutString
    EXPORT   LCD_OutDec
    EXPORT   LCD_OutFix

    AREA    |.text|, CODE, READONLY, ALIGN=2
    THUMB

  

;-----------------------LCD_OutDec-----------------------
; Output a 32-bit number in unsigned decimal format
; Input: R0 (call by value) 32-bit unsigned number
; Output: none
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutDec

		SUB SP,#16 ;allocate memory on stack for local variables
		STR LR,[SP,#linkin] ;store link
		STR R0,[SP,#num] ;store input in num
		MOV R2,#0
		MOV R1,#10
dig		ADD R2,#1 ;R2 => counts how many digits are in num
		UDIV R0,R0,R1
		CMP R0,#0
		BEQ output
		B dig
output	LDR R0,[SP,#num]
		MOV R1,#10
		MOV R12,#10
		STR R2,[SP,#count]
		CMP R2,#1 ;if there is only one digit left, output it
		BEQ print
		CMP R2,#2
		BNE divider
		UDIV R0,R0,R1
		STR R1,[SP,#div]
		B print
divider	MUL R1,R1,R12
		SUB R2,#1
		CMP R2,#2
		BNE divider
		UDIV R0,R0,R1 ;separates first digit
		STR R1,[SP,#div]
		CMP R2,#1
		BNE print
		LDR R0,[SP,#num]
print	ADD R0,#0x30 ;convert number to ASCII character
		BL ST7735_OutChar ;output ASCII character
		LDR R0,[SP,#num]
		LDR R2,[SP,#count]
		LDR R2,[SP,#div]
		UDIV R3,R0,R2
		MUL R3,R2,R3
		SUB R0,R0,R3
		STR R0,[SP,#num] ;update number w/o recently outputted digit
		LDR R2,[SP,#count]
		SUB R2,#1
		STR R2,[SP,#count] ;update digit counter
		CMP R2,#0
		BNE output
		LDR LR,[SP,#linkin] ;restore link register
		ADD SP,#16 ;deallocate stack


      BX  LR
;* * * * * * * * End of LCD_OutDec * * * * * * * *

; -----------------------LCD _OutFix----------------------
; Output characters to LCD display in fixed-point format
; unsigned decimal, resolution 0.001, range 0.000 to 9.999
; Inputs:  R0 is an unsigned 32-bit number
; Outputs: none
; E.g., R0=0,    then output "0.000 "
;       R0=3,    then output "0.003 "
;       R0=89,   then output "0.089 "
;       R0=123,  then output "0.123 "
;       R0=9999, then output "9.999 "
;       R0>9999, then output "*.*** "
; Invariables: This function must not permanently modify registers R4 to R11
LCD_OutFix

		MOV R2,#0
		SUB SP,#16 ;allocate memory on stack for local variables
		STR LR,[SP,#linkin] ;store link
		STR R0,[SP,#num] ;store input in num
		MOV R1,#10
dig2	ADD R2,#1 ;R2 => counts how many digits are in num
		UDIV R0,R0,R1
		CMP R0,#0
		BEQ fixed
		B dig2
fixed	STR R2,[SP,#count]
		CMP R2,#3
		BEQ three ;if there are three digits, branch to three
		BHI starz ;if more than three digits, output starz
		CMP R2,#2
		BEQ two ;if there are two digits, branch to two
		CMP R2,#1
		BEQ two
three	LDR R0,[SP,#num]
		MOV R1,#100
		UDIV R0,R0,R1
		ADD R0,#0x30
		BL ST7735_OutChar ;output ones place digit
		LDR R0,[SP,#num]
		MOV R1,#100
		UDIV R0,R0,R1
		MUL R2,R0,R1
		LDR R0,[SP,#num]
		SUB R0,R0,R2
		STR R0,[SP,#num] ;remove outputted digit
		MOV R0,#0x2E ;output decimal point
		BL ST7735_OutChar
two2	LDR R0,[SP,#num]
		MOV R1,#10
		UDIV R0,R0,R1
		ADD R0,#0x30
		BL ST7735_OutChar ;output tenths place digit
		LDR R0,[SP,#num]
		MOV R1,#10
		UDIV R0,R0,R1
		MUL R2,R0,R1
		LDR R0,[SP,#num]
		SUB R0,R0,R2
		STR R0,[SP,#num] ;remove outputted digit
one2	LDR R0,[SP,#num]
		ADD R0,#0x30
		BL ST7735_OutChar ;output hundredths place digit
		B done
two		MOV R0,#0x30
		BL ST7735_OutChar ;output 0 for ones place digit
		MOV R0,#0x2E
		BL ST7735_OutChar ;output decimal point
		B two2
one		MOV R0,#0x30
		BL ST7735_OutChar ;output 0 for ones place digit
		MOV R0,#0x2E
		BL ST7735_OutChar ;output decimal point
		MOV R0,#0x30
		BL ST7735_OutChar ;output 0 for tenths place digit
		MOV R0,#0x30
		BL ST7735_OutChar ;output 0 for hundredths place digit
		LDR R0,[SP,#num]
		CMP R0,#0
		BEQ done
		B one2
starz	MOV R0,#'*'
		BL ST7735_OutChar
		MOV R0,#0x2E
		BL ST7735_OutChar
		MOV R0,#'*'
		BL ST7735_OutChar
		MOV R0,#'*'
		BL ST7735_OutChar
done	LDR LR,[SP,#linkin]	;restore link register
		ADD SP,#16 ;deallocate stack

     BX   LR
 
     ALIGN
;* * * * * * * * End of LCD_OutFix * * * * * * * *

     ALIGN                           ; make sure the end of this section is aligned
     END                             ; end of file
