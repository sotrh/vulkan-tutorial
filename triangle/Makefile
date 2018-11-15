CFLAGS = -std=c++17
LDFLAGS = `pkg-config --static --libs glfw3` -lvulkan

demo: main.cpp
	g++ $(CFLAGS) -o build/demo main.cpp $(LDFLAGS)

.PHONY: test clean

test: demo
	./demo

clean:
	rm -f demo