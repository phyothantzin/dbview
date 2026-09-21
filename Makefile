TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbcli

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(SRC_SRV:src/srv/%.c=obj/srv/%.o)

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

run: clean default
			./$(TARGET_SRV) -f ./mynewdb.db -n -p 8080

default: $(TARGET_SRV) $(TARGET_CLI)

clean:
				rm -f obj/srv/*.o
				rm -f bin/*
				rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
							mkdir -p bin
							gcc -o $@ $?

obj/srv/%.o: src/srv/%.c
						mkdir -p obj/srv
						gcc -c $< -o $@ -Iinclude

$(TARGET_CLI): $(OBJ_CLI)
							mkdir -p bin
							gcc -o $@ $?

obj/cli/%.o: src/cli/%.c
						mkdir -p obj/cli
						gcc -c $< -o $@ -Iinclude
