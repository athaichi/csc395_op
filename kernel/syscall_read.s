.global SYS_READ
.global syscall_read

# This is the interrupt handler routine called when a system call is issued
syscall_read:
  # The %rax register holds the sixth syscall argument. Put it on the stack.
  push %rax

  # Call the C-land syscall handler
  call SYS_READ

  # The %rax register now holds the return value. Move the stack up without overwriting %rax.
  add $0x8, %rsp

  # Return from the interrupt handler
  iretq