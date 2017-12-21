# BashSyslogHistoryLib
A library that helps to send bash history to syslog without modifying bash binary

Works for x86_64 architecture only

Compiling and Running:
$ make
$ LD_PRELOAD=/path/to/bashsyslog.so /bin/bash

It is better to add LD_PRELOAD to 'login' and 'sshd' pam stack, tapping into pam_env.so module

Syslog Facility is user.info

