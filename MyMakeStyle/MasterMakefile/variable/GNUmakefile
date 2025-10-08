.PHONY: all clean

foo = a.o b.o c.o
bar := $(foo:.o=.c)

# -------------------------------------
# override指令：禁止变量被覆盖
override overfoo = x
# -------------------------------------

all:
	@echo "bar = $(bar)"

	@echo ""

	@echo "overfoo = $(overfoo)"
