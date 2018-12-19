#include "gba.h"
#include "exec.h"
#include "memory.h"
#include "debug.h"

#ifdef _DEBUG
//---------------------------------------------------------------------------
void FASTCALL SetExecFunction(u8 i)
{
   exec = arm.exec[bDebug][i];
}
//---------------------------------------------------------------------------
u8 arm_exec_d(void)
{
   u32 ins;
   u8 run;

   if(bPass)
       ControlBreakPoint();
   if(!bPass){
       UpdateRegister();
       WaitDebugger(FALSE);
   }
   run = 0;
	switch(((ins = OPCODE) >> 28)){
		case 0:
           if(ZFLAG)
               run = 1;
		break;
		case 1:
			if(!ZFLAG)
				run = 1;
		break;
		case 2:
			if(CFLAG) //CS
				run = 1;
		break;
		case 3:
	  		if(!CFLAG) //CC
	  			run = 1;
	  	break;
		case 4:
	  		if(NFLAG)
	  			run = 1;
	  	break;
		case 5:
	  		if(!NFLAG)
	  			run = 1;
	  	break;
		case 6:
	  		if(VFLAG)
	  			run = 1;
	  	break;
		case 7:
	    	if(!VFLAG)
	  	    	run = 1;
		break;
		case 8:
			if(CFLAG && !ZFLAG) // HI
				run = 1;
		break;
		case 9:
			if(!CFLAG || ZFLAG) // LS
	    		run = 1;
		break;
		case 10:
			if(NFLAG == VFLAG) //GE
				run = 1;
		break;
		case 11:
			if(NFLAG != VFLAG)
				run = 1;
		break;
		case 12:
			if(!ZFLAG && NFLAG == VFLAG)
				run = 1;
		break;
   	case 13:
			if(ZFLAG || NFLAG != VFLAG) //LE
				run = 1;
		break;
       default:
			run = 1;
       break;
	}
   run = run ? opcode_handles[((ins & 0x0FF00000)>>16)|(((u8)ins)>>4)]() : cycP;
   if(exec == arm_exec_d)
       advance_instruction_pipe();
   if(!bDebug)
       SetExecFunction((u8)((CPSR & T_BIT) >> 5));
	return run;
}
//---------------------------------------------------------------------------
u8 thumb_exec_d(void)
{
   u8 i;

   if(bPass)
       ControlBreakPoint();
   if(!bPass){
       UpdateRegister();
       WaitDebugger(TRUE);
   }
   i = opcode_handles_t[OPCODE_T >> 6]();
   if(exec == thumb_exec_d)
       tadvance_instruction_pipe();
   if(!bDebug)
       SetExecFunction((u8)((CPSR & T_BIT) >> 5));
   return i;
}
#endif
//---------------------------------------------------------------------------
u8 arm_exec(void)
{
   u8 run;

   run = cycP;
	switch(OPCODE >> 28){
		case 0:
           if(!ZFLAG)
               goto ex_arm_exec;
		break;
		case 1:
			if(ZFLAG)
               goto ex_arm_exec;
		break;
		case 2:
			if(!CFLAG) //CS
				goto ex_arm_exec;
		break;
		case 3:
	  		if(CFLAG) //CC
	  			goto ex_arm_exec;
	  	break;
		case 4:
	  		if(!NFLAG)
	  			goto ex_arm_exec;
	  	break;
		case 5:
	  		if(NFLAG)
	  			goto ex_arm_exec;
	  	break;
		case 6:
	  		if(!VFLAG)
	  			goto ex_arm_exec;
	  	break;
		case 7:
	    	if(VFLAG)
	  	    	goto ex_arm_exec;
		break;
		case 8:
			if(!(CFLAG && !ZFLAG))
				goto ex_arm_exec;
		break;
		case 9:
			if(!(!CFLAG || ZFLAG))
	    		goto ex_arm_exec;
		break;
		case 10:
			if(NFLAG != VFLAG) //GE
				goto ex_arm_exec;
		break;
		case 11:
			if(NFLAG == VFLAG)
				goto ex_arm_exec;
		break;
		case 12:
			if(!(!ZFLAG && NFLAG == VFLAG))
               goto ex_arm_exec;
		break;
   	case 13:
			if(!(ZFLAG || NFLAG != VFLAG))
				goto ex_arm_exec;
		break;
	}
   run = opcode_handles[((OPCODE & 0x0FF00000)>>16)|(((u8)OPCODE)>>4)]();
ex_arm_exec:
   if(exec == arm_exec)
       advance_instruction_pipe();
	return run;
}
//---------------------------------------------------------------------------
u8 thumb_exec(void)
{
   u8 i;

   i = opcode_handles_t[OPCODE_T >> 6]();
   if(exec == thumb_exec)
       tadvance_instruction_pipe();
   return i;
}

