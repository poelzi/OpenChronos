/*
	__even_in_range
	pseudo code ==> if __value not even  or out of range return 0
	This code assumes that the two paremeters are passed in R12 and R14
	and the return is passed back in R12
*/

		.name __even_in_range
		.text 
__even_in_range:
      bit.b #0, r12  ; //test: __value is even?
      jnz NotValid
      cmp.b r12, r14  ; // test: __value in range?
      jlo Exit
NotValid:           ; not valid set r12 = 0 else leave passed paremeter in r12 to be returned
      mov.b #0, r12
Exit:
      ret
	  .global __even_in_range
      
       
