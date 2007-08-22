.exports
	&MAINOBJ

.locals
FD:	$0000
OUT:	$0000


.code

;--------------------------------------------------
; PRINTFN: Prints accumulator to FD
;--------------------------------------------------
PRINTFN:
	link $0000
  	push
	pushi 48
	add
  	sal 1
	pushi 2
	lsl 0
	pushi &OUT
  	callk $2a $4
	ret

;--------------------------------------------------
; TESTSTRING: Try string in acc
;--------------------------------------------------
TESTSAID:
	pushi 2
	push
	pushi &EVENTOBJ
	callk $24 $4	; Parse
	pushi 1
	pushi &SPEC0
	callk $25 $2	; Said

	pushi 0
	call &PRINTFN 0
	ret
  	

;--------------------------------------------------
; MAINFN: Main entry point
;--------------------------------------------------
MAINFN:	SelfID

	;; File initialisation
	pushi 2
	pushi &FILENAME
	pushi 2		; Create anew
	callk $29 $4	; FOpen
	sal 0		; fd

;;	pushi 0
;;	callk $57 $0	; Debug
	;; Begin main functionality

	ldi	&PARSESTRING0
	pushi	0
	call	&TESTSAID 0

	;; Deinitialisation: Close output file
	pushi 1
	lsl 0		; fd
	callk $2c $2	; FClose
	ret		; Quit
	
.class
	$1234
	$0000			; Locals
	18			; Functarea
	4			; # of varselcs
MAINOBJ:
	;; Varselectors
	0
	0
	$8000
	&NAME
	
	$0			; species
	$1			; superclass
	$2			; -info-
	$17			; Name

	;; Funcselectors
	
	1			; One overridden
	$2a			; play
	0			; dummy
	&MAINFN

.object
	$1234
	$0000			; Locals
	20			; Functarea
	10			; # of varselcs
EVENTOBJ:
;  	$0;  species[0000] = 3398 %Event
;	$1;superClass[0001] = 2ccc %Obj
;	$2;  -info-[0002] = 8000 
;	$17;  name[0017] = 33fa 
;	$22;  type[0022] = 0000 (0)
;	$28;  message[0028] = 0000 (0)
;	$40;  modifiers[0040] = 0000 (0)
;	$3;  y[0003] = 0000 (0)
;	$4;  x[0004] = 0000 (0)
;	$4c;  claimed[004c] = 0000 (0)

	;; Varselectors
	7			; 
	7			; Event
	$0000
	&EVENTNAME
	$80			; type
	0			; message
	0			; modifiers
	0			; x
	0			; y
	0			; claimed

	;; Funcselectors
	
	0			; Zero overridden
	0			; dummy

.strings
NAME:		"said"
EVENTNAME: 	"simple-event"
PARSESTRING0:	"shine"
FILENAME:	"PARSE.000"

.said
;SPEC0:	$48b < $142 / $93c !
SPEC0:	$48b ! !
