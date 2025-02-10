#message "test_0.inc - START"
#include "test_1.inc"	    ;; >><test_0>: .include .include <test_1.inc>
.MESSAGE "test_0.inc - Finish test_1.inc"
;;----------------------
	nop                 ;; >> test_0	
;;----------------------
#message "test_0.inc - END" ;; >> test_0.inc - Normal EOF
