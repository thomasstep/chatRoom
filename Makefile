CXX = crc.c
OBJS = crc

all:
	@g++ -std=c++11 $(CXX) -o $(OBJS)

clean:
	@rm $(OBJS)
