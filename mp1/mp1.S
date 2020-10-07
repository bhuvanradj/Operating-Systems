
.data					# section declaration
        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12
        STRUCT_SIZE = 16
	
	# 80*25-1
	MAXSIZE = 1999
	
	jump_table:
	.long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0
.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
        
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret

mp1_rtc_tasklet:
        #TODO 3
    	
#	pushl %ebp		save old frame
#	movl %esp, %ebp		pointer to new frame
	pushl %esi		#save source index
	pushl %edi		#save destination index
	pushl %ebx		#save base
    
	movl mp1_list_head, %ecx	#check if list is empty
	cmpl $0, %ecx	
	je invalid		#if empty return -1
	movl %ecx, %ebx		#if not then put head in edx
	
    traverse:	
	cmpl $0, %ebx		#check if list is at end node
	je done			#traversal complete
	movw COUNTDOWN(%ebx), %dx	#check countdown field
	addw $-1, %dx
	movw %dx, COUNTDOWN(%ebx)
	cmpw $0, COUNTDOWN(%ebx)	#if ctdwm=0 then call poke
	je pokeclear		#clear appropriate regs to call poke
	jmp next_elem		#if not then go to next elem
    
    status:			
	cmpw $0, STATUS(%ebx)	#check status
	je offchar		#if zero set offchar
	jmp onchar		#if not set onchar
   
   offchar:			
	movb ON_CHAR(%ebx), %cl	#move onchar into poke param
	movzwl LOCATION(%ebx), %eax	#move correct offset into param
	shll $1, %eax		#multiply by 2 to get correct offset
	call mp1_poke		#call poke
	jmp off_up		#update rest of node
   
   onchar:			
	movb OFF_CHAR(%ebx), %cl	#move offcharr into poke param
	movzwl LOCATION(%ebx), %eax	#move correct offset into param
	shll $1, %eax		#multiply by 2 to get correct offset
	call mp1_poke		#call poke
	jmp on_up		#update rest of node
	

    off_up:
    	movw ON_LENGTH(%ebx), %ax	#opposite length goes to countdown
	movw %ax, COUNTDOWN(%ebx)
	movw $1, STATUS(%ebx)		#status set to 1
	jmp next_elem		#go to next elem and traverse
    on_up:			
	movw OFF_LENGTH(%ebx), %ax	#opposite length goes to countdown
	movw %ax, COUNTDOWN(%ebx)
	movw $0, STATUS(%ebx)		#status set to 0
	jmp next_elem		#go to next elem and traverse

    next_elem:			#update pointer to next pointer and traverse
	movl NEXT(%ebx), %ebx
	jmp traverse
    
    pokeclear: 			# clears registers needed for poke call
	xorl %ecx, %ecx
	xorl %eax, %eax
	jmp status		#check status 
    
    done:		# retern when done
	popl %ebx
	popl %edi
	popl %esi	
	ret

    invalid:			#return -1
	movl $-1, %eax
	jmp done

mp1_ioctl:
	#TODO 1
	
	movl 8(%esp), %eax	#cmd -> eax
	cmpl $3, %eax		
	ja rneg1		#cmd > 3 return -1	
	#cmpl $0, %eax		cmd < 0 return -1
	#jl rneg1
	jmp *jump_table(,%eax,4) # switch(cmd)
	
    rneg1:			#return -1
	movl $-1, %eax		
	ret

mp1_ioctl_add: 
	#TODO 2
	
	pushl %ebp 		#push frame onto stack
	movl %esp, %ebp 	#new frame pointer in ebp
	pushl %esi		#save source index
	pushl %edi		#save destination index
	pushl %ebx		#save base
	
	movl 8(%ebp), %eax	#check if arg is 0
	cmpl $0, %eax
	je errorsmall 
	
	pushl $STRUCT_SIZE	#parameter for malloc call
	call mp1_malloc		#allocate memory 
	popl %ecx		#restore esp  after pushing size
    	xorl %ecx, %ecx
	cmpl $0, %eax		#check ret val of malloc
	je error

	pushl %eax 		#push struct pointer onto stack
	pushl $STRUCT_SIZE 	#parameter for copy_from_user
	pushl 8(%ebp)		#parameter for copy_from_user
	pushl %eax		#parameter for copy_from_user
	movl %eax, %ebx		#put frame pointer in ebx to make copy
	call mp1_copy_from_user	#call copy_from_user
	addl $12, %esp		#restore stack pointer
	cmpl $0, %eax		
	jne error		#return if copy fail
	cmpw $MAXSIZE, LOCATION(%ebx) #See if location is within limits
	ja error
	
	movw ON_LENGTH(%ebx) , %cx	#onlength -> countdown
	movw %cx , COUNTDOWN(%ebx)
	movw $1, STATUS(%ebx)	#set status to 1
	
	popl %ecx		#pop pointer of frame to ecx
	movl mp1_list_head , %eax	#head node pointer in eax
	movl %eax, NEXT(%ecx)	#set next pointer
	movl %ecx, mp1_list_head	#new head pointer set
	movzwl LOCATION(%ecx), %eax	#OFFSET in eax
	shll $1, %eax		#multiply offset by 2 to account for byte mode
	movl %ecx, %edx
	movb ON_CHAR(%edx), %cl	#put ASCII in cl
	call mp1_poke		#call poke
	jmp pass		#finish function
    
    error: 			#free allocated memory and return -1
	call mp1_free
    
    errorsmall:
	movl $-1, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave 
	ret
    pass:			#pop registers and return 0
	movl $0, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave 
	ret
        
