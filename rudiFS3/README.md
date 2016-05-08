# rudiFS v3

Continuation of the second assignment: Extensions to the rudimentary unix-like filesystem supporting multi-users and groups for CSE552 - Security Engineering. The new system imitates encrypt, decrypt and compute HMAC functions in OpenSSL.

To install or test the software, see Install and Test section. The software is opensourced under ![GNU GPLv3 Licence](LICENCE)


## Libraries
opendir(), readdir() etc.


## Working Commands
`fget_decrypt`, `fput_encrypt`, `fput_validate`


## Install and Test
Compile using the Makefile provided.

1. `make`

2. Create these aliases in `~/.bashrc`

	`alias fput_encrypt='<path_to_rudiFS2>/simple_slash/fput_encrypt'`
	`alias fput_validate='<path_to_rudiFS2>/simple_slash/fput_validate'`
	`alias fget_decrypt='<path_to_rudiFS2>/simple_slash/fget_decrypt'`

3. Run `source ~/.bashrc`

4. Create multiple users with one user as "fakeroot" which mimicks root in unix-based systems.

## Assumptions and Attacks/Bugs/Errors
1. HMAC is generated with `fput_encrypt` with a file.hmac. The HMAC requires passphrase too. An integrity check can be done by using `fput_validate` which takes passphrase as input and verifies the integrity of the file. If the file content is tampered with, the programs throws an error saying "Error! This file has been tampered with.".

2. I have used temp files `.enc_temp`, `.dec_temp` and `.hmac_temp` for comparing and storing the temp key, iv and hmac along with the required permissions set for these.

3. All the three are setuid programs, whith the euid being reset once the privileged operations is finished.

4. If the file already exists, fput_encrypt appends the new content to the file and cretes a new hmac for verification.

5. I have used AES 256 in CBC mode, where the key size is 256 bits and IV is 128 bits by default. Plaintext of arbitrary length works fine. Passphrase can also be of arbitrary size.