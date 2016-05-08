all:
	gcc simple_slash/fput_encrypt.c -o simple_slash/fput_encrypt -lcrypto -g
	gcc simple_slash/fput_validate.c -o simple_slash/fput_validate -lcrypto -g
	gcc simple_slash/fget_decrypt.c -o simple_slash/fget_decrypt -lcrypto -g

	chmod u+s simple_slash/fput_encrypt simple_slash/fput_validate simple_slash/fget_decrypt

	# sudo chmod -R 777 ../rudiFS3/

permit:
	sudo chmod -R 777 ../rudiFS3/

clean:
	rm simple_slash/fput_encrypt simple_slash/fput_validate simple_slash/fget_decrypt simple_slash/.enc_temp simple_slash/.dec_temp simple_slash/.hmac_temp 
