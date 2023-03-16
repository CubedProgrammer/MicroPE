# MicroPE
Micro Privilege Escalator

Run commands as the superuser, the root account.

The first argument to the program is the command to run, the remaining arguments are arguments to that command.

The command `mupe rm -r /boot` will run `rm -r /boot` as the superuser.

Since the Greek letter μ is not fun for everyone to type, `mupe` was chosen as the command name.
If you are Greek and prefer `μpe`, you can use that instead.
## Configuration
The file /etc/mupe.conf has the following format.
```
%d
%s
%s
%s
```
The file will be read using fscanf accordingly.

The first line is the maximum number of seconds to allow a user to run commands without authorizing, after authorizing once.

The second line is the user whose password is asked for, if it starts with $ then the environment variable is used instead.
Use $USER so every user has to type their own password.

The third line is the list of users permitted, separated by commas and no spaces, names only, IDs not accepted.

The fourth line is the list of groups permitted, if a user is in any group it will be permitted.
## Compilation
Open your terminal in the base directory of this repository.
```
cc -O3 -c conf.c main.c
cc -o mupe conf.o main.o -lcrypt
```
## Installation
```sh
sudo chown root:root mupe
sudo chmod 4755 mupe
sudo cp mupe.conf /etc/mupe.conf
./mupe mv mupe /usr/local/bin/mupe
```
The default mupe.conf is probably not what you are looking for, this will emulate the default behaviour of sudo closely.
```
360
$USER
"Your Username"
sudo,wheel
```
Replace third line with whatever your username is, without any quotes.
## Security
Reading from the configuration file does not check for stack smashing, since only administrators can modify it.

Password buffer is overwritten with encrypted password.
If encrypted password is longer, which it usually is, your password will be erased from memory.

If a pseudo-terminal is closed, but a command was run on it and the persist timer is not zero, there may be an issue.
A program can create a new pseudo-terminal, if it has the same name as the old pseudo-terminal, it can run commands.

For example, suppose a command was run on /dev/pts/4, the persist timer is 240 seconds, and the terminal is closed.
Before the time is up, a program can open a new terminal, if it happens to also become /dev/pts/4, it can run commands.
