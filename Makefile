SERVER = crsd.c
CLIENT = crc.c
SERVEROUT = crsd.out
CLIENTOUT = crc.out
OBJS = crc.out crsd.out

all:
	@g++ -std=c++11 $(SERVER) -pthread -o $(SERVEROUT)
	@g++ -std=c++11 $(CLIENT) -pthread -o $(CLIENTOUT)

clean:
	@rm $(OBJS)
