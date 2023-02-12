clang -Iinclude -c ./src/test.c -o ./bin/test.o -O2
clang -Iinclude -c ./src/qmath.c -o ./bin/qmath.o -O2
clang -Iinclude -c ./src/qpixel.c -o ./bin/qpixel.o -O2
clang -Iinclude -c ./src/qmesh.c -o ./bin/qmesh.o -O2
clang -Iinclude -c ./src/qtga.c -o ./bin/qtga.o -O2
clang ./bin/test.o ./bin/qmath.o ./bin/qpixel.o ./bin/qmesh.o ./bin/qtga.o -o test.exe
