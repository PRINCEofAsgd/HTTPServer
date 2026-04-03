all:client_input client_concurrent client_test

client_input:client_input.cpp
	g++ -g client_input.cpp -o client_input

client_concurrent:client_concurrent.cpp ../src/core/Buffer.cpp
	g++ -g client_concurrent.cpp ../src/core/Buffer.cpp -o client_concurrent -lpthread

client_test:client_test.cpp ../src/core/Buffer.cpp
	g++ -g client_test.cpp ../src/core/Buffer.cpp -o client_test -lpthread

clean:
	rm -f client_input client_concurrent client_test
