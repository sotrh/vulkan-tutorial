CFLAGS = -std=c++11
LDFLAGS = -lvulkan

demo: main.cpp
	g++ $(CFLAGS) -o build/demo main.cpp $(LDFLAGS)

run: demo
	./build/demo

clean:
	rm -f build/demo