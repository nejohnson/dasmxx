Our test for dasm78k3 is the system ROM of the Kawai K4 revision 1.4 (as of
24-April-2014 available from Kawai website).

On Linux the command to convert a Motorla S-Rec file to binary is

    srec_cat k4_v14.mhx -Output k4_v14.bin -Binary

As well as testing dasm78k3 it also a work in progress to understand the 
software running inside this synthesizer.

Ports
-------------

P0.0	PA0
P0.1	PA1
P0.2	PA2
P0.3	PA3
P0.4	RSE
P0.5	R/W
P0.6	ECL
P0.7	MUTE

P1.0	PD0
P1.1	PD1
P1.2	PD2
P1.3	PD3
P1.4	PD4
P1.5	PD5
P1.6	PD6
P1.7	PD7

P2.0	(Data card) CDET
P2.1	TEV
P2.2	HOLD
P2.3	Low volt alarm
P2.4	TXD
P2.5	RXD
P2.6	KEY
P2.7	Not used

P3.0	Not used
P3.1	Not used
P3.2	Not used
P3.3	Not used
P3.4	BankSel
P3.5	BankSel
P3.6	PBend??
P3.7	PBend??

Ports 4 and 5 are used for Address/Data bus.

