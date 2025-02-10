
.MESSAGE "test.inc - START"
.INCLUDE "test_0.inc" 	  ;; >><test.inc>: .include .include <test_0.inc>
.MESSAGE "test.inc - Finish test_0.inc"
.INCLUDE "test_1.inc" 	  ;; >><test.inc>: .include .include <test_1.inc>
.MESSAGE "test.inc - Finish test_1.inc"
.EXIT		      	  ;; >>test: 'Warning-Needed': Forced abort test.inc
;;-------------------
	nop               ;; >>test	
;;-------------------
.MESSAGE "test.inc - END" ;; >> test.inc - normal EOF
		
