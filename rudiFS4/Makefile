all:
	gcc simple_slash/fput_encrypt.c -o simple_slash/fput_encrypt -lcrypto -g -w
	gcc simple_slash/fput_validate.c -o simple_slash/fput_validate -lcrypto -g -w
	gcc simple_slash/fget_decrypt.c -o simple_slash/fget_decrypt -lcrypto -g -w

	chmod u+s simple_slash/fput_encrypt simple_slash/fput_validate simple_slash/fget_decrypt

permit:
	sudo chmod -R 777 ../rudiFS3/

clean:
	rm simple_slash/fput_encrypt simple_slash/fput_validate simple_slash/fget_decrypt simple_slash/.enc_temp simple_slash/.dec_temp 