mp1_ioctl_remove:
	#TODO 6
	
	pushl %ebp		#standard set up as all other subroutines
	movl %esp, %ebp
	pushl %esi
	pushl %edi
	pushl %ebx
    
	movl mp1_list_head, %ecx	#head pointer in ecx
	movl 8(%ebp), %edx	#arg in edx
	
    #temp code begin
    edgecase:
	cmpl $0, %ecx
	je neg1
	cmpw LOCATION(%ecx), %dx
	jne skip
	movl %ecx, %ebx
	movl NEXT(%ecx), %ecx
	movl %ecx. mp1_list_head
	pushl %ebx
	call mp1_free
	addl $4, %esp
	jmp wedone

    skip:
	movl mp1_list_head, %ecx	#head pointer in ecx
	movl 8(%ebp), %edx	#arg in edx
	jmp looper
   #temp code end
    
    looper:			#find element with matching location
        cmpl $0, %ecx		#if list is at end, return
        je neg1
        cmpw LOCATION(%ecx), %dx
        je remove		#if location matches, remove it
        movl %ecx, %ebx		#prev elem
	movl NEXT(%ecx), %ecx	#go to next elem
        jmp looper
    
    remove:
	movl NEXT(%ecx), %eax	#prev->next = curr->next
	movl %eax, NEXT(%ebx)	
	jmp freemem
    
    freemem:			#free the node
	cmpl $0, %ecx
	je neg1
	pushl %ecx
	call mp1_free
	addl $4, %esp
	jmp wedone
    
    wedone:			#free callee-saved registers and return 0
    	movl $0, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave 
	ret
    
    neg1:			#free call-saved registers and return -1
	movl $-1, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave
	ret

mp1_ioctl_find:
	#TODO 4
	
	pushl %ebp		#standard setup as all other subroutines
	movl %esp, %ebp
	pushl %esi		
	pushl %edi
	pushl %ebx
	
	movl 8(%ebp), %edx	#check if arg is valid or NULL
	cmpl $0, %edx
	je fail			#if NULL return -1
	movl %edx, %ebx		#base gets struct pointer
	
	movw LOCATION(%ebx) , %ax	#put location param in edx
	movl mp1_list_head, %ecx	#head put in ecx

    travloop:			#loop through list to find matching location
	cmpl $0, %ecx
	je fail
	cmpw %ax, LOCATION(%ecx)
	je copy			#if found, copy
	movl NEXT(%ecx), %ecx	#if not go to next elem
	jmp travloop
    
    copy:
	pushl $STRUCT_SIZE	#size param for copy call
	pushl %ecx		#from pointer for copy call
	pushl %edx		#to pointer for copy call
	call mp1_copy_to_user	#call copy to user
	addl $12, %esp		#restore stack pointer
	cmpl $0, %eax
	jne fail			#return -1 if copy fail
	jmp success 		#return 0 if copy succeeds
    
    success:
	movl $0, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave 
	ret
    
    fail:		
	movl $-1, %eax		#-1 -> eax
	popl %ebx		#pop all initally saved registers
	popl %edi	
	popl %esi
	leave			#return
	ret

mp1_ioctl_sync:
	#TODO 5
   
	pushl %ebp		#standard setup as all other subroutines
	movl %esp, %ebp
	pushl %esi
	pushl %edi
	pushl %ebx

	xorl %ecx, %ecx		#clear ecx and edx for usage
	xorl %edx, %edx		
	
	movl 8(%ebp), %edx	#put arg into edx
	movw %dx, %cx		#location B in cx
	shrl $16, %edx		#location A in dx
	movl mp1_list_head, %ebx	#head pointer in ebx
    
    aloop:
	cmpl $0, %ebx		#if list is at end return
	je fayl			
	cmpw %dx, LOCATION(%ebx)	#if locations match, save pointer data
	je save		
	movl NEXT(%ebx), %ebx	#go to next elem
	jmp aloop
    
    save:
	movl %ebx, %esi		#save pointer to esi
	movl mp1_list_head, %ebx	#update ebx with head again
	jmp bloop		#go to find second match
    
    bloop:
	cmpl $0, %ebx		#if list is at end return
	je fayl			
	cmpw %cx, LOCATION(%ebx)	#if locations match, save pointer data
	je copydata
	movl NEXT(%ebx), %ebx		#go to next elem
	jmp bloop
				#first elem data in esi
				#second elem data in ebx
    copydata:
	xorl %ecx, %ecx		
	xorl %eax, %eax		#clear registers for use
	movw ON_LENGTH(%esi), %cx	#copy on length
	movw %cx, ON_LENGTH(%ebx)
	
	movw OFF_LENGTH(%esi), %cx	#copy off length
	movw %cx, OFF_LENGTH(%ebx)
	
	movw COUNTDOWN(%esi), %cx	#copy countdown
	movw %cx, COUNTDOWN(%ebx)
	
	movw STATUS(%esi), %cx		#copy status
	movw %cx, STATUS(%ebx)
    
    checkstatus:		#check status
	cmpw $0, %cx		
	je onc			#if zero output offchar
	jmp offc		#if one output onchar		
    
    onc:
	movb ON_CHAR(%ebx), %cl	#put on char in cl
	movzwl LOCATION(%ebx), %eax	#put location ax	
	shll $1, %eax		#multiply by 2
	call mp1_poke		#output
	jmp good		#return success
    
    offc:
	movb OFF_CHAR(%ebx), %cl	#put off char in cl
	movzwl LOCATION(%ebx), %eax	#put location in ax
	shll $1, %eax		#multiply by 2
	call mp1_poke		#output
	jmp good		#return success
	
    
    good:			#return 0 for success
	movl $0, %eax
	popl %ebx	
	popl %edi
	popl %esi
	leave 
	ret
    
    fayl:			#return -1 for fail
	movl $-1, %eax
	popl %ebx
	popl %edi
	popl %esi
	leave 
	ret
.end
