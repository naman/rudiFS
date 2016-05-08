all:
	gcc simple_slash/ls.c -o simple_slash/ls
	gcc simple_slash/fget.c -o simple_slash/fget
	gcc simple_slash/fput.c -o simple_slash/fput
	gcc simple_slash/create_dir.c -o simple_slash/create_dir
	gcc simple_slash/chmod.c -o simple_slash/chmod
	gcc simple_slash/setacl.c -o simple_slash/setacl
	gcc simple_slash/getacl.c -o simple_slash/getacl
	chmod u+s simple_slash/ls simple_slash/create_dir simple_slash/fput simple_slash/fget simple_slash/setacl simple_slash/getacl simple_slash/chmod
	sudo chmod -R 777 ../rudiFS2/

permit:
	sudo chmod -R 777 ../rudiFS2/

clean:
	rm simple_slash/ls simple_slash/fput simple_slash/fget simple_slash/create_dir simple_slash/setacl simple_slash/getacl simple_slash/chmod

