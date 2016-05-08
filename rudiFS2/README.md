# rudiFS v2

Continuation of the first assignment: Extensions to the rudimentary unix-like filesystem supporting multi-users and groups for CSE552 - Security Engineering. The new system imitates the unix-permissions (DACs and ACLs).

To install or test the software, see Install and Test section. The software is opensourced under ![GNU GPLv3 Licence](LICENCE)


## Libraries
opendir(), readdir() etc.


## Working Commands
list, create_dir, fget, fput, modify, setacl, getacl


## Install and Test
Compile using the Makefile provided.

1. `make`

2. Create these aliases in `~/.bashrc`

	`alias list='<path_to_rudiFS2>/simple_slash/ls'`
	`alias fget='<path_to_rudiFS2>/simple_slash/fget'`
	`alias fput='<path_to_rudiFS2>/simple_slash/fput'`
	`alias create_dir='<path_to_rudiFS2>/simple_slash/create_dir'`
	`alias modify='<path_to_rudiFS2>/simple_slash/chmod'`
	`alias setacl='<path_to_rudiFS2>/simple_slash/setacl'`
	`alias getacl='<path_to_rudiFS2>/simple_slash/getacl'`

3. Run `source ~/.bashrc`

4. Create multiple users with one user as `fakeroot` which mimicks root in unix-based systems.

## Assumptions and Attacks/Bugs/Errors
1. There is a group for every user.

2. You can have multiple groups, with many members. The mapping can be done in ![.groups](simple_slash/.groups)

3. Linux supports filenames upto 256 characters, the software abides by the rule. (and various other limits mentioned below).

4. The DAC permissions are set automatically once a folder is created using `create_dir` or a file is created using `fput` commands. The relevant permissions are set in ![.permissions](simple_slash/.permissions) which follows the format `@path;@owner;@groups/users`

5. DAC Permissions can be modified using `modify [path]` command.

6. We can set the DAC permissions for all newly created directories indivisually using `create_dir`(recursive mkdir). 

7. Path length has to be < 4096 bytes long. It can contain any special characters, just like UNIX based filesystems do.
Eg. of some special characters are ^[[A, ^[[D, ^[[B, ^[[C

8. The paths before simple_slash have been blaclisted and cannot be fget-ed or ls-ed.

9. Since it is not alllowed to have the write permissions right away when you create a file, you need to set `sudo chmod a+w <path_to_file>` to allow other user's to write on the file. Unix prevents the file's permissions to be inherited from the parent directory.

10. Every executable and file/directory is initially owner by fakeroot.

11. The permissions are relinquished right after a privileged task by `setuid(caller_id)` folowing the principle of least priviledges. 

12. We can set the permissions for all newly created directories indivisually using `create_dir`(recursive mkdir). 

13. fgets fails for very very large inputs approx 2 lakh characters eg. ![error_inputs](error_inputs.txt)

14. If file is overwritten, the system prevents transfer of ownership. The systems prevents that only allowing the users having requisite permissions to modify the file without changing the ownership.

15. [For non-fakeroot users] if you need to write and read a file, you have to explicitly set acl permissions inside the file.

16. Permission mechanism is as follows: 
	Permission Ticket = check_DAC_bit && check_ACL_bit

	So, Permission Ticket = 1 if check_DAC_bit  == 1 && check_ACL_bit == 1.

17. For a file, the ACLs for a file is stored on the first line of every file like `@owner;@groups/users`. For every directory, the ACLs are stored in a separate `directory.acl` inside the respective directory. It is to be noted that `setacl` command NEED to be explicitly called to setup the ACLs.

18. The ACL's are not checked and only DAC is checked while using the `setacl` and `getacl` commands for obvious reasons ie. If a user is not allowed to `getacl` he/she is surely not allowed to read the file itself (or directory.acl), so only DAC is checked in this case. OR Only DAC is required for getacl because it wouldnt matter if we arent allowed to even read the file.

19. As an extension to the previous assignment, many users (if allowed), can append their contents to the same file, without changing the actual owner of the file.

20. `fget` starts reading from the second line of the file, and only displays the content of the file thereby disallowing any security information leak.

21. The ACLs created for each directory ie. `directory.acl` are also secured under DAC mechanism to disallow possible overwrite or read by any adversary. 

22. The system prints the actual userID/username and the effective user immediately before and after accessing the file or directory (just to make sure that the programs are working correctly).4

23. `modify` can be used to change the DAC permissions (just like `chmod`) of a file or directory without changning the ownership. Only the file owner can change the DAC permissions. Note, that earlier permissions have tp be removed after the command runs. Also, `setacl` can be used to set ACLs for a directory or file. Only the file owner can change the ACL permissions.

24. run `make permit` to allow * every file the required unix permissions if in any case an error due to "unix permissions" occur. This possibly resoves all the error that can crop up if any user doesnt have access to someone else's (UNIX) directory.

25. `make` command is to be run by 'fakeroot' so that it becomes the owner. Anyone can else also run the command without any consequences.

26. The system automatically writes permission for fakeroot, and allows it full acess to every file and directory without any permission checks or hassles. So it prevents against possible "deadlock DOS" if anyone tries to change the owner to "null" manually. fakeroot will always be able to acess a file no matter what.

