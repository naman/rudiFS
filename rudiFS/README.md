# rudiFS
A rudimentary unix-like filesystem supporting multi-users and groups for CSE552 - Security Engineering. The client-server software emulates an elementary unix-like filesystem which handles permissions (users and groups). The system also has a root user with an all-acess pass. Each connected user is a separate child thread with it's own resources. When a client login, automatically the environment is created with the directory structure with the necessary permissions. Once logged in, the client sees a custom made linux shell (using fork), where each command is a separate process with it's own resources. The software defends against possible loopholes in client entering various inputs at various stages in the program. The software defends against possible loopholes in client login when and if the server is disconnected when client is entering their username. This is prevented by trapping signals and sending them to the child threads, when required.


To install or test the software, see Install and Test section.

The software is opensourced under ![GNU GPLv3 Licence](LICENCE)


## Libraries
Posix threads library, fork(), opendir(), readdir()


## Working Commands
ls, create_dir, fget, fput


## Install and Test
Compile using the Makefile provided.

1. `make`
2. run `./server`
3. run multiple clients in different terminal windows/tabs using `./client`


## Assumptions
1. There is a group for every user.
2. You can have multiple groups, with many members. The mapping can be done in ![.groups](simple_slash/.groups)
3. Initially, we have a list of allowed users which can be found in ![.allowed_users](simple_slash/.allowed_users). To allow another user, simply edit this file.
4. Linux supports filenames upto 256 characters, the software abides by the rule. (and various other limits mentioned below).
5. The permissions are set automatically once a folder is created using `create_dir` or a file is created using `fput` commands. The relevant permissions are set in ![.permissions](simple_slash/.permissions) which follows the format `@path;@owner;@groups/users`
6. Permissions can be added through software, but if you wish to reduce them - do it manually by just removing the old entry.
7. We can set the permissions for all newly created directories indivisually using `create_dir`(recursive mkdir). 
8. Path length has to be < 4096 bytes long. It can contain any special characters, just like UNIX based filesystems do. We cannot ls/autocomplete/fget(cat) using the paths containing those special characters.
Eg. of some special characters are ^[[A, ^[[D, ^[[B, ^[[C
9. A file and directory may not have the same names ( if they are in the same folder), to allow conflict in reading permissions.
10. The paths before simple_slash have been blaclisted and cannot be fget-ed or ls-ed.


## Attacks/Bugs/Errors

1. < 256 character naming convention for file is followed everywhere, < 4096 characters for path is followed and <32 characters for usernames.

2.  When a regular client logs in, the .permission files are checked to avoid multiple entries of permissions with the same file.

3. A new permission entry is created if a file is overwritten AND has a newer set of group privileges. In this case, we don't need to delete the old permission entries, the software takes care of the newly added group priviledges.

4. We can set the permissions for all newly created directories indivisually using `create_dir`(recursive mkdir). 

5. fgets fails for very very large inputs approx 2 lakh characters eg. ![error_inputs](error_inputs.txt)

6. handles storing of permissions of each direcrory when recursively creating directories.

7. If file is overwritten, the system prevents transfer of ownership.

8. Handles Ctrl-C and Ctrl-\ for preventing killing the server. 

9. The software defends against possible loopholes in client login when and if the server is disconnected when client is entering their username. This is prevented by trapping signals and sending them to the child threads, when required.

10. The paths before simple_slash have been blaclisted and cannot be fget-ed or ls-ed.