.PHONY: all clean

ifeq ($(DEBUG),true)
# -g 生成调试信息, GNU可以进行调试
CC = g++ -g
VERSION = debug
else
CC = g++
VERSION = release
endif

# wildcard 函数的核心功能是扩展通配符并返回匹配的文件名
SRCS = $(wildcard *.cxx)

# 字符串中.cxx替换为.o
OBJS = $(SRCS:.cxx=.o)

# 字符串中.cxx替换为.d
DEPS = $(SRCS:.cxx=.d)

# 指定BIN二进制文件的位置
BIN := $(addprefix $(BUILD_ROOT)/, $(BIN))

# obj目标文件存放在同一目录下, 统一进行链接
LINK_OBJ_DIR = $(BUILD_ROOT)/app/link_obj
DEP_DIR = $(BUILD_ROOT)/app/dep

# -p参数递归创建目录
$(shell mkdir -p $(LINK_OBJ_DIR))
$(shell mkdir -p $(DEP_DIR))

# 目标文件生成到前述的目标文件目录中
OBJS := $(addprefix $(LINK_OBJ_DIR)/, $(OBJS))
DEPS := $(addprefix $(DEP_DIR)/, $(DEPS))

# 找到目录中的所有.o文件 (编译生成的.o目标文件)
LINK_OBJ = $(wildcard $(LINK_OBJ_DIR)/*.o)
LINK_OBJ += $(OBJS)

# ------------------------------------
# make 执行入口
all: $(DEPS) $(OBJS) $(BIN)

# 需要先判断$(DEPS)目录中的文件是否存在, 否则make会报找不到.d文件的错误
ifneq ("$(wildcard $(DEPS))", "")
include $(DEPS)
endif

# ------------------------------------


# ------------------------------------
# 生成可执行文件
# 命令参数: -o <file>: Place the output into <file>
$(BIN): $(LINK_OBJ)
	@echo "------------------------------------build $(VERSION) mode------------------------------------"
	$(CC) -o $@ $^

# ------------------------------------


# ------------------------------------
# 由.cxx源文件生成.o目标文件
# 命令参数: -o <file>: Place the output into <file>
# 			-c 		 : Compile and assemble, but do not link
$(LINK_OBJ_DIR)/%.o: %.cxx
	$(CC) -I$(INCLUDE_PATH) -o $@ -c $(filter %.cxx, $^)

# ------------------------------------


# ------------------------------------
# 生成.d依赖关系文件
# 记录.o目标文件与.h头文件的依赖关系
# 从而能够允许在.h头文件内容发生修改时
# make可以自动重新编译
# 命令参数: -M: Generate make dependencies with including std headers
# 			-MM: Generate make dependencies without including std headers
$(DEP_DIR)/%.d: %.cxx
	echo -n $(LINK_OBJ_DIR)/ > $@
	$(CC) -I$(INCLUDE_PATH) -MM $^ >> $@

# ------------------------------------


# ------------------------------------
# make clean执行入口
clean:
	@echo "------Done Nothing------"
#	rm -f $(BIN) $(OBJS) $(DEPS) *.gch
# ------------------------------------
